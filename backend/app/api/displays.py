from datetime import datetime, timezone

from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy import select
from sqlalchemy.orm import Session

from app.database import get_db
from app.models.db import BatteryReading, ContentCache, Design, Display, ElementLiveValue, Gateway
from app.schemas.display import (
    BatteryEstimateOut,
    BatteryReadingOut,
    ButtonEventReport,
    DisplayAssign,
    DisplayCreate,
    DisplayFullRefreshIntervalSet,
    DisplayGatewayAssign,
    DisplayOut,
    DisplayRotationSet,
    DisplayStatusReport,
    DisplayWakeIntervalSet,
    PayloadDiff,
    PayloadFull,
    PayloadInSync,
)
from app.services.battery import estimate_remaining
from app.services.rendering import build_value_diff, find_button_element, recompute_desired_hashes

router = APIRouter(prefix="/api/displays", tags=["displays"])


@router.post("", response_model=DisplayOut)
async def create_display(payload: DisplayCreate, db: Session = Depends(get_db)):
    if db.scalar(select(Display).where(Display.mac_address == payload.mac_address)):
        raise HTTPException(status_code=409, detail="mac_address already registered")
    if payload.gateway_id is not None and db.get(Gateway, payload.gateway_id) is None:
        raise HTTPException(status_code=404, detail="gateway not found")

    display = Display(
        name=payload.name,
        mac_address=payload.mac_address,
        gateway_id=payload.gateway_id,
        width=payload.width,
        height=payload.height,
    )
    db.add(display)
    db.commit()
    db.refresh(display)
    return display


@router.get("", response_model=list[DisplayOut])
async def list_displays(db: Session = Depends(get_db)):
    return db.scalars(select(Display)).all()


@router.get("/{display_id}", response_model=DisplayOut)
async def get_display(display_id: int, db: Session = Depends(get_db)):
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")
    return display


@router.delete("/{display_id}", status_code=204)
async def delete_display(display_id: int, db: Session = Depends(get_db)):
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")

    db.delete(display)
    db.commit()


@router.post("/{display_id}/assign", response_model=DisplayOut)
async def assign_design(display_id: int, payload: DisplayAssign, db: Session = Depends(get_db)):
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")
    design = db.get(Design, payload.design_id)
    if design is None:
        raise HTTPException(status_code=404, detail="design not found")
    if design.width != display.width or design.height != display.height:
        raise HTTPException(
            status_code=409,
            detail=(
                f"resolution mismatch: design is {design.width}x{design.height}, "
                f"display is {display.width}x{display.height}"
            ),
        )

    display.design_id = design.id
    db.commit()
    recompute_desired_hashes(db, design)
    db.refresh(display)
    return display


@router.post("/{display_id}/assign-gateway", response_model=DisplayOut)
async def assign_gateway(display_id: int, payload: DisplayGatewayAssign, db: Session = Depends(get_db)):
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")
    if db.get(Gateway, payload.gateway_id) is None:
        raise HTTPException(status_code=404, detail="gateway not found")

    display.gateway_id = payload.gateway_id
    db.commit()
    db.refresh(display)
    return display


@router.post("/{display_id}/rotate", response_model=DisplayOut)
async def set_rotation(display_id: int, payload: DisplayRotationSet, db: Session = Depends(get_db)):
    """Sets a display's physical mounting orientation. Purely a device-local rendering
    transform (see Display.rotate_180) — doesn't touch content_hash/design, so the
    gateway just re-asserts it on the next sync (gateway/gateway/sync.py) rather than
    this endpoint needing to push anything itself."""
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")

    display.rotate_180 = payload.rotate_180
    db.commit()
    db.refresh(display)
    return display


@router.post("/{display_id}/wake-interval", response_model=DisplayOut)
async def set_wake_interval(display_id: int, payload: DisplayWakeIntervalSet, db: Session = Depends(get_db)):
    """Sets how often the display wakes to check in and poll for new content
    (firmware's APP_WAKE_INTERVAL_S, see main.c). Like rotate_180, this is asserted on
    every gateway sync (gateway/gateway/sync.py) rather than pushed once, so it's
    self-healing after a firmware reset."""
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")

    display.wake_interval_s = payload.wake_interval_s
    db.commit()
    db.refresh(display)
    return display


@router.post("/{display_id}/force-full-refresh", response_model=DisplayOut)
async def force_full_refresh(display_id: int, db: Session = Depends(get_db)):
    """Manual "fix ghosting now" trigger. Flags a hardware anti-ghosting flash-and-
    redraw (COMMAND_FORCE_FULL_REFRESH) for the gateway to assert on the next sync
    (gateway/gateway/sync.py) regardless of whether the display is already in_sync —
    this isn't a content change, so desired_content_hash is untouched. Cleared by
    /full-refresh-ack once the gateway confirms it was written over BLE."""
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")

    display.full_refresh_requested = True
    db.commit()
    db.refresh(display)
    return display


@router.post("/{display_id}/full-refresh-interval", response_model=DisplayOut)
async def set_full_refresh_interval(
    display_id: int, payload: DisplayFullRefreshIntervalSet, db: Session = Depends(get_db)
):
    """Sets (or clears, via null) the recurring full-refresh schedule. Purely
    declarative here, like wake_interval_s — Display.full_refresh_due does the actual
    "is it time yet" math and the gateway asserts it on sync."""
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")

    display.full_refresh_interval_s = payload.full_refresh_interval_s
    db.commit()
    db.refresh(display)
    return display


