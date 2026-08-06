"""
Background task (started from main.py's lifespan) that periodically fetches every
Home-Assistant-bound value element's current state and, on change, updates
ElementLiveValue and recomputes desired_content_hash for affected displays — the same
recompute path a manual design edit goes through (see rendering.recompute_desired_hashes).
This is what makes the poller feed into the existing full/diff sync machinery for free.
"""

import asyncio
import logging
import os

from sqlalchemy import select
from sqlalchemy.orm import Session

from app.database import SessionLocal
from app.models.db import Design, ElementLiveValue, HomeAssistantConfig
from app.services.home_assistant import HomeAssistantError, fetch_entity_value
from app.services.rendering import recompute_desired_hashes

logger = logging.getLogger("app.ha_poller")

POLL_INTERVAL_S = float(os.environ.get("HOMESCREEN_HA_POLL_INTERVAL_S", "30"))


def _ha_bound_elements(design: Design) -> list[dict]:
    return [
        el
        for el in design.layout_json.get("elements", [])
        if el.get("type") == "value" and el.get("props", {}).get("source") == "home_assistant"
    ]


async def poll_once(db: Session, config: HomeAssistantConfig) -> None:
    for design in db.scalars(select(Design)).all():
        ha_elements = _ha_bound_elements(design)
        if not ha_elements:
            continue

        changed = False
        for element in ha_elements:
            element_id = element.get("id")
            entity_id = element.get("props", {}).get("entity_id")
            if element_id is None or not entity_id:
                continue
            attribute = element.get("props", {}).get("attribute")

            try:
                value = await fetch_entity_value(config.base_url, config.access_token, entity_id, attribute)
            except HomeAssistantError as exc:
                logger.warning("failed to fetch %s for design %s: %s", entity_id, design.id, exc)
                continue

            row = db.get(ElementLiveValue, (design.id, element_id))
            if row is None:
                db.add(ElementLiveValue(design_id=design.id, element_id=element_id, value=value))
                changed = True
            elif row.value != value:
                row.value = value
                changed = True

        if changed:
            db.commit()
            recompute_desired_hashes(db, design)
            logger.info("design %s's resolved content changed from Home Assistant", design.id)


async def run_poller() -> None:
    """Runs forever until cancelled. Never lets one bad cycle (a network blip, a
    misconfigured entity) kill the loop — logs and retries next interval, same
    resilience principle as the gateway's per-device sync loop."""
    while True:
        try:
            db = SessionLocal()
            try:
                config = db.get(HomeAssistantConfig, 1)
                if config and config.base_url and config.access_token:
                    await poll_once(db, config)
            finally:
                db.close()
        except Exception:
            logger.exception("unexpected error during Home Assistant poll cycle")
        await asyncio.sleep(POLL_INTERVAL_S)
