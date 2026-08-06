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
