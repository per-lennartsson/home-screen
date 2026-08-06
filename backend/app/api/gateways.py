from datetime import datetime, timezone

from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import select
from sqlalchemy.orm import Session

from app.database import get_db
from app.models.db import Display, Gateway
from app.schemas.display import DisplayOut
from app.schemas.gateway import GatewayCreate, GatewayOut

router = APIRouter(prefix="/api/gateways", tags=["gateways"])


@router.post("", response_model=GatewayOut)
async def create_gateway(payload: GatewayCreate, db: Session = Depends(get_db)):
    if db.scalar(select(Gateway).where(Gateway.name == payload.name)):
        raise HTTPException(status_code=409, detail="gateway name already registered")
    gateway = Gateway(name=payload.name, location=payload.location)
    db.add(gateway)
    db.commit()
    db.refresh(gateway)
    return gateway


@router.get("", response_model=list[GatewayOut])
async def list_gateways(db: Session = Depends(get_db)):
    return db.scalars(select(Gateway)).all()


@router.get("/{gateway_id}/assigned-displays", response_model=list[DisplayOut])
async def assigned_displays(gateway_id: int, db: Session = Depends(get_db)):
    gateway = db.get(Gateway, gateway_id)
    if gateway is None:
        raise HTTPException(status_code=404, detail="gateway not found")

    gateway.last_checkin_at = datetime.now(timezone.utc)
    db.commit()

    displays = db.scalars(select(Display).where(Display.gateway_id == gateway_id)).all()
    return displays
