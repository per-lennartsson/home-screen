from datetime import datetime, timezone

from sqlalchemy import ForeignKey, JSON, String
from sqlalchemy.orm import Mapped, mapped_column, relationship

from app.database import Base


def _utcnow() -> datetime:
    return datetime.now(timezone.utc)


class Gateway(Base):
    __tablename__ = "gateways"

    id: Mapped[int] = mapped_column(primary_key=True)
    name: Mapped[str] = mapped_column(String, unique=True)
    location: Mapped[str | None] = mapped_column(String, nullable=True)
    last_checkin_at: Mapped[datetime | None] = mapped_column(nullable=True)
    created_at: Mapped[datetime] = mapped_column(default=_utcnow)

    displays: Mapped[list["Display"]] = relationship(back_populates="gateway")


class Design(Base):
    __tablename__ = "designs"

    id: Mapped[int] = mapped_column(primary_key=True)
    name: Mapped[str] = mapped_column(String)
    # {"elements": [{"id": int, "type": "text"|"value"|"image", "x", "y", "w", "h", "props": {...}}]}
    # "value" elements' props: {"source": "static"|"home_assistant"|"button", "value": str
    # (when static or button - a button element's "value" is its static label text),
    # "entity_id": str, "attribute": str|None (when home_assistant), "button_index": int
    # 0-4 (when button - which physical device button toggles this row's checked state,
    # stored live in ElementLiveValue as "checked"/"unchecked", merged in by
    # rendering.resolve_layout() into a runtime-only "checked": bool prop).
    # Both "text" and "value" elements' props also carry "fontSize": int (px, at the
    # design's authored resolution, which for this project's only supported panel is
    # 1:1 with device pixels), plus the styling props "bold", "align", "underline" and
    # "strikethrough" — all of which reach the panel as of wire format 3.
    # fontSize must be one of the shared font ladder's sizes: those are the only ones the
    # device has a generated font for (firmware/src/fonts/hs_fonts.h). The design editor
    # only offers ladder values, and gateway/gateway/protocol.py::_font_id_for() snaps
    # anything else to the nearest when encoding, for designs saved before the ladder.
    layout_json: Mapped[dict] = mapped_column(JSON)
    # Canvas resolution this design was authored for. Defaults match the only panel this
    # system currently targets (Seeed XIAO + 4.2" 400x300 SSD1683, see firmware/README.md).
    width: Mapped[int] = mapped_column(default=400)
    height: Mapped[int] = mapped_column(default=300)
    updated_at: Mapped[datetime] = mapped_column(default=_utcnow, onupdate=_utcnow)
    created_at: Mapped[datetime] = mapped_column(default=_utcnow)

    displays: Mapped[list["Display"]] = relationship(back_populates="design")
    content_cache_entries: Mapped[list["ContentCache"]] = relationship(back_populates="design")
    live_values: Mapped[list["ElementLiveValue"]] = relationship(
        back_populates="design", cascade="all, delete-orphan"
    )


