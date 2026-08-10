"""Thin async wrapper around the backend endpoints the gateway needs (spec 6.2)."""

from __future__ import annotations

import httpx


class BackendClient:
    def __init__(self, client: httpx.AsyncClient):
        # Caller owns construction of the httpx client: base_url + auth for a real
        # deployment, or an httpx.ASGITransport pointed straight at the FastAPI app
        # for tests. Keeps this class agnostic to how the backend is actually reached.
        self._client = client

    async def get_assigned_displays(self, gateway_id: int) -> list[dict]:
        resp = await self._client.get(f"/api/gateways/{gateway_id}/assigned-displays")
        resp.raise_for_status()
        return resp.json()

    async def report_status(self, display_id: int, content_hash: int, battery_pct: int, battery_mv: int) -> dict:
        resp = await self._client.post(
            f"/api/displays/{display_id}/status",
            json={"content_hash": content_hash, "battery_pct": battery_pct, "battery_mv": battery_mv},
        )
        resp.raise_for_status()
        return resp.json()

    async def report_button_event(self, display_id: int, button_mask: int) -> dict:
        resp = await self._client.post(
            f"/api/displays/{display_id}/button-event", json={"button_mask": button_mask}
        )
        resp.raise_for_status()
        return resp.json()

    async def get_payload(self, display_id: int) -> dict:
        resp = await self._client.get(f"/api/displays/{display_id}/payload")
        resp.raise_for_status()
        return resp.json()

    async def ack_full_refresh(self, display_id: int) -> dict:
        resp = await self._client.post(f"/api/displays/{display_id}/full-refresh-ack")
        resp.raise_for_status()
        return resp.json()
