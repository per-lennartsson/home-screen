"""
Real BLE transport (bleak), implementing the same `BleTransport`/`BleConnection`
interface `MockBleTransport` does — so `GatewayService` is unchanged between the mock
integration tests and real hardware, which is the whole reason ble_transport.py put the
abstraction there in the first place.

Everything here is either mechanically checkable against firmware (the status struct
layout, the characteristic UUIDs) or a bleak/OS behaviour that can only really be
confirmed with a board in front of you (MTU negotiation, scan/connect interleaving). The
former is unit-tested in tests/test_bleak_transport.py against fakes; the latter is
commented with what was assumed and why.

## Addresses are not MAC addresses on macOS

`BLEDevice.address` is a real BD_ADDR on Linux/BlueZ, but on macOS CoreBluetooth refuses
to expose it and bleak substitutes a per-host, per-device CoreBluetooth UUID string. That
string is what this transport yields from `scan()` and accepts in `connect()`, and it is
therefore what has to go in a display's `mac_address` field in the backend.

Consequences worth knowing before you register anything:
  - The value you register on a Mac will not match the MAC printed on the board, and will
    not be recognised by a gateway running on a different host. Re-register if you move
    the gateway.
  - `python -m gateway scan` exists precisely so you can read off the right value instead
    of guessing.
"""

from __future__ import annotations

import asyncio
import contextlib
import logging
import struct
import time
from collections.abc import AsyncIterator, Callable
from contextlib import asynccontextmanager

from bleak import BleakClient, BleakScanner
from bleak.backends.device import BLEDevice

from gateway import protocol, uuids
from gateway.ble_transport import BleConnection, BleTransport

logger = logging.getLogger("gateway.ble")

# `struct status_value` in firmware/src/ble_service.c: __packed, all little-endian.
# uint32 content_hash, uint8 battery_pct, uint16 battery_mv, uint16 fw_version. This is
# the *guaranteed* prefix — charging (below) was appended later and is read separately,
# defensively, so a display still running pre-charge-status firmware (shorter status
# read) doesn't fail here.
STATUS_STRUCT = struct.Struct("<IBHH")

# An ATT Write Request spends 3 bytes on opcode + handle, so the largest writable value
# is ATT_MTU - 3. Same overhead for write-with-response (which is what data_transfer
# uses: BT_GATT_CHRC_WRITE only, no WRITE_WITHOUT_RESP).
ATT_WRITE_OVERHEAD = 3

# Worst case if MTU negotiation never happens: the 23-byte default ATT MTU. bleak reports
# this as a max write size of 20.
MIN_WRITE_SIZE = 20

# bleak's docs warn that a device can be slow to report its negotiated write size, and
# that reading it again shortly after connecting may return the real (larger) value. Poll
# briefly for that rather than permanently falling back to 13-byte chunk bodies — but keep
# the budget small, since it burns into firmware's 4-second advertising window.
MTU_SETTLE_TIMEOUT_S = 0.5
MTU_SETTLE_POLL_S = 0.1


