from datetime import datetime, timezone

from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import select
from sqlalchemy.orm import Session

from app.database import get_db
from app.models.db import ContentCache, Design, Display, Gateway
from app.schemas.display import (
    DisplayAssign,
    DisplayCreate,
    DisplayGatewayAssign,
    DisplayOut,
    DisplayStatusReport,
    PayloadDiff,
    PayloadFull,
    PayloadInSync,
)
from app.services.rendering import build_value_diff, recompute_desired_hashes

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
    db.commit()
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
