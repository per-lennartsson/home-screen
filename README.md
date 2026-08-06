# BLE ePaper Display System

Backend + web GUI for managing BLE ePaper displays. See [docs/protocol.md](docs/protocol.md)
for cross-component wire-protocol decisions, and [firmware/README.md](firmware/README.md)
for firmware-specific notes.

## Running with Docker

Only Docker is required — no local Python or Node install needed.

```bash
docker compose up --build
```

- Backend: http://localhost:8000 (SQLite data persists in the `backend_data` volume)
- Frontend: http://localhost:5173

Backend code changes require a restart (`docker compose restart backend`) — see the
comment in `docker-compose.yml` for why `--reload` isn't on by default and how to
re-enable it if your host supports it. Frontend changes hot-reload automatically via Vite.

To stop: `docker compose down` (add `-v` to also drop the persisted backend data).

## Components

- `backend/` — FastAPI + SQLite, design/display/gateway management API.
- `frontend/` — React (Vite) dashboard: design editor, gateway/display registration, sync status.
- `gateway/` — Python BLE gateway (bleak), proven against a mock peripheral in `gateway/tests/`.
- `firmware/` — Zephyr/nRF Connect SDK firmware for the NRF52840 display hardware.