class BleakConnection(BleConnection):
    """One connected session with a real display. Created by BleakTransport.connect()
    and only valid inside that context manager."""

    def __init__(self, client: BleakClient, mtu_payload_size: int):
        self._client = client
        self._mtu_payload_size = mtu_payload_size

    async def read_status(self) -> dict:
        raw = await self._client.read_gatt_char(uuids.STATUS_CHAR_UUID)
        if len(raw) < STATUS_STRUCT.size:
            raise ValueError(
                f"status characteristic returned {len(raw)} bytes, expected at least "
                f"{STATUS_STRUCT.size} — firmware/gateway struct mismatch?"
            )
        # Tolerate a longer read than expected: a future firmware may append fields, and
        # every field this gateway cares about is ahead of anything it would add.
        content_hash, battery_pct, battery_mv, fw_version = STATUS_STRUCT.unpack(
            raw[: STATUS_STRUCT.size]
        )
        # charging (struct status_value's 10th byte, fw_version 2+): None rather than
        # False on an older, not-yet-reflashed display — the backend falls back to its
        # own voltage-trend inference only when it actually doesn't know, and "doesn't
        # know" and "confirmed not charging" shouldn't look the same.
        charging = bool(raw[STATUS_STRUCT.size]) if len(raw) > STATUS_STRUCT.size else None
        return {
            "content_hash": content_hash,
            "battery_pct": battery_pct,
            "battery_mv": battery_mv,
            "fw_version": fw_version,
            "charging": charging,
        }

    async def read_button_event(self) -> int:
        raw = await self._client.read_gatt_char(uuids.BUTTON_EVENT_CHAR_UUID)
        if not raw:
            return 0  # nothing pending; firmware always returns 1 byte, but don't crash
        return raw[0]

    async def write_data_transfer(self, chunk: bytes) -> None:
        # response=True is required, not just preferred: firmware's reassembler demands
        # strictly ascending chunk indices (firmware/src/chunk_protocol.h), so each write
        # must be acknowledged before the next is queued.
        await self._client.write_gatt_char(uuids.DATA_TRANSFER_CHAR_UUID, chunk, response=True)

    async def write_command(self, command: int, value: bytes = b"") -> None:
        """Used by sync.py to assert a display's rotate_180/wake_interval_s settings
        every sync, and by the diagnostics in __main__.py (e.g. triggering IDENTIFY on a
        specific display)."""
        await self._client.write_gatt_char(
            uuids.COMMAND_CHAR_UUID, bytes([command]) + value, response=True
        )

    @property
    def mtu_payload_size(self) -> int:
        return self._mtu_payload_size


async def _resolve_write_size(client: BleakClient) -> int:
    """Largest value we can put in one write to data_transfer, in bytes.

    Two sources, because neither is reliable everywhere:
      - `characteristic.max_write_without_response_size` is what bleak's own docs point
        at, and is correct on CoreBluetooth and BlueZ >= 5.62. Despite the name it's the
        ATT_MTU - 3 figure, which is the write-with-response limit too.
      - `client.mtu_size` is documented to always return 23 on BlueZ, so it can't be used
        alone — but it's a valid fallback where the first source returns the 20-byte
        default.
    Take whichever is larger and never go below the 20-byte ATT default.
    """
    char = client.services.get_characteristic(uuids.DATA_TRANSFER_CHAR_UUID)
    if char is None:
        raise RuntimeError(
            f"data_transfer characteristic {uuids.DATA_TRANSFER_CHAR_UUID} not found — "
            "is the device running this project's firmware, and do gateway/gateway/uuids.py "
            "and firmware/src/ble_service.c still agree?"
        )

    deadline = time.monotonic() + MTU_SETTLE_TIMEOUT_S
    size = MIN_WRITE_SIZE
    while True:
        candidates = [MIN_WRITE_SIZE]
        with contextlib.suppress(Exception):
            candidates.append(int(char.max_write_without_response_size))
        with contextlib.suppress(Exception):
            candidates.append(int(client.mtu_size) - ATT_WRITE_OVERHEAD)
        size = max(candidates)

        if size > MIN_WRITE_SIZE or time.monotonic() >= deadline:
            break
        await asyncio.sleep(MTU_SETTLE_POLL_S)

    if size <= MIN_WRITE_SIZE:
        logger.warning(
            "MTU negotiation did not raise the write size above the %d-byte ATT default; "
            "transfers will use %d-byte chunk bodies and be slow",
            MIN_WRITE_SIZE,
            MIN_WRITE_SIZE - protocol.CHUNK_HEADER_LEN,
        )
    return size