class Display(Base):
    __tablename__ = "displays"

    id: Mapped[int] = mapped_column(primary_key=True)
    name: Mapped[str] = mapped_column(String)
    mac_address: Mapped[str] = mapped_column(String, unique=True)
    gateway_id: Mapped[int | None] = mapped_column(ForeignKey("gateways.id"), nullable=True)
    design_id: Mapped[int | None] = mapped_column(ForeignKey("designs.id"), nullable=True)

    # Physical panel resolution — see Design.width/height for the same default.
    width: Mapped[int] = mapped_column(default=400)
    height: Mapped[int] = mapped_column(default=300)

    # Per-display mounting setting (e.g. an uneven enclosure bezel needs to sit at the
    # other edge) — a device-local rendering transform, deliberately independent of
    # Design/content_hash: it doesn't change what's on screen, only which way up.
    # Applied firmware-side (rasterizer.c); the gateway asserts it every sync
    # (gateway/gateway/sync.py) via the `command` characteristic.
    rotate_180: Mapped[bool] = mapped_column(default=False)

    # How long the display sleeps between DEEP_SLEEP wake cycles (main.c's
    # APP_WAKE_INTERVAL_S, now runtime-configurable). Same "assert every sync" model as
    # rotate_180 above — self-healing after a firmware reset, which starts back at
    # ble_service.h's BLE_SERVICE_DEFAULT_WAKE_INTERVAL_S until the gateway re-pushes
    # this value. Bounds (5-3600s) mirror HomeAssistantConfig.poll_interval_s and
    # firmware's BLE_SERVICE_MIN/MAX_WAKE_INTERVAL_S.
    wake_interval_s: Mapped[int] = mapped_column(default=15)

    # One-shot manual trigger for COMMAND_FORCE_FULL_REFRESH (epaper.h) — a hardware
    # anti-ghosting flash-and-redraw, independent of content_hash: the gateway asserts it
    # on the next sync regardless of whether the display is already in_sync, then clears
    # it via /full-refresh-ack (gateway/gateway/sync.py). Same "declarative flag, gateway
    # decides when to assert" model as rotate_180/wake_interval_s above.
    full_refresh_requested: Mapped[bool] = mapped_column(default=False)

    # Optional recurring schedule (seconds) so partial-refresh ghosting can't accumulate
    # indefinitely between manual presses. None disables it. See full_refresh_due below
    # for how this combines with last_full_refresh_at.
    full_refresh_interval_s: Mapped[int | None] = mapped_column(nullable=True)

    # Set by /full-refresh-ack once the gateway confirms COMMAND_FORCE_FULL_REFRESH was
    # written over BLE — both clears full_refresh_requested and restarts the recurring
    # schedule's clock.
    last_full_refresh_at: Mapped[datetime | None] = mapped_column(nullable=True)

    # uint32 CRC32 of the rendered content this display currently holds / should hold
    current_content_hash: Mapped[int | None] = mapped_column(nullable=True)
    desired_content_hash: Mapped[int | None] = mapped_column(nullable=True)

    battery_pct: Mapped[int | None] = mapped_column(nullable=True)
    battery_mv: Mapped[int | None] = mapped_column(nullable=True)
    last_seen_at: Mapped[datetime | None] = mapped_column(nullable=True)
    created_at: Mapped[datetime] = mapped_column(default=_utcnow)

    gateway: Mapped[Gateway | None] = relationship(back_populates="displays")
    design: Mapped[Design | None] = relationship(back_populates="displays")
    battery_readings: Mapped[list["BatteryReading"]] = relationship(
        back_populates="display", cascade="all, delete-orphan"
    )

    @property
    def in_sync(self) -> bool:
        return self.current_content_hash == self.desired_content_hash

    @property
    def full_refresh_due(self) -> bool:
        """Whether the gateway should assert COMMAND_FORCE_FULL_REFRESH on the next
        sync — either a pending manual request, or the recurring schedule's interval has
        elapsed since the last confirmed refresh (or none has ever happened)."""
        if self.full_refresh_requested:
            return True
        if self.full_refresh_interval_s is None:
            return False
        if self.last_full_refresh_at is None:
            return True
        # SQLAlchemy round-trips DateTime columns through SQLite as naive values (see
        # frontend/src/lib/format.js's parseUtc comment for the same gotcha) even though
        # they were stored from an aware UTC datetime — so this can't just use _utcnow()
        # directly, or comparing aware-vs-naive raises TypeError.
        now = datetime.now(timezone.utc).replace(tzinfo=None)
        elapsed = (now - self.last_full_refresh_at).total_seconds()
        return elapsed >= self.full_refresh_interval_s


