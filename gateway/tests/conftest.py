"""
Runs the real backend app in-process (via httpx.ASGITransport) so gateway integration
tests exercise the actual API contract, not a hand-rolled fake. This only reaches across
into backend/ for testing convenience — the gateway's real runtime path always talks to
the backend over HTTP, never by importing it (spec 1: keep them as separate processes).
"""

import sys
from pathlib import Path

import httpx
import pytest_asyncio

BACKEND_DIR = Path(__file__).resolve().parents[2] / "backend"
if str(BACKEND_DIR) not in sys.path:
    sys.path.insert(0, str(BACKEND_DIR))


@pytest_asyncio.fixture
async def backend_client(tmp_path, monkeypatch):
    monkeypatch.setenv("HOMESCREEN_DATA_DIR", str(tmp_path))

    # Force a fresh import so each test gets its own SQLite file per HOMESCREEN_DATA_DIR
    # instead of reusing an engine cached from a previous test's module import.
    for mod_name in list(sys.modules):
        if mod_name == "app" or mod_name.startswith("app."):
            del sys.modules[mod_name]

    from app.database import Base, engine
    from app.main import app as fastapi_app

    Base.metadata.create_all(bind=engine)

    transport = httpx.ASGITransport(app=fastapi_app)
    async with httpx.AsyncClient(transport=transport, base_url="http://testserver") as client:
        yield client
