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
