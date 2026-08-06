"""
Proves the physical-button round trip end to end against a mock BLE peripheral and the
real backend app: a press on the mock device reaches the backend as an ElementLiveValue
toggle within the *same* connection that reports status, and the very next payload for
that connection already reflects it as a cheap diff — the ordering decision in
gateway/gateway/sync.py::_sync_device (button_event read/report happens before
get_payload, not after).
"""

from gateway.backend_client import BackendClient
from gateway.ble_transport import MockBleTransport, MockDisplay
from gateway.sync import GatewayService

CHECKLIST_LAYOUT = {
    "elements": [
        {"id": 1, "type": "text", "x": 0, "y": 0, "w": 100, "h": 10, "props": {"text": "Chores"}},
        {
            "id": 2,
            "type": "value",
            "x": 0,
            "y": 10,
            "w": 100,
            "h": 10,
            "props": {"source": "button", "value": "Take out trash", "button_index": 0},
        },
    ]
}


async def _register(client, mac="AA:AA:AA:AA:AA:09"):
    gw = (await client.post("/api/gateways", json={"name": "gw-buttons"})).json()
    disp = (
        await client.post(
            "/api/displays", json={"name": "d-buttons", "mac_address": mac, "gateway_id": gw["id"]}
        )
    ).json()
    design = (
        await client.post("/api/designs", json={"name": "checklist", "layout_json": CHECKLIST_LAYOUT})
    ).json()
    await client.post(f"/api/displays/{disp['id']}/assign", json={"design_id": design["id"]})
    return gw, disp, design


async def _get_display(client, display_id):
    return (await client.get(f"/api/displays/{display_id}")).json()


async def _run_cycles_until_in_sync(service, client, mac_address, display_id, max_cycles=5):
    for _ in range(max_cycles):
        await service.handle_advertisement(mac_address)
        display = await _get_display(client, display_id)
        if display["in_sync"]:
            return display
    raise AssertionError(f"display {display_id} did not reach in_sync within {max_cycles} cycles")


async def test_button_press_toggles_checked_and_is_visible_same_connection(backend_client):
    gw, disp, design = await _register(backend_client)

    mock_display = MockDisplay(mac_address=disp["mac_address"])
    transport = MockBleTransport([mock_display])
    service = GatewayService(
        gateway_id=gw["id"], backend=BackendClient(backend_client), transport=transport, checkin_interval_s=0
    )
    await service.refresh_assigned_displays()

    # Get the device fully in sync first (unchecked state), same as any other design.
    await _run_cycles_until_in_sync(service, backend_client, disp["mac_address"], disp["id"])
    assert mock_display.last_rendered["elements"][1]["checkable"] is True
    assert mock_display.last_rendered["elements"][1]["checked"] is False

    # Simulate a physical press on row 0 (the only button-sourced row in this design).
    mock_display.press_button(0)

    # One advertisement cycle: reads status, reports it, reads+reports the button event,
    # THEN fetches payload — so this single connection's push already reflects the
    # toggle instead of waiting a whole extra wake cycle for it to show up.
    await service.handle_advertisement(disp["mac_address"])

    assert mock_display.pending_button_mask == 0  # cleared on read, same as firmware would
    assert mock_display.last_rendered["elements"][1]["checked"] is True
    assert mock_display.last_rendered["elements"][1]["text"] == "Take out trash"  # label untouched

    mid_flight = await _get_display(backend_client, disp["id"])
    assert mid_flight["in_sync"] is False  # backend doesn't know the push landed yet (spec 5.1)

    confirmed = await _run_cycles_until_in_sync(service, backend_client, disp["mac_address"], disp["id"])
    assert confirmed["in_sync"] is True


async def test_button_press_with_no_bound_row_is_ignored(backend_client):
    """A press on a physical button index with no matching element on the currently
    assigned design must not error or desync the display — see
    api/displays.py::report_button_event's "no row on this design bound to this
    physical button - ignore" branch."""
    gw, disp, design = await _register(backend_client)

    mock_display = MockDisplay(mac_address=disp["mac_address"])
    transport = MockBleTransport([mock_display])
    service = GatewayService(
        gateway_id=gw["id"], backend=BackendClient(backend_client), transport=transport, checkin_interval_s=0
    )
    await service.refresh_assigned_displays()
    await _run_cycles_until_in_sync(service, backend_client, disp["mac_address"], disp["id"])

    mock_display.press_button(4)  # no element bound to row 4 in CHECKLIST_LAYOUT
    await service.handle_advertisement(disp["mac_address"])

    still_synced = await _get_display(backend_client, disp["id"])
    assert still_synced["in_sync"] is True
