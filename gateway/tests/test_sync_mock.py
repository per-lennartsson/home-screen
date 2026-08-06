"""
Proves the sync loop end-to-end against a mock BLE peripheral and the real backend app
(Section 7 build-order step 2) — full sync, value-diff sync, per-connection failure with
next-cycle retry, and one flaky device not blocking another.

Important protocol nuance these tests surface: a single wake/advertisement cycle only
pushes new content — it never re-reads `status` afterward to confirm the push landed
(spec 5.1 has no such step; each transaction stays short and self-contained, per the
duty-cycle design in spec 3). Confirmation that a display is actually in sync only
happens on the *next* cycle, when the gateway reads the now-updated on-device hash and
reports it. So convergence to `in_sync` always takes at least two `handle_advertisement`
calls, and a failed push takes three (push, discover-still-stale, push again, confirm).
That's why the tests below drive multiple cycles instead of asserting after one.
"""

from gateway import protocol
from gateway.backend_client import BackendClient
from gateway.ble_transport import MockBleTransport, MockDisplay
from gateway.sync import GatewayService

LAYOUT_V1 = {
    "elements": [
        {"id": 1, "type": "text", "x": 0, "y": 0, "w": 50, "h": 10, "props": {"text": "Temp"}},
        {"id": 2, "type": "value", "x": 0, "y": 10, "w": 50, "h": 10, "props": {"value": "21.5C"}},
    ]
}
LAYOUT_V2 = {
    "elements": [
        {"id": 1, "type": "text", "x": 0, "y": 0, "w": 50, "h": 10, "props": {"text": "Temp"}},
        {"id": 2, "type": "value", "x": 0, "y": 10, "w": 50, "h": 10, "props": {"value": "22.1C"}},
    ]
}


async def _register(client, mac="AA:AA:AA:AA:AA:01", gw_name="gw-1"):
    gw = (await client.post("/api/gateways", json={"name": gw_name})).json()
    disp = (
        await client.post(
            "/api/displays", json={"name": "d1", "mac_address": mac, "gateway_id": gw["id"]}
        )
    ).json()
    design = (
        await client.post("/api/designs", json={"name": "design", "layout_json": LAYOUT_V1})
    ).json()
    await client.post(f"/api/displays/{disp['id']}/assign", json={"design_id": design["id"]})
    return gw, disp, design


async def _get_display(client, display_id):
    return (await client.get(f"/api/displays/{display_id}")).json()


async def _run_cycles_until_in_sync(service, client, mac_address, display_id, max_cycles=5):
    """Drives successive advertisement/wake cycles — see module docstring for why more
    than one is expected — until the backend reports the display in sync."""
    for _ in range(max_cycles):
        await service.handle_advertisement(mac_address)
        display = await _get_display(client, display_id)
        if display["in_sync"]:
            return display
    raise AssertionError(f"display {display_id} did not reach in_sync within {max_cycles} cycles")


async def test_full_sync_brings_mock_display_in_sync(backend_client):
    gw, disp, design = await _register(backend_client)

    mock_display = MockDisplay(mac_address=disp["mac_address"])
    transport = MockBleTransport([mock_display])
    service = GatewayService(
        gateway_id=gw["id"], backend=BackendClient(backend_client), transport=transport, checkin_interval_s=0
    )

    await service.refresh_assigned_displays()

    # Cycle 1: pushes the full render, but the backend hasn't heard about it yet.
    await service.handle_advertisement(disp["mac_address"])
    mid_flight = await _get_display(backend_client, disp["id"])
    assert mid_flight["in_sync"] is False
    assert mock_display.content_hash == mid_flight["desired_content_hash"]  # device already has it
    # last_rendered is what MockDisplay decoded from the actual binary wire format
    # (protocol.encode_full_layout), not the original layout_json — round-tripping
    # LAYOUT_V1 through encode/decode gives the same lossy-but-normalized shape real
    # firmware would end up with.
    assert mock_display.last_rendered == protocol.decode_full_layout(protocol.encode_full_layout(LAYOUT_V1))

    # Cycle 2: gateway reads the now-updated status and reports it -> confirmed in sync.
    await service.handle_advertisement(disp["mac_address"])
    confirmed = await _get_display(backend_client, disp["id"])
    assert confirmed["in_sync"] is True
    assert confirmed["current_content_hash"] == confirmed["desired_content_hash"]


