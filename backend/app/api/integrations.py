from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session

from app.database import get_db
from app.models.db import HomeAssistantConfig
from app.schemas.integration import EntityPreviewOut, HomeAssistantConfigIn, HomeAssistantConfigOut
from app.services.home_assistant import HomeAssistantError, fetch_entity_value

router = APIRouter(prefix="/api/integrations/home-assistant", tags=["integrations"])


def _get_or_create_config(db: Session) -> HomeAssistantConfig:
    config = db.get(HomeAssistantConfig, 1)
    if config is None:
        config = HomeAssistantConfig(id=1)
        db.add(config)
        db.commit()
        db.refresh(config)
    return config


@router.get("", response_model=HomeAssistantConfigOut)
async def get_config(db: Session = Depends(get_db)):
    config = _get_or_create_config(db)
    return HomeAssistantConfigOut(
        base_url=config.base_url,
        token_set=bool(config.access_token),
        updated_at=config.updated_at,
    )


@router.put("", response_model=HomeAssistantConfigOut)
async def set_config(payload: HomeAssistantConfigIn, db: Session = Depends(get_db)):
    config = _get_or_create_config(db)
    config.base_url = payload.base_url
    if payload.access_token:  # blank means "keep the existing token"
        config.access_token = payload.access_token
    db.commit()
    db.refresh(config)
    return HomeAssistantConfigOut(
        base_url=config.base_url,
        token_set=bool(config.access_token),
        updated_at=config.updated_at,
    )


@router.get("/entities/{entity_id}", response_model=EntityPreviewOut)
async def preview_entity(entity_id: str, attribute: str | None = None, db: Session = Depends(get_db)):
    """Lets the design editor show a live value while picking an entity_id, without
    waiting for the poller's next cycle."""
    config = _get_or_create_config(db)
    if not config.base_url or not config.access_token:
        raise HTTPException(status_code=409, detail="Home Assistant is not configured yet")

    try:
        value = await fetch_entity_value(config.base_url, config.access_token, entity_id, attribute)
    except HomeAssistantError as exc:
        return EntityPreviewOut(entity_id=entity_id, error=str(exc))
    return EntityPreviewOut(entity_id=entity_id, value=value)
