"""
Turns a design's layout_json into cacheable "rendered" content and decides whether a
display can be brought up to date with a value diff or needs a full re-render.

Real ePaper bitmap generation is Section 7 build-order step 4 and doesn't exist yet;
`rendered_bitmap` here is a canonical JSON snapshot of the layout, which is enough to
drive change detection and the diff/full decision end to end.
"""

from copy import deepcopy

from sqlalchemy.orm import Session

from app.models.db import ContentCache, Design
from app.services.hashing import canonical_json_bytes, crc32_of


def _structure_only(layout: dict) -> dict:
    stripped = deepcopy(layout)
    for element in stripped.get("elements", []):
        if element.get("type") == "value":
            element.get("props", {}).pop("value", None)
    return stripped


def content_hash_for(layout: dict) -> int:
    return crc32_of(canonical_json_bytes(layout))


def structure_signature_for(layout: dict) -> int:
    return crc32_of(canonical_json_bytes(_structure_only(layout)))


def render_and_cache(db: Session, design: Design) -> ContentCache:
    """Get-or-create the ContentCache row for the design's current layout_json."""
    content_hash = content_hash_for(design.layout_json)
    existing = db.get(ContentCache, (design.id, content_hash))
    if existing is not None:
        return existing

    entry = ContentCache(
        design_id=design.id,
        content_hash=content_hash,
        rendered_bitmap=canonical_json_bytes(design.layout_json),
        structure_signature=structure_signature_for(design.layout_json),
    )
    db.add(entry)
    db.flush()
    return entry


def build_value_diff(current: ContentCache, desired: ContentCache) -> dict[int, str]:
    """Map of element_id -> new value for elements whose bound value changed.

    Only valid when current.structure_signature == desired.structure_signature —
    callers must check that before calling this.
    """
    import json

    current_layout = json.loads(current.rendered_bitmap)
    desired_layout = json.loads(desired.rendered_bitmap)

    current_values = {
        el["id"]: el.get("props", {}).get("value")
        for el in current_layout.get("elements", [])
        if el.get("type") == "value"
    }
    diff: dict[int, str] = {}
    for el in desired_layout.get("elements", []):
        if el.get("type") != "value":
            continue
        new_value = el.get("props", {}).get("value")
        if current_values.get(el["id"]) != new_value:
            diff[el["id"]] = new_value
    return diff
