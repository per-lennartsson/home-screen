from gateway import protocol


def test_chunk_round_trip_single_chunk():
    payload = b'{"hello":"world"}'
    chunks = protocol.encode_chunks(protocol.MSG_TYPE_FULL, payload, mtu_payload_size=200)
    assert len(chunks) == 1

    reassembler = protocol.ChunkReassembler()
    result = reassembler.feed(chunks[0])
    assert result == (protocol.MSG_TYPE_FULL, payload)


def test_chunk_round_trip_multi_chunk_out_of_order():
    payload = bytes(range(256)) * 3  # 768 bytes, forces multiple chunks at a small MTU
    chunks = protocol.encode_chunks(protocol.MSG_TYPE_DIFF, payload, mtu_payload_size=50)
    assert len(chunks) > 1

    reassembler = protocol.ChunkReassembler()
    result = None
    for chunk in reversed(chunks):  # firmware buffers by index, order shouldn't matter
        result = reassembler.feed(chunk)
    assert result == (protocol.MSG_TYPE_DIFF, payload)


def test_corrupted_chunk_is_discarded_not_raised():
    payload = b"some payload bytes"
    chunks = protocol.encode_chunks(protocol.MSG_TYPE_FULL, payload, mtu_payload_size=200)
    corrupted = chunks[0][:-1] + bytes([chunks[0][-1] ^ 0xFF])

    reassembler = protocol.ChunkReassembler()
    result = reassembler.feed(corrupted)
    assert result is None  # silently discarded per spec 4.3, not an exception


def test_target_hash_wrap_unwrap_round_trip():
    data = b"some diff or full payload bytes"
    wrapped = protocol.wrap_payload_with_target_hash(0xDEADBEEF, data)
    target_hash, unwrapped = protocol.unwrap_payload_with_target_hash(wrapped)
    assert target_hash == 0xDEADBEEF
    assert unwrapped == data


def test_diff_encode_decode_round_trip():
    values = {1: "21.5C", 2: "88%"}
    encoded = protocol.encode_diff(values)
    assert protocol.decode_diff(encoded) == {1: "21.5C", 2: "88%"}


def test_content_hash_matches_backend_canonical_encoding():
    # This must stay identical to backend/app/services/hashing.py's canonical_json_bytes
    # + crc32 combination, or the gateway and backend will disagree on content_hash.
    layout = {"elements": [{"id": 2, "type": "value", "props": {"value": "1"}}]}
    a = protocol.content_hash_of(protocol.canonical_json_bytes(layout))
    b = protocol.content_hash_of(protocol.canonical_json_bytes({"elements": [{"props": {"value": "1"}, "type": "value", "id": 2}]}))
    assert a == b  # key order must not affect the hash


def _element(element_id, **props):
    """A design-editor element as toLayoutJson (frontend/src/lib/layout.js) emits it."""
    base = {"id": element_id, "type": "text", "x": 0, "y": 0, "w": 100, "h": 20}
    base["props"] = {"text": "x", **props}
    return base


def test_full_layout_v3_byte_layout_is_pinned():
    # The exact bytes firmware/src/layout_store.c parses. Pinned literally rather than
    # round-tripped, because a round-trip through this module's own decoder would still
    # pass if both halves drifted together -- and layout_store.c is the other half that
    # cannot be imported here.
    layout = {
        "elements": [
            {
                "id": 7,
                "type": "text",
                "x": 0x0102,
                "y": 0x0304,
                "w": 0x0506,
                "h": 20,
                "props": {"text": "Hi", "fontSize": 24, "bold": True, "align": "right", "underline": True},
            }
        ]
    }
    encoded = protocol.encode_full_layout(layout)

    assert encoded[0] == 3, "format version"
    assert encoded[1] == 1, "element count"
    assert encoded[2] == 7, "element_id"
    assert encoded[3:5] == b"\x02\x01", "x, uint16 LE"
    assert encoded[5:7] == b"\x04\x03", "y, uint16 LE"
    assert encoded[7:9] == b"\x06\x05", "w, uint16 LE -- new in v3, required for align"
    # underline (bit2) + align right (2 << 4) == 0x24. No bold bit: weight is in font_id.
    assert encoded[9] == 0x24, "flags"
    # 24px is index 3 in the ladder; +6 for semibold.
    assert encoded[10] == 9, "font_id encodes size AND weight"
    assert encoded[11] == 2, "text_len"
    assert encoded[12:] == b"Hi"


def test_font_id_round_trips_size_and_weight():
    for size in protocol.FONT_SIZES:
        for bold in (False, True):
            font_id = protocol._font_id_for(size, bold)
            assert protocol.font_id_to_size_and_weight(font_id) == (size, bold)


def test_off_ladder_font_size_snaps_to_nearest():
    # Designs saved before the ladder existed can hold any px value; the device has no
    # font for one, so the gateway must snap rather than emit an unrenderable id.
    assert protocol.font_id_to_size_and_weight(protocol._font_id_for(18, False))[0] == 16
    assert protocol.font_id_to_size_and_weight(protocol._font_id_for(64, False))[0] == 48
    assert protocol.font_id_to_size_and_weight(protocol._font_id_for(1, False))[0] == 12


def test_out_of_range_font_id_falls_back_like_firmware():
    # Mirrors hs_font_get()'s fallback in firmware/src/fonts/hs_fonts.c.
    assert protocol.font_id_to_size_and_weight(200) == (protocol.DEFAULT_FONT_SIZE_PX, False)


def test_full_layout_v3_carries_every_style_the_editor_offers():
    # Format 2 silently dropped bold/align/underline/strikethrough even though the design
    # editor has always stored them (frontend/src/lib/layout.js). Guard against a
    # regression to that.
    layout = {
        "elements": [
            _element(1, align="center"),
            _element(2, underline=True),
            _element(3, strikethrough=True),
            _element(4, bold=True),
        ]
    }
    decoded = protocol.decode_full_layout(protocol.encode_full_layout(layout))["elements"]

    assert decoded[0]["align"] == protocol.ALIGN_CENTER
    assert decoded[1]["underline"] is True
    assert decoded[2]["strikethrough"] is True
    assert decoded[3]["bold"] is True
