from datetime import datetime
from typing import Any, Literal

from pydantic import BaseModel, ConfigDict, Field


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


class DisplayRotationSet(BaseModel):
    rotate_180: bool


class DisplayWakeIntervalSet(BaseModel):
    # Bounds mirror firmware's BLE_SERVICE_MIN/MAX_WAKE_INTERVAL_S (ble_service.h) — a
    # value outside this range can't actually be applied on-device, so reject it here
    # rather than silently clamping.
    wake_interval_s: int = Field(ge=5, le=3600)


class DisplayFullRefreshIntervalSet(BaseModel):
    # None disables the recurring schedule. Lower bound (5 min) avoids a schedule that
    # would burn battery on a hardware flash-and-redraw for no visible benefit —
    # partial-refresh ghosting takes many cycles to become noticeable; upper bound is a
    # generous 30 days.
    full_refresh_interval_s: int | None = Field(default=None, ge=300, le=2592000)


class DisplayStatusReport(BaseModel):
    content_hash: int
    battery_pct: int
    battery_mv: int


class ButtonEventReport(BaseModel):
    # Bitmask read from the device's button_event GATT characteristic - bit i (0-4) set
    # means physical row button i was pressed since firmware last cleared the mask.
    button_mask: int


class DisplayOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    name: str
    mac_address: str
    gateway_id: int | None
    design_id: int | None
    width: int
    height: int
    rotate_180: bool
    wake_interval_s: int
    full_refresh_interval_s: int | None
    last_full_refresh_at: datetime | None
    current_content_hash: int | None
    desired_content_hash: int | None
    battery_pct: int | None
    battery_mv: int | None
    last_seen_at: datetime | None
    created_at: datetime
    in_sync: bool
    full_refresh_due: bool


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
