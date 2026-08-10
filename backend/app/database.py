import os
from pathlib import Path

from sqlalchemy import create_engine
from sqlalchemy.orm import DeclarativeBase, sessionmaker

DATA_DIR = Path(os.environ.get("HOMESCREEN_DATA_DIR", Path(__file__).resolve().parent.parent / "data"))
DATA_DIR.mkdir(parents=True, exist_ok=True)
DATABASE_URL = f"sqlite:///{DATA_DIR / 'homescreen.db'}"

engine = create_engine(DATABASE_URL, connect_args={"check_same_thread": False})
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)


class Base(DeclarativeBase):
    pass


def ensure_schema_additions() -> None:
    """create_all() only creates missing tables, not missing columns on tables that
    already exist. This project has no migration framework yet (SQLite, v1, single
    dev deployment) — patch any columns added to an existing model here so a pre-existing
    homescreen.db doesn't fail with "no such column" after a code update."""
    with engine.begin() as conn:
        cols = {row[1] for row in conn.exec_driver_sql("PRAGMA table_info(home_assistant_config)")}
        if cols and "poll_interval_s" not in cols:
            conn.exec_driver_sql(
                "ALTER TABLE home_assistant_config ADD COLUMN poll_interval_s INTEGER NOT NULL DEFAULT 30"
            )

        display_cols = {row[1] for row in conn.exec_driver_sql("PRAGMA table_info(displays)")}
        if display_cols and "rotate_180" not in display_cols:
            conn.exec_driver_sql(
                "ALTER TABLE displays ADD COLUMN rotate_180 BOOLEAN NOT NULL DEFAULT 0"
            )


async def get_db():
    # async def, not def: FastAPI runs sync generator dependencies through anyio's
    # threadpool, which failed to spawn threads in some Docker environments (see
    # README/compose comments). SQLite is a fast local file, so running the
    # synchronous SQLAlchemy session directly on the event loop — no thread needed —
    # is a reasonable simplification at this app's scale.
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
