from datetime import datetime

from pydantic import BaseModel


class HomeAssistantConfigIn(BaseModel):
    base_url: str
    # Optional/blank means "keep the existing token" — lets the GUI show a save form
    # without ever having to redisplay a previously-set secret (see api/integrations.py).
    access_token: str | None = None


class HomeAssistantConfigOut(BaseModel):
    base_url: str | None
    token_set: bool
    updated_at: datetime | None


class EntityPreviewOut(BaseModel):
    entity_id: str
    value: str | None = None
    error: str | None = None
