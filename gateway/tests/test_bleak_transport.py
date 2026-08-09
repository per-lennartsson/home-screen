"""
Covers the parts of the real transport that are checkable without a radio: the status
struct layout (which must match firmware's `struct status_value`), MTU/chunk-size
arithmetic, and the scan debounce. The BLE calls themselves are faked — what's being
tested is this module's own logic, not bleak's.
"""

from __future__ import annotations

import asyncio
import struct

import pytest

from gateway import protocol, uuids
from gateway.bleak_transport import MIN_WRITE_SIZE, BleakConnection, BleakTransport


class FakeCharacteristic:
    def __init__(self, max_write_without_response_size: int):
        self.max_write_without_response_size = max_write_without_response_size


class FakeServices:
    def __init__(self, characteristics: dict[str, FakeCharacteristic]):
        self._characteristics = characteristics

    def get_characteristic(self, uuid):
        return self._characteristics.get(uuid)


class FakeClient:
    """Stands in for BleakClient: same async context manager + read/write surface."""

    def __init__(self, address, timeout=None, *, values=None, write_size=244, mtu_size=247):
        self.address = address
        self.timeout = timeout
        self.mtu_size = mtu_size
        self.values = values or {}
        self.writes: list[bytes] = []
        self.connected = False
        self.disconnected = False
        self.services = FakeServices(
            {uuids.DATA_TRANSFER_CHAR_UUID: FakeCharacteristic(write_size)}
        )

    async def __aenter__(self):
        self.connected = True
        return self

    async def __aexit__(self, *exc_info):
        self.disconnected = True
        return False

    async def read_gatt_char(self, uuid, **kwargs):
        return self.values[uuid]

    async def write_gatt_char(self, uuid, data, response=None):
        assert response is True, "data_transfer is write-with-response only (see ble_service.c)"
        self.writes.append((uuid, bytes(data)))


class FakeScanner:
    instances: list["FakeScanner"] = []

    def __init__(self, detection_callback=None, service_uuids=None):
        self.detection_callback = detection_callback
        self.service_uuids = service_uuids
        self.start_count = 0
        self.stop_count = 0
        self.running = False
        FakeScanner.instances.append(self)

    async def start(self):
        self.start_count += 1
        self.running = True

    async def stop(self):
        self.stop_count += 1
        self.running = False


class FakeDevice:
    def __init__(self, address):
        self.address = address
        self.name = uuids.DEVICE_NAME


def _status_bytes(content_hash, battery_pct, battery_mv, fw_version=1):
    # Built with an independent format string rather than STATUS_STRUCT, so this asserts
    # the layout instead of just round-tripping the module's own definition.
    return struct.pack("<IBHH", content_hash, battery_pct, battery_mv, fw_version)


async def test_read_status_matches_firmware_struct_layout():
    client = FakeClient(
        "AA:BB", values={uuids.STATUS_CHAR_UUID: _status_bytes(0xDEADBEEF, 87, 3912)}
    )
    conn = BleakConnection(client, mtu_payload_size=237)

    status = await conn.read_status()

    assert status["content_hash"] == 0xDEADBEEF
    assert status["battery_pct"] == 87
    assert status["battery_mv"] == 3912
    assert status["fw_version"] == 1


async def test_read_status_is_nine_bytes_packed():
    """firmware's `struct status_value` is __packed — if this gateway ever picks up
    natural alignment (13 bytes), every field after content_hash silently shifts."""
    assert struct.calcsize("<IBHH") == 9
    assert len(_status_bytes(1, 2, 3)) == 9


async def test_read_status_rejects_a_short_read():
    client = FakeClient("AA:BB", values={uuids.STATUS_CHAR_UUID: b"\x01\x02\x03"})
    conn = BleakConnection(client, mtu_payload_size=237)

    with pytest.raises(ValueError, match="struct mismatch"):
        await conn.read_status()


async def test_read_status_tolerates_extra_trailing_bytes():
    """A future firmware appending a field shouldn't break an older gateway."""
    client = FakeClient(
        "AA:BB", values={uuids.STATUS_CHAR_UUID: _status_bytes(7, 50, 3700) + b"\xff\xff"}
    )
    conn = BleakConnection(client, mtu_payload_size=237)

    assert (await conn.read_status())["content_hash"] == 7


async def test_read_button_event_returns_the_mask():
    client = FakeClient("AA:BB", values={uuids.BUTTON_EVENT_CHAR_UUID: b"\x05"})
    conn = BleakConnection(client, mtu_payload_size=237)

    assert await conn.read_button_event() == 0b101


async def test_read_button_event_treats_an_empty_read_as_nothing_pending():
    client = FakeClient("AA:BB", values={uuids.BUTTON_EVENT_CHAR_UUID: b""})
    conn = BleakConnection(client, mtu_payload_size=237)

    assert await conn.read_button_event() == 0


async def test_write_data_transfer_targets_the_right_characteristic():
    client = FakeClient("AA:BB")
    conn = BleakConnection(client, mtu_payload_size=237)

    await conn.write_data_transfer(b"\x01\x02\x03")

    assert client.writes == [(uuids.DATA_TRANSFER_CHAR_UUID, b"\x01\x02\x03")]


async def test_connect_derives_chunk_size_from_the_negotiated_write_size():
    clients: list[FakeClient] = []

    def factory(address, timeout=None):
        client = FakeClient(address, timeout, write_size=244, mtu_size=247)
        clients.append(client)
        return client

    transport = BleakTransport(client_factory=factory, scanner_factory=FakeScanner)

    async with transport.connect("AA:BB") as conn:
        # 244 usable write bytes (ATT_MTU 247 - 3) minus the 7-byte chunk header.
        assert conn.mtu_payload_size == 244 - protocol.CHUNK_HEADER_LEN

    assert clients[0].disconnected


