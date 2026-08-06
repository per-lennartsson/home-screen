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
    # "value" elements' props: {"source": "static"|"home_assistant", "value": str (when
    # static), "entity_id": str, "attribute": str|None (when home_assistant)}.
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

    # uint32 CRC32 of the rendered content this display currently holds / should hold
    current_content_hash: Mapped[int | None] = mapped_column(nullable=True)
    desired_content_hash: Mapped[int | None] = mapped_column(nullable=True)

    battery_pct: Mapped[int | None] = mapped_column(nullable=True)
    battery_mv: Mapped[int | None] = mapped_column(nullable=True)
    last_seen_at: Mapped[datetime | None] = mapped_column(nullable=True)
    created_at: Mapped[datetime] = mapped_column(default=_utcnow)

    gateway: Mapped[Gateway | None] = relationship(back_populates="displays")
    design: Mapped[Design | None] = relationship(back_populates="displays")

    @property
    def in_sync(self) -> bool:
        return self.current_content_hash == self.desired_content_hash


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
    updated_at: Mapped[datetime] = mapped_column(default=_utcnow, onupdate=_utcnow)
