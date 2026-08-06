"""
Wire protocol shared with firmware: the chunk format on `data_transfer` (spec 4.3) and
the diff payload TLV format pinned in docs/protocol.md. Firmware must implement the
receiving half of this exact format.
"""

from __future__ import annotations

import json
import zlib
from dataclasses import dataclass, field

MSG_TYPE_FULL = 0x01
MSG_TYPE_DIFF = 0x02

CHUNK_HEADER_LEN = 7  # type(1) + chunk_index(2) + total_chunks(2) + crc16(2)
TARGET_HASH_LEN = 4  # uint32 prefix inside the reassembled payload — see wrap/unwrap below

# CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) — not specified in the original spec,
# pinned here since gateway and firmware must agree on one variant for chunk integrity.
_CRC16_POLY = 0x1021
_CRC16_INIT = 0xFFFF


def crc16_ccitt_false(data: bytes) -> int:
    crc = _CRC16_INIT
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ _CRC16_POLY) & 0xFFFF if (crc & 0x8000) else (crc << 1) & 0xFFFF
    return crc


def canonical_json_bytes(obj: dict) -> bytes:
    return json.dumps(obj, sort_keys=True, separators=(",", ":")).encode("utf-8")


def content_hash_of(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


def wrap_payload_with_target_hash(target_content_hash: int, data: bytes) -> bytes:
    """Prefix a data_transfer payload with the content_hash firmware should adopt once
    CRC16 confirms the message arrived intact.

    Firmware cannot derive this value itself for a diff message — it only receives a
    small value patch, not the full layout the backend hashed to produce
    `desired_content_hash` (see docs/protocol.md, "content_hash for diff updates"). So
    the backend computes it and the wire format carries it explicitly; the existing
    CRC16 chunk-integrity check (spec 4.3) protects these 4 bytes exactly like the rest
    of the payload — no separate integrity mechanism needed.
    """
    return target_content_hash.to_bytes(TARGET_HASH_LEN, "little") + data


def unwrap_payload_with_target_hash(payload: bytes) -> tuple[int, bytes]:
    target_content_hash = int.from_bytes(payload[:TARGET_HASH_LEN], "little")
    return target_content_hash, payload[TARGET_HASH_LEN:]


def encode_chunks(msg_type: int, payload: bytes, mtu_payload_size: int) -> list[bytes]:
    """Split `payload` into data_transfer chunks per spec 4.3."""
    if mtu_payload_size <= 0:
        raise ValueError("mtu_payload_size must be positive")

    crc = crc16_ccitt_false(payload)
    body_chunk_size = mtu_payload_size
    raw_chunks = [payload[i : i + body_chunk_size] for i in range(0, len(payload), body_chunk_size)] or [b""]
    total = len(raw_chunks)

    chunks = []
    for index, body in enumerate(raw_chunks):
        header = (
            msg_type.to_bytes(1, "little")
            + index.to_bytes(2, "little")
            + total.to_bytes(2, "little")
            + crc.to_bytes(2, "little")
        )
        chunks.append(header + body)
    return chunks


def encode_diff(values: dict[int, str]) -> bytes:
    """TLV encoding for a 0x02 diff message payload (docs/protocol.md)."""
    if len(values) > 255:
        raise ValueError("diff payload supports at most 255 value updates")
    out = bytes([len(values)])
    for element_id, value in values.items():
        value_bytes = value.encode("utf-8")
        if not (0 <= element_id <= 255):
            raise ValueError(f"element_id {element_id} out of uint8 range")
        if len(value_bytes) > 255:
            raise ValueError(f"value for element {element_id} exceeds 255 bytes")
        out += bytes([element_id, len(value_bytes)]) + value_bytes
    return out


def decode_diff(payload: bytes) -> dict[int, str]:
    count = payload[0]
    values: dict[int, str] = {}
    offset = 1
    for _ in range(count):
        element_id = payload[offset]
        length = payload[offset + 1]
        value = payload[offset + 2 : offset + 2 + length].decode("utf-8")
        values[element_id] = value
        offset += 2 + length
    return values


@dataclass
class ChunkReassembler:
    """Mirrors the buffering firmware does on `data_transfer` writes (spec 4.3)."""

    _chunks: dict[int, bytes] = field(default_factory=dict)
    _total: int | None = None
    _expected_crc: int | None = None
    _msg_type: int | None = None

    def reset(self) -> None:
        self._chunks.clear()
        self._total = None
        self._expected_crc = None
        self._msg_type = None

    def feed(self, chunk: bytes) -> tuple[int, bytes] | None:
        """Buffer one chunk. Returns (msg_type, payload) once the full message has been
        received AND its CRC16 matches — otherwise None, exactly like firmware discarding
        silently on mismatch (spec 4.3: 'the gateway will see the stale content_hash on
        its next check-in and retry')."""
        msg_type = chunk[0]
        index = int.from_bytes(chunk[1:3], "little")
        total = int.from_bytes(chunk[3:5], "little")
        crc = int.from_bytes(chunk[5:7], "little")
        body = chunk[CHUNK_HEADER_LEN:]

        if self._total is not None and (total != self._total or crc != self._expected_crc or msg_type != self._msg_type):
            # A new message started before the previous one finished — start fresh.
            self.reset()

        self._total = total
        self._expected_crc = crc
        self._msg_type = msg_type
        self._chunks[index] = body

        if len(self._chunks) < self._total:
            return None

        payload = b"".join(self._chunks[i] for i in range(self._total))
        result = None
        if crc16_ccitt_false(payload) == self._expected_crc:
            result = (self._msg_type, payload)
        self.reset()
        return result