async def test_connect_falls_back_to_mtu_size_when_the_characteristic_reports_the_default():
    """BlueZ < 5.62 pins max_write_without_response_size at 20; mtu_size may still be
    right. Take whichever source reports more."""

    def factory(address, timeout=None):
        return FakeClient(address, timeout, write_size=MIN_WRITE_SIZE, mtu_size=185)

    transport = BleakTransport(client_factory=factory, scanner_factory=FakeScanner)

    async with transport.connect("AA:BB") as conn:
        assert conn.mtu_payload_size == (185 - 3) - protocol.CHUNK_HEADER_LEN


async def test_connect_survives_neither_source_negotiating_up():
    """Worst case is slow, not broken: the default 23-byte ATT MTU still leaves a
    positive chunk body."""

    def factory(address, timeout=None):
        return FakeClient(address, timeout, write_size=MIN_WRITE_SIZE, mtu_size=23)

    transport = BleakTransport(client_factory=factory, scanner_factory=FakeScanner)

    async with transport.connect("AA:BB") as conn:
        assert conn.mtu_payload_size == MIN_WRITE_SIZE - protocol.CHUNK_HEADER_LEN
        assert conn.mtu_payload_size > 0


async def test_connect_rejects_a_device_without_our_characteristic():
    """A wrong-firmware device, or a UUID drift between uuids.py and ble_service.c."""

    def factory(address, timeout=None):
        client = FakeClient(address, timeout)
        client.services = FakeServices({})
        return client

    transport = BleakTransport(client_factory=factory, scanner_factory=FakeScanner)

    with pytest.raises(RuntimeError, match="not found"):
        async with transport.connect("AA:BB"):
            pass


async def test_connect_disconnects_even_when_the_transaction_raises():
    clients: list[FakeClient] = []

    def factory(address, timeout=None):
        client = FakeClient(address, timeout)
        clients.append(client)
        return client

    transport = BleakTransport(client_factory=factory, scanner_factory=FakeScanner)

    with pytest.raises(ZeroDivisionError):
        async with transport.connect("AA:BB"):
            1 / 0

    assert clients[0].disconnected


async def test_scan_debounces_repeat_advertisements():
    FakeScanner.instances.clear()
    transport = BleakTransport(
        advertisement_debounce_s=60.0, client_factory=FakeClient, scanner_factory=FakeScanner
    )

    scan = transport.scan()
    first = asyncio.ensure_future(scan.__anext__())
    await asyncio.sleep(0)  # let scan() start the scanner and register the callback

    scanner = FakeScanner.instances[-1]
    assert scanner.service_uuids == [uuids.SERVICE_UUID]

    device = FakeDevice("AA:BB")
    for _ in range(5):  # firmware advertises many times per wake window
        scanner.detection_callback(device, object())

    assert await first == "AA:BB"

    second = asyncio.ensure_future(scan.__anext__())
    scanner.detection_callback(device, object())
    await asyncio.sleep(0.05)
    assert not second.done(), "a repeat advertisement inside the debounce should be ignored"

    scanner.detection_callback(FakeDevice("CC:DD"), object())
    assert await second == "CC:DD"

    second.cancel()
    await scan.aclose()
    assert scanner.stop_count >= 1


async def test_connect_reuses_the_advertising_device_object():
    """bleak has to re-scan for a device given only an address string, and that
    rediscovery can outlast firmware's ~4s advertising window. Connect against the
    BLEDevice the scan already produced instead."""
    FakeScanner.instances.clear()
    targets = []

    def factory(target, timeout=None):
        targets.append(target)
        return FakeClient(getattr(target, "address", target), timeout)

    transport = BleakTransport(client_factory=factory, scanner_factory=FakeScanner)
    scan = transport.scan()
    pending = asyncio.ensure_future(scan.__anext__())
    await asyncio.sleep(0)

    device = FakeDevice("AA:BB")
    FakeScanner.instances[-1].detection_callback(device, object())
    assert await pending == "AA:BB"

    async with transport.connect("AA:BB"):
        pass
    assert targets == [device], "should connect with the BLEDevice, not the address string"

    await scan.aclose()


async def test_connect_falls_back_to_the_address_when_the_device_is_unknown():
    targets = []

    def factory(target, timeout=None):
        targets.append(target)
        return FakeClient(target, timeout)

    transport = BleakTransport(client_factory=factory, scanner_factory=FakeScanner)

    async with transport.connect("AA:BB"):
        pass

    assert targets == ["AA:BB"]


async def test_connect_pauses_scanning_for_the_duration():
    """BlueZ makes connections unreliable while discovery is active, so the scanner must
    be stopped for the transaction and restarted after."""
    FakeScanner.instances.clear()
    transport = BleakTransport(client_factory=FakeClient, scanner_factory=FakeScanner)

    scan = transport.scan()
    pending = asyncio.ensure_future(scan.__anext__())
    await asyncio.sleep(0)
    scanner = FakeScanner.instances[-1]
    assert scanner.running

    async with transport.connect("AA:BB"):
        assert not scanner.running
    assert scanner.running

    pending.cancel()
    with pytest.raises(asyncio.CancelledError):
        await pending  # let the cancellation land before closing the generator
    await scan.aclose()