class BatteryReading(Base):
    """Historical log of battery_pct/battery_mv, one row per /status report (see
    api/displays.py's report_status) — Display.battery_pct/battery_mv only ever hold the
    latest value, this table is what battery history/estimate
    (app/services/battery.py) reads from. wake_interval_s is captured per-reading
    (rather than joined off Display at read time) because drain rate depends heavily on
    it and the display's current setting can change after a reading was logged."""

    __tablename__ = "battery_readings"

    id: Mapped[int] = mapped_column(primary_key=True)
    display_id: Mapped[int] = mapped_column(ForeignKey("displays.id"), index=True)
    battery_pct: Mapped[int] = mapped_column()
    battery_mv: Mapped[int] = mapped_column()
    wake_interval_s: Mapped[int] = mapped_column()
    recorded_at: Mapped[datetime] = mapped_column(default=_utcnow, index=True)

    # Whether this wake's connection went on to push a content payload (diff or full —
    # see api/displays.py's report_status, computed from Display.in_sync right after
    # current_content_hash is updated) rather than just read status and disconnect. The
    # push (BLE transfer + e-paper refresh) happens *after* this reading's battery_mv was
    # sampled, so its extra cost shows up in the *next* reading, not this one — battery.py
    # uses this flag to tell "bare check-in" drain apart from "check-in + push" drain
    # instead of averaging them into one undifferentiated per-wake cost.
    pushed_payload: Mapped[bool] = mapped_column(default=False)

    # Real signal from the charger IC's status pin (firmware/src/battery.c, appended to
    # the BLE status struct as of fw_version 2 — see docs/protocol.md), not inferred.
    # NULL means "unknown" (either this reading predates the schema column, or the
    # display was still running pre-charge-status firmware at the time) — deliberately
    # distinct from False ("device confirmed not charging"), since battery.py's
    # charging_flags() only trusts this when it isn't NULL and falls back to its own
    # voltage-trend heuristic otherwise.
    reported_charging: Mapped[bool | None] = mapped_column(nullable=True, default=None)

    display: Mapped["Display"] = relationship(back_populates="battery_readings")


class ContentCache(Base):
    """Cache of rendered output per design version, keyed by (design_id, content_hash)."""

    __tablename__ = "content_cache"

    design_id: Mapped[int] = mapped_column(ForeignKey("designs.id"), primary_key=True)
    content_hash: Mapped[int] = mapped_column(primary_key=True)

    # Placeholder for the real ePaper bitmap (Section 7 step 4). For now this stores the
    # canonical JSON snapshot of the layout that produced this hash, so payload/diff
    # endpoints have something concrete to serve against.
    rendered_bitmap: Mapped[bytes] = mapped_column()
    # CRC32 of the layout with all bound values stripped out — two renders that share this
    # signature differ only in bound values, which is what makes a diff payload valid.
    structure_signature: Mapped[int] = mapped_column()
    rendered_at: Mapped[datetime] = mapped_column(default=_utcnow)

    design: Mapped[Design] = relationship(back_populates="content_cache_entries")


class ElementLiveValue(Base):
    """Last-fetched value for a design's externally-sourced (e.g. Home Assistant)
    value element. Kept separate from Design.layout_json — the layout is the binding
    *template* (which entity to watch), this is the live data the poller writes.
    render_and_cache resolves the two together before hashing (see rendering.py)."""

    __tablename__ = "element_live_values"

    design_id: Mapped[int] = mapped_column(ForeignKey("designs.id"), primary_key=True)
    element_id: Mapped[int] = mapped_column(primary_key=True)
    value: Mapped[str] = mapped_column(String)
    updated_at: Mapped[datetime] = mapped_column(default=_utcnow, onupdate=_utcnow)

    design: Mapped[Design] = relationship(back_populates="live_values")


class HomeAssistantConfig(Base):
    """Singleton row (id is always 1) holding the one Home Assistant instance this
    system polls — matches v1's "one location" scope (spec 1)."""

    __tablename__ = "home_assistant_config"

    id: Mapped[int] = mapped_column(primary_key=True, default=1)
    base_url: Mapped[str | None] = mapped_column(String, nullable=True)
    access_token: Mapped[str | None] = mapped_column(String, nullable=True)
    # Seconds between poll cycles (see app/services/ha_poller.py). Column default matches
    # HOMESCREEN_HA_POLL_INTERVAL_S's env-var default so a freshly created config row
    # behaves the same as before this became UI-configurable.
    poll_interval_s: Mapped[int] = mapped_column(default=30)
    updated_at: Mapped[datetime] = mapped_column(default=_utcnow, onupdate=_utcnow)