@router.post("/{display_id}/full-refresh-ack", response_model=DisplayOut)
async def ack_full_refresh(display_id: int, db: Session = Depends(get_db)):
    """Gateway calls this right after successfully writing
    COMMAND_FORCE_FULL_REFRESH over BLE (sync.py) — clears the one-shot manual request
    and restarts the recurring schedule's clock."""
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")

    display.full_refresh_requested = False
    display.last_full_refresh_at = datetime.now(timezone.utc)
    db.commit()
    db.refresh(display)
    return display


@router.post("/{display_id}/status", response_model=DisplayOut)
async def report_status(display_id: int, payload: DisplayStatusReport, db: Session = Depends(get_db)):
    """Gateway calls this after reading the `status` GATT characteristic (Section 5.1 step 4)."""
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")

    display.current_content_hash = payload.content_hash
    display.battery_pct = payload.battery_pct
    display.battery_mv = payload.battery_mv
    display.last_seen_at = datetime.now(timezone.utc)
    db.add(
        BatteryReading(
            display_id=display.id,
            battery_pct=payload.battery_pct,
            battery_mv=payload.battery_mv,
            wake_interval_s=display.wake_interval_s,
        )
    )
    db.commit()
    db.refresh(display)
    return display


@router.get("/{display_id}/battery-history", response_model=list[BatteryReadingOut])
async def get_battery_history(display_id: int, db: Session = Depends(get_db)):
    """Full logged battery history (BatteryReading, one row per /status report) for
    charting, oldest first."""
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")

    return db.scalars(
        select(BatteryReading)
        .where(BatteryReading.display_id == display_id)
        .order_by(BatteryReading.recorded_at)
    ).all()


@router.get("/{display_id}/battery-estimate", response_model=BatteryEstimateOut)
async def get_battery_estimate(
    display_id: int,
    wake_interval_s: int | None = Query(default=None, ge=5, le=3600),
    db: Session = Depends(get_db),
):
    """Estimated remaining battery life at `wake_interval_s` (defaults to the display's
    current setting). If that exact interval hasn't been run long enough to measure its
    own drain rate, but two or more other intervals have, this projects one instead of
    refusing — see app/services/battery.py for the model behind "what if I switched to a
    15-minute interval" without ever having run at 15 minutes."""
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")

    return estimate_remaining(db, display, wake_interval_s=wake_interval_s)


@router.post("/{display_id}/button-event", response_model=DisplayOut)
async def report_button_event(display_id: int, payload: ButtonEventReport, db: Session = Depends(get_db)):
    """Gateway calls this right after reading the device's button_event characteristic
    (Section 5.1 step 4, extended) - deliberately before GET /payload in the gateway's
    per-connection sequence, so the same connection's payload already reflects the
    toggle instead of waiting a full extra wake cycle."""
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")
    if display.design_id is None:
        raise HTTPException(status_code=409, detail="display has no design assigned")
    design = db.get(Design, display.design_id)

    changed = False
    for bit in range(5):
        if not (payload.button_mask & (1 << bit)):
            continue
        element = find_button_element(design, bit)
        if element is None:
            continue  # no row on this design bound to this physical button - ignore
        element_id = element["id"]
        row = db.get(ElementLiveValue, (design.id, element_id))
        currently_checked = row is not None and row.value == "checked"
        new_value = "unchecked" if currently_checked else "checked"
        if row is None:
            db.add(ElementLiveValue(design_id=design.id, element_id=element_id, value=new_value))
        else:
            row.value = new_value
        changed = True

    if changed:
        db.commit()
        recompute_desired_hashes(db, design)
    db.refresh(display)
    return display


@router.get("/{display_id}/payload", response_model=PayloadInSync | PayloadFull | PayloadDiff)
async def get_payload(display_id: int, db: Session = Depends(get_db)):
    """Gateway calls this right after POSTing status (Section 5.1 step 5)."""
    display = db.get(Display, display_id)
    if display is None:
        raise HTTPException(status_code=404, detail="display not found")

    if display.desired_content_hash is None or display.current_content_hash == display.desired_content_hash:
        return PayloadInSync()

    if display.design_id is None:
        raise HTTPException(status_code=409, detail="display has a desired hash but no design assigned")

    desired_cache = db.get(ContentCache, (display.design_id, display.desired_content_hash))
    if desired_cache is None:
        raise HTTPException(status_code=500, detail="desired content_hash has no cached render")

    current_cache = None
    if display.current_content_hash is not None:
        current_cache = db.get(ContentCache, (display.design_id, display.current_content_hash))

    diff_eligible = (
        current_cache is not None
        and current_cache.structure_signature == desired_cache.structure_signature
    )
    if diff_eligible:
        values = build_value_diff(current_cache, desired_cache)
        if values:
            return PayloadDiff(content_hash=desired_cache.content_hash, data={"values": {str(k): v for k, v in values.items()}})

    import json

    return PayloadFull(
        content_hash=desired_cache.content_hash,
        data=json.loads(desired_cache.rendered_bitmap),
    )
