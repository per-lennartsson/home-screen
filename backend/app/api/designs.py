from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import select
from sqlalchemy.orm import Session

from app.database import get_db
from app.models.db import ContentCache, Design, Display
from app.schemas.design import DesignCreate, DesignOut, DesignUpdate
from app.services.rendering import recompute_desired_hashes

router = APIRouter(prefix="/api/designs", tags=["designs"])


@router.post("", response_model=DesignOut)
async def create_design(payload: DesignCreate, db: Session = Depends(get_db)):
    design = Design(
        name=payload.name,
        layout_json=payload.layout_json,
        width=payload.width,
        height=payload.height,
    )
    db.add(design)
    db.commit()
    db.refresh(design)
    recompute_desired_hashes(db, design)
    db.refresh(design)
    return design


@router.get("", response_model=list[DesignOut])
async def list_designs(db: Session = Depends(get_db)):
    return db.scalars(select(Design)).all()


@router.get("/{design_id}", response_model=DesignOut)
async def get_design(design_id: int, db: Session = Depends(get_db)):
    design = db.get(Design, design_id)
    if design is None:
        raise HTTPException(status_code=404, detail="design not found")
    return design


@router.put("/{design_id}", response_model=DesignOut)
async def update_design(design_id: int, payload: DesignUpdate, db: Session = Depends(get_db)):
    design = db.get(Design, design_id)
    if design is None:
        raise HTTPException(status_code=404, detail="design not found")

    if payload.name is not None:
        design.name = payload.name
    if payload.layout_json is not None:
        design.layout_json = payload.layout_json
    if payload.width is not None:
        design.width = payload.width
    if payload.height is not None:
        design.height = payload.height
    db.commit()
    db.refresh(design)

    # Recompute the target render + hash for every display currently on this design.
    recompute_desired_hashes(db, design)
    db.refresh(design)
    return design


@router.delete("/{design_id}", status_code=204)
async def delete_design(design_id: int, db: Session = Depends(get_db)):
    design = db.get(Design, design_id)
    if design is None:
        raise HTTPException(status_code=404, detail="design not found")
    if db.scalar(select(Display).where(Display.design_id == design_id)):
        raise HTTPException(status_code=409, detail="design has assigned displays; unassign them first")

    for cache_entry in db.scalars(select(ContentCache).where(ContentCache.design_id == design_id)):
        db.delete(cache_entry)
    db.delete(design)  # cascades ElementLiveValue rows via the model's relationship
    db.commit()