class BleakTransport(BleTransport):
    """
    Scanning and connecting are deliberately mutually exclusive (`_scan_paused` around
    each connection). BlueZ is documented to make connections unreliable while discovery
    is active, and the gateway has no reason to keep scanning during the few hundred
    milliseconds a transaction takes — displays that advertise meanwhile will advertise
    again next cycle.
    """

    def __init__(
        self,
        *,
        connect_timeout_s: float = 10.0,
        advertisement_debounce_s: float = 2.0,
        mtu_payload_size_override: int | None = None,
        client_factory: Callable[..., BleakClient] = BleakClient,
        scanner_factory: Callable[..., BleakScanner] = BleakScanner,
    ):
        self._connect_timeout_s = connect_timeout_s
        self._advertisement_debounce_s = advertisement_debounce_s
        self._mtu_payload_size_override = mtu_payload_size_override
        self._client_factory = client_factory
        self._scanner_factory = scanner_factory

        self._scanner: BleakScanner | None = None
        self._connect_lock = asyncio.Lock()
        self._queue: asyncio.Queue[str] = asyncio.Queue()
        self._queued: set[str] = set()
        self._last_yielded: dict[str, float] = {}

        # BLEDevice objects from the most recent advertisement, keyed by address. Handing
        # one of these to BleakClient instead of the bare address string matters here:
        # given a string, bleak has to go find the device again with its own scan before
        # it can connect, and firmware's advertising window is only ~4 seconds wide — a
        # rediscovery scan can easily outlast it and turn every connection into a
        # timeout. The device object skips that entirely.
        self._devices: dict[str, BLEDevice] = {}

    async def scan(self) -> AsyncIterator[str]:
        """Yield an address each time one of our displays advertises.

        Debounced per address: firmware advertises many times during its ~4s window, and
        `GatewayService.handle_advertisement` is a no-op for all but the first of those,
        so there's nothing to gain from waking it up for every packet.
        """

        def on_detection(device, advertisement_data) -> None:
            address = device.address
            # Always refresh, even for an advertisement we're about to drop: a stale
            # BLEDevice is worse than none, and this is the only place they come from.
            self._devices[address] = device
            if address in self._queued:
                return  # already waiting to be handled; don't let the queue grow unbounded
            self._queued.add(address)
            self._queue.put_nowait(address)

        # service_uuids filters in the backend where possible (a CoreBluetooth scan
        # filter, a BlueZ discovery filter) rather than in Python, which also stops macOS
        # from surfacing every unrelated device in range.
        self._scanner = self._scanner_factory(
            detection_callback=on_detection, service_uuids=[uuids.SERVICE_UUID]
        )
        await self._scanner.start()
        logger.info("scanning for displays advertising service %s", uuids.SERVICE_UUID)

        try:
            while True:
                address = await self._queue.get()
                self._queued.discard(address)

                # Re-check the debounce at dequeue rather than at detection: a connection
                # may have happened while this entry sat in the queue, and the address is
                # marked as just-seen when that connection ends.
                now = time.monotonic()
                previous = self._last_yielded.get(address)
                if previous is not None and (now - previous) < self._advertisement_debounce_s:
                    continue
                self._last_yielded[address] = now
                yield address
        finally:
            scanner, self._scanner = self._scanner, None
            if scanner is not None:
                with contextlib.suppress(Exception):
                    await scanner.stop()

    @asynccontextmanager
    async def connect(self, address: str):
        # The lock is what makes "one transaction at a time" true, which the scan
        # pause/resume below depends on. The runner is sequential today, so it's
        # uncontended — it's here so that stays safe if that ever changes.
        async with self._connect_lock:
            await self._pause_scanning()
            # Falls back to the address string if we've never seen this device advertise
            # (nothing in the sync loop does that today, but connect() is public).
            target = self._devices.get(address, address)
            try:
                async with self._client_factory(target, timeout=self._connect_timeout_s) as client:
                    if self._mtu_payload_size_override is not None:
                        payload_size = self._mtu_payload_size_override
                    else:
                        payload_size = await _resolve_write_size(client) - protocol.CHUNK_HEADER_LEN

                    if payload_size <= 0:
                        raise RuntimeError(
                            f"negotiated write size leaves no room for a chunk body "
                            f"(payload_size={payload_size})"
                        )

                    logger.debug("connected to %s, chunk body size %d", address, payload_size)
                    yield BleakConnection(client, payload_size)
            finally:
                # Suppress the just-completed transaction's own trailing advertisements:
                # firmware keeps advertising for the rest of its window after we
                # disconnect, and without this the next packet would immediately trigger
                # another pointless connection.
                self._last_yielded[address] = time.monotonic()
                await self._resume_scanning()

    async def _pause_scanning(self) -> None:
        if self._scanner is not None:
            with contextlib.suppress(Exception):
                await self._scanner.stop()

    async def _resume_scanning(self) -> None:
        if self._scanner is not None:
            try:
                await self._scanner.start()
            except Exception:
                # Don't take the process down: the caller is mid-teardown of a
                # transaction that may itself have failed. A scanner that won't restart
                # shows up as silence, which the runner's periodic refresh logging makes
                # visible.
                logger.exception("failed to resume scanning after a connection")
