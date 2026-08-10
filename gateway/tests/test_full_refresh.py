"""
Proves the full-refresh (anti-ghosting) trigger end to end against a mock BLE peripheral
and the real backend app: both the one-shot manual "refresh now" request and the
recurring schedule reach the display as exactly one COMMAND_FORCE_FULL_REFRESH write,
then get acked back to the backend so they aren't repeated on every subsequent sync
(unlike rotate_180/wake_interval_s, which are asserted every sync regardless).
"""

from gateway import uuids
from gateway.backend_client import BackendClient
from gateway.ble_transport import MockBleTransport, MockDisplay
from gateway.sync import GatewayService

LAYOUT = {
    "elements": [
        {"id": 1, "type": "text", "x": 0, "y": 0, "w": 50, "h": 10, "props": {"text": "Temp"}},
    ]
}


async def _register(client, mac="AA:AA:AA:AA:AA:20"):
    gw = (await client.post("/api/gateways", json={"name": "gw-refresh"})).json()
    disp = (
        await client.post(
            "/api/displays", json={"name": "d-refresh", "mac_address": mac, "gateway_id": gw["id"]}
        )
    ).json()
    design = (await client.post("/api/designs", json={"name": "design", "layout_json": LAYOUT})).json()
    await client.post(f"/api/displays/{disp['id']}/assign", json={"design_id": design["id"]})
    return gw, disp, design


def _make_service(gw, disp, client):
    mock_display = MockDisplay(mac_address=disp["mac_address"])
    transport = MockBleTransport([mock_display])
    service = GatewayService(
        gateway_id=gw["id"], backend=BackendClient(client), transport=transport, checkin_interval_s=0
    )
    return mock_display, service


async def test_manual_full_refresh_is_sent_once_and_acked(backend_client):
    gw, disp, design = await _register(backend_client)
    mock_display, service = _make_service(gw, disp, backend_client)

    await service.refresh_assigned_displays()
    await service.handle_advertisement(disp["mac_address"])  # brings content in sync, no refresh due yet
    assert mock_display.full_refresh_count == 0

    triggered = (await backend_client.post(f"/api/displays/{disp['id']}/force-full-refresh")).json()
    assert triggered["full_refresh_due"] is True

    await service.refresh_assigned_displays()
    await service.handle_advertisement(disp["mac_address"])

    assert mock_display.full_refresh_count == 1
    assert mock_display.last_command == uuids.COMMAND_FORCE_FULL_REFRESH

    after = (await backend_client.get(f"/api/displays/{disp['id']}")).json()
    assert after["full_refresh_due"] is False
    assert after["last_full_refresh_at"] is not None

    # A subsequent sync must not re-send it - it's one-shot, not asserted every sync
    # like rotate_180/wake_interval_s.
    await service.refresh_assigned_displays()
    await service.handle_advertisement(disp["mac_address"])
    assert mock_display.full_refresh_count == 1


async def test_scheduled_full_refresh_fires_when_never_yet_refreshed(backend_client):
    gw, disp, design = await _register(backend_client)
    mock_display, service = _make_service(gw, disp, backend_client)

    scheduled = (
        await backend_client.post(
            f"/api/displays/{disp['id']}/full-refresh-interval", json={"full_refresh_interval_s": 3600}
        )
    ).json()
    # No last_full_refresh_at yet -> due immediately, same as a freshly registered display
    # that's never had its ghosting cleared.
    assert scheduled["full_refresh_due"] is True

    await service.refresh_assigned_displays()
    await service.handle_advertisement(disp["mac_address"])

    assert mock_display.full_refresh_count == 1
    after = (await backend_client.get(f"/api/displays/{disp['id']}")).json()
    assert after["full_refresh_due"] is False
    assert after["full_refresh_interval_s"] == 3600
