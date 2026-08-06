from datetime import datetime
from typing import Any, Literal

from pydantic import BaseModel, ConfigDict


class DisplayCreate(BaseModel):
    name: str
    mac_address: str
    gateway_id: int | None = None
    width: int = 400
    height: int = 300


class DisplayAssign(BaseModel):
    design_id: int


class DisplayGatewayAssign(BaseModel):
    gateway_id: int


class DisplayStatusReport(BaseModel):
    content_hash: int
    battery_pct: int
    battery_mv: int


class DisplayOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    name: str
    mac_address: str
    gateway_id: int | None
    design_id: int | None
    width: int
    height: int
    current_content_hash: int | None
    desired_content_hash: int | None
    battery_pct: int | None
    battery_mv: int | None
    last_seen_at: datetime | None
    created_at: datetime
    in_sync: bool


class PayloadInSync(BaseModel):
    in_sync: Literal[True] = True


class PayloadFull(BaseModel):
    type: Literal["full"] = "full"
    content_hash: int
    data: dict[str, Any]


class PayloadDiff(BaseModel):
    type: Literal["diff"] = "diff"
    content_hash: int
    # data.values maps element_id (as string, since it travels over JSON) -> new value.
    # On the wire to firmware this becomes the TLV-encoded 0x02 diff chunk payload
    # (see docs/protocol.md) — the gateway is responsible for that translation.
    data: dict[str, dict[str, str]]