async def test_value_only_change_pushes_diff_not_full(backend_client):
    gw, disp, design = await _register(backend_client)

    mock_display = MockDisplay(mac_address=disp["mac_address"])
    transport = MockBleTransport([mock_display])
    service = GatewayService(
        gateway_id=gw["id"], backend=BackendClient(backend_client), transport=transport, checkin_interval_s=0
    )

    await service.refresh_assigned_displays()
    await _run_cycles_until_in_sync(service, backend_client, disp["mac_address"], disp["id"])

    await backend_client.put(f"/api/designs/{design['id']}", json={"layout_json": LAYOUT_V2})

    payload = await BackendClient(backend_client).get_payload(disp["id"])
    assert payload["type"] == "diff"  # structure unchanged, only element 2's value differs

    await _run_cycles_until_in_sync(service, backend_client, disp["mac_address"], disp["id"])
    # text element untouched, value patched via the diff path (not a re-sent full layout)
    assert mock_display.last_rendered == protocol.decode_full_layout(protocol.encode_full_layout(LAYOUT_V2))


async def test_corrupted_write_is_retried_and_eventually_confirmed(backend_client):
    gw, disp, design = await _register(backend_client)

    mock_display = MockDisplay(mac_address=disp["mac_address"])
    mock_display.corrupt_next_write = True  # simulates one bad chunk over the air
    transport = MockBleTransport([mock_display])
    service = GatewayService(
        gateway_id=gw["id"], backend=BackendClient(backend_client), transport=transport, checkin_interval_s=0
    )

    await service.refresh_assigned_displays()

    # Cycle 1: push is corrupted, firmware discards it silently (spec 4.3) -> hash unmoved.
    await service.handle_advertisement(disp["mac_address"])
    assert mock_display.content_hash == 0

    # No retry happens inside that connection (spec 5.1 step 8) — it takes the normal
    # multi-cycle path (push again, then confirm) to recover, same as any other sync.
    final = await _run_cycles_until_in_sync(service, backend_client, disp["mac_address"], disp["id"], max_cycles=5)
    assert final["in_sync"] is True


async def test_one_flaky_display_does_not_block_another(backend_client):
    gw = (await backend_client.post("/api/gateways", json={"name": "gw-multi"})).json()
    design = (await backend_client.post("/api/designs", json={"name": "d", "layout_json": LAYOUT_V1})).json()

    healthy_disp = (
        await backend_client.post(
            "/api/displays", json={"name": "healthy", "mac_address": "AA:AA:AA:AA:AA:02", "gateway_id": gw["id"]}
        )
    ).json()
    flaky_disp = (
        await backend_client.post(
            "/api/displays", json={"name": "flaky", "mac_address": "AA:AA:AA:AA:AA:03", "gateway_id": gw["id"]}
        )
    ).json()
    for d in (healthy_disp, flaky_disp):
        await backend_client.post(f"/api/displays/{d['id']}/assign", json={"design_id": design["id"]})

    healthy_mock = MockDisplay(mac_address=healthy_disp["mac_address"])
    flaky_mock = MockDisplay(mac_address=flaky_disp["mac_address"])
    flaky_mock.fail_next_connects = 1  # first connection attempt raises ConnectionError

    transport = MockBleTransport([healthy_mock, flaky_mock])
    service = GatewayService(
        gateway_id=gw["id"], backend=BackendClient(backend_client), transport=transport, checkin_interval_s=0
    )
    await service.refresh_assigned_displays()

    # Interleave both devices every round — a scan pass sees every advertisement in
    # whatever order they arrive, so the flaky one's failure must not stall the healthy
    # one's progress within or across rounds.
    for _ in range(5):
        await service.handle_advertisement(flaky_disp["mac_address"])
        await service.handle_advertisement(healthy_disp["mac_address"])
        if (await _get_display(backend_client, healthy_disp["id"]))["in_sync"] and (
            await _get_display(backend_client, flaky_disp["id"])
        )["in_sync"]:
            break

    healthy_after = await _get_display(backend_client, healthy_disp["id"])
    flaky_after = await _get_display(backend_client, flaky_disp["id"])
    assert healthy_after["in_sync"] is True
    assert flaky_after["in_sync"] is True
