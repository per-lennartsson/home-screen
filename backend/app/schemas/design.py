from datetime import datetime
from typing import Any

from pydantic import BaseModel, ConfigDict


class DesignCreate(BaseModel):
    name: str
    layout_json: dict[str, Any]
    width: int = 400
    height: int = 300


class DesignUpdate(BaseModel):
    name: str | None = None
    layout_json: dict[str, Any] | None = None
    width: int | None = None
    height: int | None = None


class DesignOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    name: str
    layout_json: dict[str, Any]
    width: int
    height: int
    updated_at: datetime
    created_at: datetime
