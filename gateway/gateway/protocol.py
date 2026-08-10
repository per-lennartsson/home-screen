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
        # Latin-1, not UTF-8 — same reasoning as encode_full_layout: firmware treats
        # every byte of `value` as one rendered glyph column (layout_store.c copies it
        # verbatim, rasterizer.c draws text_len bytes 1:1), so a multi-byte UTF-8
        # sequence would misrender as extra garbled columns, not just fail to show the
        # one character. This is the path a live Home Assistant value (e.g. "21.4°C")
        # actually updates through — encode_full_layout alone wasn't enough to fix the
        # degree sign for a value that updates via diff rather than a fresh full push.
        value_bytes = value.encode("latin-1", errors="replace")
        if not (0 <= element_id <= 255):
            raise ValueError(f"element_id {element_id} out of uint8 range")
        if len(value_bytes) > 255:
            raise ValueError(f"value for element {element_id} exceeds 255 bytes")
        out += bytes([element_id, len(value_bytes)]) + value_bytes
    return out


FULL_LAYOUT_FORMAT_VERSION = 1
LAYOUT_MAX_ELEMENTS = 16
LAYOUT_MAX_TEXT_LEN = 32


def encode_full_layout(layout: dict) -> bytes:
    """Flat binary encoding of a resolved layout for a 0x01 (full) data_transfer message.

    Firmware has no JSON library (docs/protocol.md's "full payload format" note already
    calls the JSON shape a placeholder pending real rendering — this is that moment), so
    this is the wire format it actually parses: a 1-byte format version, a 1-byte element
    count, then one fixed-size record per element:
      byte 0   : element_id (uint8)
      byte 1-2 : x (uint16 LE)
      byte 3-4 : y (uint16 LE)
      byte 5   : flags — bit0 checkable, bit1 checked
      byte 6   : text_len (uint8)
      byte 7.. : text (Latin-1, not null-terminated — see below)
    Capped at LAYOUT_MAX_ELEMENTS/LAYOUT_MAX_TEXT_LEN to match firmware's fixed-size
    in-RAM layout store — same sizing style as CHUNK_MAX_DIFF_ENTRIES above.
    """
    elements = layout.get("elements", [])[:LAYOUT_MAX_ELEMENTS]
    out = bytes([FULL_LAYOUT_FORMAT_VERSION, len(elements)])
    for element in elements:
        props = element.get("props", {})
        checkable = element.get("type") == "value" and props.get("source") == "button"
        checked = bool(props.get("checked", False)) if checkable else False
        text = props.get("text") if element.get("type") == "text" else props.get("value")
        # Latin-1, not ASCII: firmware's rasterizer keeps one byte == one glyph column
        # (font_basic.h has no multi-byte/UTF-8 handling), and Latin-1's code points
        # 0-255 map 1:1 onto Unicode's, so this is the widest single-byte encoding that
        # still fits that assumption. In practice it only buys one extra character over
        # ASCII — 0xB0, the degree sign, which firmware has a dedicated glyph for
        # (font_basic.h's font_glyph_degree) — everything else outside ASCII still isn't
        # in the font and falls back to errors="replace"'s "?" like before.
        text_bytes = (text or "").encode("latin-1", errors="replace")[:LAYOUT_MAX_TEXT_LEN]

        element_id = element["id"]
        if not (0 <= element_id <= 255):
            raise ValueError(f"element_id {element_id} out of uint8 range")
        flags = (1 if checkable else 0) | (2 if checked else 0)

        out += bytes([element_id])
        out += int(element.get("x", 0)).to_bytes(2, "little")
        out += int(element.get("y", 0)).to_bytes(2, "little")
        out += bytes([flags, len(text_bytes)]) + text_bytes
    return out


def decode_full_layout(data: bytes) -> dict:
    """Inverse of encode_full_layout — what firmware's layout_store_apply_full does
    on-device. Used by MockDisplay (ble_transport.py) so gateway tests exercise the real
    wire format instead of assuming the original JSON layout survives the trip."""
    if len(data) < 2:
        raise ValueError("full layout payload too short")
    if data[0] != FULL_LAYOUT_FORMAT_VERSION:
        raise ValueError(f"unsupported full layout format version {data[0]}")

    count = data[1]
    offset = 2
    elements = []
    for _ in range(count):
        element_id = data[offset]
        x = int.from_bytes(data[offset + 1 : offset + 3], "little")
        y = int.from_bytes(data[offset + 3 : offset + 5], "little")
        flags = data[offset + 5]
        text_len = data[offset + 6]
        text = data[offset + 7 : offset + 7 + text_len].decode("latin-1")
        offset += 7 + text_len
        elements.append(
            {
                "id": element_id,
                "x": x,
                "y": y,
                "checkable": bool(flags & 1),
                "checked": bool(flags & 2),
                "text": text,
            }
        )
    return {"elements": elements}


def decode_diff(payload: bytes) -> dict[int, str]:
    count = payload[0]
    values: dict[int, str] = {}
    offset = 1
    for _ in range(count):
        element_id = payload[offset]
        length = payload[offset + 1]
        value = payload[offset + 2 : offset + 2 + length].decode("latin-1")
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
