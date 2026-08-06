"""
BLE access sits behind this interface so the library underneath (bleak/BlueZ) can be
swapped without touching sync.py — flagged in spec 5 as a likely source of instability
in the previous build. `MockBleTransport` is the only implementation for now (Section 7
step 2: prove the sync loop before real firmware exists). A `BleakTransport` implementing
the same interface against real hardware is step 3's job, once firmware exists to test
it against — writing it earlier would be untestable, unverified code.
"""

from __future__ import annotations

import asyncio
import json
from abc import ABC, abstractmethod
from collections.abc import AsyncIterator
from contextlib import asynccontextmanager
from typing import AsyncContextManager

from gateway import protocol


class BleConnection(ABC):
    """One connected, about-to-disconnect BLE session with a single display."""

    @abstractmethod
    async def read_status(self) -> dict:
        """Read the `status` characteristic: {content_hash, battery_pct, battery_mv}."""

    @abstractmethod
    async def write_data_transfer(self, chunk: bytes) -> None:
        """Write one chunk to the `data_transfer` characteristic."""

    @property
    @abstractmethod
    def mtu_payload_size(self) -> int:
        """Usable payload bytes per data_transfer chunk after the protocol header,
        given this connection's negotiated ATT MTU."""


class BleTransport(ABC):
    @abstractmethod
    def scan(self) -> AsyncIterator[str]:
        """Yield the MAC address of a display each time its advertisement is seen."""

    @abstractmethod
    def connect(self, mac_address: str) -> AsyncContextManager[BleConnection]:
        """Connect to a display for one short transaction; always disconnects on exit."""


class MockDisplay:
    """A fake NRF52840 peripheral: applies chunk protocol messages exactly as firmware
    is specified to (spec 4.3), so a passing sync against this proves the protocol, not
    just the HTTP plumbing."""

    def __init__(self, mac_address: str, battery_pct: int = 90, battery_mv: int = 3000):
        self.mac_address = mac_address
        self.content_hash = 0
        self.battery_pct = battery_pct
        self.battery_mv = battery_mv
        self.last_rendered: dict | None = None
        self._reassembler = protocol.ChunkReassembler()

        # Test hooks for proving retry/backoff behavior (spec 5.1 step 8).
        self.fail_next_connects = 0
        self.corrupt_next_write = False

    def status(self) -> dict:
        return {
            "content_hash": self.content_hash,
            "battery_pct": self.battery_pct,
            "battery_mv": self.battery_mv,
        }

    def apply_chunk(self, chunk: bytes) -> None:
        if self.corrupt_next_write:
            chunk = chunk[:-1] + bytes([chunk[-1] ^ 0xFF])
            self.corrupt_next_write = False

        result = self._reassembler.feed(chunk)
        if result is None:
            return  # incomplete, or CRC mismatch -> discarded silently, per spec 4.3

        msg_type, wrapped_payload = result
        target_hash, payload = protocol.unwrap_payload_with_target_hash(wrapped_payload)

        if msg_type == protocol.MSG_TYPE_FULL:
            self.last_rendered = json.loads(payload)
        elif msg_type == protocol.MSG_TYPE_DIFF:
            if self.last_rendered is None:
                return  # can't apply a diff with nothing rendered yet
            values = protocol.decode_diff(payload)
            for element in self.last_rendered.get("elements", []):
                if element.get("id") in values:
                    element.setdefault("props", {})["value"] = values[element["id"]]

        # content_hash is adopted from the backend, not re-derived — see
        # wrap_payload_with_target_hash for why. last_rendered is kept only so tests
        # (and, on real firmware, whatever the panel is displaying) can be inspected.
        self.content_hash = target_hash


class _MockConnection(BleConnection):
    def __init__(self, display: MockDisplay, mtu_payload_size: int):
        self._display = display
        self._mtu_payload_size = mtu_payload_size

    async def read_status(self) -> dict:
        return self._display.status()

    async def write_data_transfer(self, chunk: bytes) -> None:
        self._display.apply_chunk(chunk)

    @property
    def mtu_payload_size(self) -> int:
        return self._mtu_payload_size


class MockBleTransport(BleTransport):
    def __init__(self, displays: list[MockDisplay], advertise_interval: float = 1.0, mtu_payload_size: int = 200):
        self._displays = {d.mac_address: d for d in displays}
        self._advertise_interval = advertise_interval
        self._mtu_payload_size = mtu_payload_size

    async def scan(self) -> AsyncIterator[str]:
        while True:
            for mac in list(self._displays):
                yield mac
            await asyncio.sleep(self._advertise_interval)

    @asynccontextmanager
    async def connect(self, mac_address: str):
        display = self._displays[mac_address]
        if display.fail_next_connects > 0:
            display.fail_next_connects -= 1
            raise ConnectionError(f"simulated connection failure for {mac_address}")

        await asyncio.sleep(0.01)  # simulate connection setup latency
        yield _MockConnection(display, self._mtu_payload_size)
