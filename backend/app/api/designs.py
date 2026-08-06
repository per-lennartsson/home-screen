from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import select
from sqlalchemy.orm import Session

from app.database import get_db
from app.models.db import Design, Display
from app.schemas.design import DesignCreate, DesignOut, DesignUpdate
from app.services.rendering import render_and_cache

router = APIRouter(prefix="/api/designs", tags=["designs"])


@router.post("", response_model=DesignOut)
def create_design(payload: DesignCreate, db: Session = Depends(get_db)):
    design = Design(name=payload.name, layout_json=payload.layout_json)
    db.add(design)
    db.commit()
    db.refresh(design)
    render_and_cache(db, design)
    db.commit()
    return design


@router.get("", response_model=list[DesignOut])
def list_designs(db: Session = Depends(get_db)):
    return db.scalars(select(Design)).all()


@router.get("/{design_id}", response_model=DesignOut)
def get_design(design_id: int, db: Session = Depends(get_db)):
    design = db.get(Design, design_id)
    if design is None:
        raise HTTPException(status_code=404, detail="design not found")
    return design


@router.put("/{design_id}", response_model=DesignOut)
def update_design(design_id: int, payload: DesignUpdate, db: Session = Depends(get_db)):
    design = db.get(Design, design_id)
    if design is None:
        raise HTTPException(status_code=404, detail="design not found")

    if payload.name is not None:
        design.name = payload.name
    if payload.layout_json is not None:
        design.layout_json = payload.layout_json
    db.commit()
    db.refresh(design)

    # Recompute the target render + hash for every display currently on this design.
    cache_entry = render_and_cache(db, design)
    for display in db.scalars(select(Display).where(Display.design_id == design_id)):
        display.desired_content_hash = cache_entry.content_hash
    db.commit()
    return design
