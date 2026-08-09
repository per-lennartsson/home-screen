# BLE ePaper Display System

Backend + web GUI for managing BLE ePaper displays. See [docs/protocol.md](docs/protocol.md)
for cross-component wire-protocol decisions, [gateway/README.md](gateway/README.md) for
running against real displays, and [firmware/README.md](firmware/README.md) for
firmware-specific notes.

## Running with Docker

Only Docker is required — no local Python or Node install needed. This covers the
backend and frontend; the gateway runs outside Docker (see below).

```bash
docker compose up --build
```

- Backend: http://localhost:8000 (SQLite data persists in the `backend_data` volume)
- Frontend: http://localhost:5173

Backend code changes require a restart (`docker compose restart backend`) — see the
comment in `docker-compose.yml` for why `--reload` isn't on by default and how to
re-enable it if your host supports it. Frontend changes hot-reload automatically via Vite.

To stop: `docker compose down` (add `-v` to also drop the persisted backend data).

## Running against real displays

The gateway needs the host's Bluetooth radio, which Docker Desktop doesn't share with
containers, so it runs as a normal process alongside the compose stack:

```bash
python3 -m venv gateway/.venv && gateway/.venv/bin/pip install -r gateway/requirements.txt
gateway/.venv/bin/python -m gateway scan          # find your display's address
gateway/.venv/bin/python -m gateway run --gateway-id 1
```

[gateway/README.md](gateway/README.md) has the full first-run walkthrough — registering
the display, the macOS Bluetooth-permission step, why the address on a Mac isn't the
board's MAC, and how long a sync actually takes.

## Components

- `backend/` — FastAPI + SQLite, design/display/gateway management API.
- `frontend/` — React (Vite) dashboard: design editor, gateway/display registration, sync status.
- `gateway/` — Python BLE gateway (bleak). Runs on the host, not in Docker; also has a
  mock peripheral in `gateway/tests/` so the sync loop is testable without hardware.
- `firmware/` — Zephyr/nRF Connect SDK firmware for the NRF52840 display hardware.
