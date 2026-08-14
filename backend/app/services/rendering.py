"""
Turns a design's layout_json into cacheable "rendered" content and decides whether a
display can be brought up to date with a value diff or needs a full re-render.

A design's layout_json is a *template*: "value" elements either carry a static value
directly, or a binding to an external source (currently: a Home Assistant entity — see
app/services/home_assistant.py). resolve_layout() merges the template with whatever the
latest fetched values are (ElementLiveValue rows) before anything gets hashed or cached,
so externally-sourced values flow through the exact same change-detection and diff
machinery as manually-typed ones.

Real ePaper bitmap generation is Section 7 build-order step 4 and doesn't exist yet;
`rendered_bitmap` here is a canonical JSON snapshot of the resolved layout, which is
enough to drive change detection and the diff/full decision end to end.
"""

import json
from copy import deepcopy

from sqlalchemy import select
from sqlalchemy.orm import Session

from app.models.db import ContentCache, Design, Display, ElementLiveValue
from app.services.hashing import canonical_json_bytes, crc32_of


def _structure_only(layout: dict) -> dict:
    stripped = deepcopy(layout)
    for element in stripped.get("elements", []):
        if element.get("type") == "value":
            props = element.get("props", {})
            props.pop("value", None)
            # "checked" is button elements' bound runtime state, same treatment as
            # "value" above - stripping it is what makes a checklist toggle diff-eligible
            # instead of forcing a full re-render every press.
            props.pop("checked", None)
    return stripped


def content_hash_for(layout: dict) -> int:
    return crc32_of(canonical_json_bytes(layout))


def structure_signature_for(layout: dict) -> int:
    return crc32_of(canonical_json_bytes(_structure_only(layout)))


def _format_value(raw: str | None, unit: str | None, precision, round_to=None) -> str | None:
    """Applies a home_assistant element's "unit", "precision", and "round_to" props to
    its raw fetched state - same display-time formatting HA's own entity "Display
    precision" setting does, plus an optional snap-to-step before that (e.g. round_to=0.5
    turns 21.34/21.26/20.98 into 21.5/21.0/21.0 instead of chasing every 0.1 wobble).
    round_to exists for entities whose *displayed* value should change less often than
    its raw precision would otherwise force - the content hash (and so a push) only
    changes when the rounded value does, see rendering's module docstring and
    ha_poller.py. Rounding only applies when the raw state parses as a number;
    non-numeric states (e.g. "on"/"off") pass through untouched but can still get a
    unit suffix appended."""
    if raw is None:
        return None
    text = raw
    if precision not in (None, "") or round_to not in (None, ""):
        try:
            num = float(raw)
            if round_to not in (None, ""):
                step = float(round_to)
                if step > 0:
                    num = round(num / step) * step
            text = f"{num:.{int(precision)}f}" if precision not in (None, "") else f"{num:g}"
        except ValueError:
            text = raw
    return f"{text} {unit}" if unit else text


def resolve_layout(layout: dict, live_values: dict[int, str]) -> dict:
    """Merge live_values (element_id -> latest fetched value) into a copy of the
    layout template. Static value elements pass through untouched; externally-sourced
    ones get their current value substituted in (formatted per the element's "unit",
    "precision", and "round_to" props), or None if nothing's been fetched yet. Button elements are
    similar but bind a runtime-only "checked" bool instead of replacing "value" - their
    label text is always static, only the checked state is live (see
    ElementLiveValue's "checked"/"unchecked" sentinel strings)."""
    resolved = deepcopy(layout)
    for element in resolved.get("elements", []):
        if element.get("type") != "value":
            continue
        props = element.setdefault("props", {})
        if props.get("source") == "home_assistant":
            raw = live_values.get(element.get("id"))
            props["value"] = _format_value(raw, props.get("unit"), props.get("precision"), props.get("round_to"))
        elif props.get("source") == "button":
            props["checked"] = live_values.get(element.get("id")) == "checked"
    return resolved


def live_values_for_design(db: Session, design_id: int) -> dict[int, str]:
    rows = db.scalars(select(ElementLiveValue).where(ElementLiveValue.design_id == design_id)).all()
    return {row.element_id: row.value for row in rows}


def render_and_cache(db: Session, design: Design) -> ContentCache:
    """Get-or-create the ContentCache row for the design's current resolved layout
    (template merged with any live externally-sourced values)."""
    resolved = resolve_layout(design.layout_json, live_values_for_design(db, design.id))
    content_hash = content_hash_for(resolved)
    existing = db.get(ContentCache, (design.id, content_hash))
    if existing is not None:
        return existing

    entry = ContentCache(
        design_id=design.id,
        content_hash=content_hash,
        rendered_bitmap=canonical_json_bytes(resolved),
        structure_signature=structure_signature_for(resolved),
    )
    db.add(entry)
    db.flush()
    return entry


def build_value_diff(current: ContentCache, desired: ContentCache) -> dict[int, str]:
    """Map of element_id -> new wire value for elements whose bound state changed.
    Button elements diff their "checked" bool (encoded as the literal strings
    "checked"/"unchecked", the same sentinels ElementLiveValue stores); every other
    value element diffs its "value" string as before.

    Only valid when current.structure_signature == desired.structure_signature —
    callers must check that before calling this.
    """
    current_layout = json.loads(current.rendered_bitmap)
    desired_layout = json.loads(desired.rendered_bitmap)

    current_elements = {
        el["id"]: el for el in current_layout.get("elements", []) if el.get("type") == "value"
    }
    diff: dict[int, str] = {}
    for el in desired_layout.get("elements", []):
        if el.get("type") != "value":
            continue
        element_id = el["id"]
        props = el.get("props", {})
        current_props = current_elements.get(element_id, {}).get("props", {})
        if props.get("source") == "button":
            new_checked = bool(props.get("checked", False))
            if bool(current_props.get("checked", False)) != new_checked:
                diff[element_id] = "checked" if new_checked else "unchecked"
        else:
            new_value = props.get("value")
            if current_props.get("value") != new_value:
                diff[element_id] = new_value
    return diff


def find_button_element(design: Design, button_index: int) -> dict | None:
    """Locates the design's "value" element bound to a given physical button index
    (0-4), if any - mirrors app.services.ha_poller._ha_bound_elements's lookup style."""
    for element in design.layout_json.get("elements", []):
        props = element.get("props", {})
        if (
            element.get("type") == "value"
            and props.get("source") == "button"
            and props.get("button_index") == button_index
        ):
            return element
    return None


def recompute_desired_hashes(db: Session, design: Design) -> ContentCache:
    """Recomputes the target render for a design and pushes the new desired_content_hash
    to every display currently assigned to it. Shared by the design-update endpoint and
    the Home Assistant poller — both need the exact same "something about this design's
    resolved output changed" handling."""
    cache_entry = render_and_cache(db, design)
    for display in db.scalars(select(Display).where(Display.design_id == design.id)):
        display.desired_content_hash = cache_entry.content_hash
    db.commit()
    return cache_entry
