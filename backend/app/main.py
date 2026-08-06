import asyncio
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from app.api import designs, displays, gateways, integrations
from app.database import Base, engine
from app.services.ha_poller import run_poller


@asynccontextmanager
async def lifespan(app: FastAPI):
    Base.metadata.create_all(bind=engine)
    poller_task = asyncio.create_task(run_poller())
    try:
        yield
    finally:
        poller_task.cancel()


app = FastAPI(title="BLE ePaper Display Backend", lifespan=lifespan)

# No auth in v1 (trusted local network, matches the BLE no-bonding decision).
# CORS is wide open here only because the Vite dev server runs on a different port;
# tighten this once the frontend is served from the same origin as the API.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(gateways.router)
app.include_router(displays.router)
app.include_router(designs.router)
app.include_router(integrations.router)


@app.get("/api/health")
async def health():
    return {"status": "ok"}
