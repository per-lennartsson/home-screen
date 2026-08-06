import json
import zlib

# CRC32 is the content_hash algorithm shared with firmware (see docs/protocol.md) —
# cheap on the NRF52840, fits the uint32 status characteristic, and reuses the same
# family of checksum already used for chunk integrity in the data_transfer protocol.


def crc32_of(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


def canonical_json_bytes(obj: dict) -> bytes:
    return json.dumps(obj, sort_keys=True, separators=(",", ":")).encode("utf-8")
