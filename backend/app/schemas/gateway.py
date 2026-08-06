from datetime import datetime

from pydantic import BaseModel, ConfigDict


class GatewayCreate(BaseModel):
    name: str
    location: str | None = None


class GatewayOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    name: str
    location: str | None
    last_checkin_at: datetime | None
    created_at: datetime
