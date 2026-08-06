from datetime import datetime
from typing import Any

from pydantic import BaseModel, ConfigDict


class DesignCreate(BaseModel):
    name: str
    layout_json: dict[str, Any]


class DesignUpdate(BaseModel):
    name: str | None = None
    layout_json: dict[str, Any] | None = None


class DesignOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    name: str
    layout_json: dict[str, Any]
    updated_at: datetime
    created_at: datetime
