from datetime import datetime

from pydantic import BaseModel, Field


class HomeAssistantConfigIn(BaseModel):
    base_url: str
    # Optional/blank means "keep the existing token" — lets the GUI show a save form
    # without ever having to redisplay a previously-set secret (see api/integrations.py).
    access_token: str | None = None
    # None means "keep the existing interval" (same convention as access_token above).
    poll_interval_s: int | None = Field(default=None, ge=5, le=3600)


class HomeAssistantConfigOut(BaseModel):
    base_url: str | None
    token_set: bool
    poll_interval_s: int
    updated_at: datetime | None


class EntityPreviewOut(BaseModel):
    entity_id: str
    value: str | None = None
    error: str | None = None
