"""
Runner-level behaviour, driven through MockBleTransport + the real backend app (via the
`backend_client` fixture in conftest.py). Together with test_sync_mock.py this covers the
whole process path except the radio itself: config -> backend preflight -> scan loop ->
sync -> content on the (mock) device.
"""

from __future__ import annotations

import asyncio

import httpx
import pytest

from gateway import runner
from gateway.backend_client import BackendClient
from gateway.ble_transport import MockBleTransport, MockDisplay
from gateway.config import GatewayConfig, config_from_args, parse_args
from gateway.sync import GatewayService

MAC = "AA:BB:CC:DD:EE:FF"


async def _register(client: httpx.AsyncClient, mac: str = MAC) -> tuple[int, int]:
    gateway_id = (await client.post("/api/gateways", json={"name": "test-gw"})).json()["id"]
    display_id = (
        await client.post(
            "/api/displays", json={"name": "kitchen", "mac_address": mac, "gateway_id": gateway_id}
        )
    ).json()["id"]
    return gateway_id, display_id


async def test_wait_for_backend_returns_assigned_displays(backend_client):
    gateway_id, display_id = await _register(backend_client)

    displays = await runner.wait_for_backend(BackendClient(backend_client), gateway_id)

    assert [d["id"] for d in displays] == [display_id]


async def test_wait_for_backend_rejects_an_unregistered_gateway_id(backend_client):
    """A typo'd id must fail loudly at startup rather than retry forever — it can never
    become valid on its own."""
    with pytest.raises(runner.GatewayNotRegisteredError, match="no gateway with id 999"):
        await runner.wait_for_backend(BackendClient(backend_client), 999)


async def test_wait_for_backend_retries_while_the_backend_is_down(monkeypatch):
    """Starting the gateway before `docker compose up` is normal, not fatal."""
    monkeypatch.setattr(runner, "BACKEND_RETRY_INTERVAL_S", 0.01)
    attempts = 0

    async def handler(request: httpx.Request) -> httpx.Response:
        nonlocal attempts
        attempts += 1
        if attempts < 3:
            raise httpx.ConnectError("connection refused", request=request)
        return httpx.Response(200, json=[])

    async with httpx.AsyncClient(
        transport=httpx.MockTransport(handler), base_url="http://backend"
    ) as client:
        assert await runner.wait_for_backend(BackendClient(client), 1) == []

    assert attempts == 3


async def test_run_service_syncs_a_display_end_to_end(backend_client):
    gateway_id, display_id = await _register(backend_client)
    design = (
        await backend_client.post(
            "/api/designs",
            json={
                "name": "d",
                "layout_json": {
                    "elements": [{"id": 1, "type": "text", "x": 10, "y": 10, "props": {"text": "hi"}}]
                },
            },
        )
    ).json()
    await backend_client.post(f"/api/displays/{display_id}/assign", json={"design_id": design["id"]})

    display = MockDisplay(MAC)
    transport = MockBleTransport([display], advertise_interval=0.01)
    service = GatewayService(gateway_id, BackendClient(backend_client), transport, checkin_interval_s=0.0)
    await service.refresh_assigned_displays()

    config = GatewayConfig(gateway_id=gateway_id, refresh_interval_s=3600.0)
    task = asyncio.ensure_future(runner.run_service(service, transport, config))
    try:
        # Convergence takes two wake cycles by design (docs/protocol.md) — poll rather
        # than assuming a fixed number of advertisements got through.
        for _ in range(200):
            await asyncio.sleep(0.01)
            if display.last_rendered is not None:
                break
    finally:
        task.cancel()
        with pytest.raises(asyncio.CancelledError):
            await task

    assert display.last_rendered["elements"][0]["text"] == "hi"


async def test_run_service_ignores_advertisements_from_unregistered_displays(backend_client, caplog):
    """On macOS the registered address is a CoreBluetooth UUID, so an address mismatch is
    the likeliest bring-up failure. It has to produce a log line, not silence."""
    gateway_id, _ = await _register(backend_client, mac="11:22:33:44:55:66")

    stranger = MockDisplay("99:88:77:66:55:44")
    transport = MockBleTransport([stranger], advertise_interval=0.01)
    service = GatewayService(gateway_id, BackendClient(backend_client), transport)
    await service.refresh_assigned_displays()

    config = GatewayConfig(gateway_id=gateway_id, refresh_interval_s=3600.0)
    with caplog.at_level("INFO", logger="gateway.runner"):
        task = asyncio.ensure_future(runner.run_service(service, transport, config))
        await asyncio.sleep(0.05)
        task.cancel()
        with pytest.raises(asyncio.CancelledError):
            await task

    hints = [r for r in caplog.records if "not registered to this gateway" in r.message]
    assert hints, "an unregistered display advertising should be surfaced"
    assert stranger.last_rendered is None


async def test_refresh_loop_survives_a_failing_refresh():
    """A backend blip must not stop the loop — the previous display list stays usable."""
    calls = 0

    class FlakyService:
        devices: dict = {}

        async def refresh_assigned_displays(self):
            nonlocal calls
            calls += 1
            raise RuntimeError("backend down")

    task = asyncio.ensure_future(runner.refresh_loop(FlakyService(), 0.01))
    await asyncio.sleep(0.05)
    task.cancel()
    with pytest.raises(asyncio.CancelledError):
        await task

    assert calls >= 2


def test_bare_invocation_defaults_to_run():
    args = parse_args([], env={"HOMESCREEN_GATEWAY_ID": "7"})
    assert args.command == "run"
    assert config_from_args(args).gateway_id == 7


def test_cli_flags_win_over_the_environment():
    args = parse_args(
        ["run", "--gateway-id", "2", "--backend-url", "http://pi:8000"],
        env={"HOMESCREEN_GATEWAY_ID": "7", "HOMESCREEN_BACKEND_URL": "http://mac:8000"},
    )
    config = config_from_args(args)

    assert config.gateway_id == 2
    assert config.backend_url == "http://pi:8000"


def test_run_requires_a_gateway_id():
    with pytest.raises(SystemExit):
        parse_args(["run"], env={})
