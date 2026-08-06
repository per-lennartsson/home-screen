"""
Turns a design's layout_json into cacheable "rendered" content and decides whether a
display can be brought up to date with a value diff or needs a full re-render.

A design's layout_json is a *template*: "value" elements either carry a static value
directly, or a binding to an external source (currently: a Home Assistant entity — see
app/services/home_assistant.py). resolve_layout() merges the template with whatever the
latest fetched values are (ElementLiveValue rows) before anything gets hashed or cached,
so externally-sourced values flow through the exact same change-detection and diff
machinery as manually-typed ones.

Real ePaper bitmap generation is Section 7 build-order step 4 and doesn't exist yet;
`rendered_bitmap` here is a canonical JSON snapshot of the resolved layout, which is
enough to drive change detection and the diff/full decision end to end.
"""

import json
from copy import deepcopy

from sqlalchemy import select
from sqlalchemy.orm import Session

from app.models.db import ContentCache, Design, Display, ElementLiveValue
from app.services.hashing import canonical_json_bytes, crc32_of


def _structure_only(layout: dict) -> dict:
    stripped = deepcopy(layout)
    for element in stripped.get("elements", []):
        if element.get("type") == "value":
            element.get("props", {}).pop("value", None)
    return stripped


def content_hash_for(layout: dict) -> int:
    return crc32_of(canonical_json_bytes(layout))


def structure_signature_for(layout: dict) -> int:
    return crc32_of(canonical_json_bytes(_structure_only(layout)))


def resolve_layout(layout: dict, live_values: dict[int, str]) -> dict:
    """Merge live_values (element_id -> latest fetched value) into a copy of the
    layout template. Static value elements pass through untouched; externally-sourced
    ones get their current value substituted in, or None if nothing's been fetched yet."""
    resolved = deepcopy(layout)
    for element in resolved.get("elements", []):
        if element.get("type") != "value":
            continue
        props = element.setdefault("props", {})
        if props.get("source") == "home_assistant":
            props["value"] = live_values.get(element.get("id"))
    return resolved


def live_values_for_design(db: Session, design_id: int) -> dict[int, str]:
    rows = db.scalars(select(ElementLiveValue).where(ElementLiveValue.design_id == design_id)).all()
    return {row.element_id: row.value for row in rows}


def render_and_cache(db: Session, design: Design) -> ContentCache:
    """Get-or-create the ContentCache row for the design's current resolved layout
    (template merged with any live externally-sourced values)."""
    resolved = resolve_layout(design.layout_json, live_values_for_design(db, design.id))
    content_hash = content_hash_for(resolved)
    existing = db.get(ContentCache, (design.id, content_hash))
    if existing is not None:
        return existing

    entry = ContentCache(
        design_id=design.id,
        content_hash=content_hash,
        rendered_bitmap=canonical_json_bytes(resolved),
        structure_signature=structure_signature_for(resolved),
    )
    db.add(entry)
    db.flush()
    return entry


def build_value_diff(current: ContentCache, desired: ContentCache) -> dict[int, str]:
    """Map of element_id -> new value for elements whose bound value changed.

    Only valid when current.structure_signature == desired.structure_signature —
    callers must check that before calling this.
    """
    current_layout = json.loads(current.rendered_bitmap)
    desired_layout = json.loads(desired.rendered_bitmap)

    current_values = {
        el["id"]: el.get("props", {}).get("value")
        for el in current_layout.get("elements", [])
        if el.get("type") == "value"
    }
    diff: dict[int, str] = {}
    for el in desired_layout.get("elements", []):
        if el.get("type") != "value":
            continue
        new_value = el.get("props", {}).get("value")
        if current_values.get(el["id"]) != new_value:
            diff[el["id"]] = new_value
    return diff


def recompute_desired_hashes(db: Session, design: Design) -> ContentCache:
    """Recomputes the target render for a design and pushes the new desired_content_hash
    to every display currently assigned to it. Shared by the design-update endpoint and
    the Home Assistant poller — both need the exact same "something about this design's
    resolved output changed" handling."""
    cache_entry = render_and_cache(db, design)
    for display in db.scalars(select(Display).where(Display.design_id == design.id)):
        display.desired_content_hash = cache_entry.content_hash
    db.commit()
    return cache_entry
