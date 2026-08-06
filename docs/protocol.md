# Cross-component protocol decisions

Decisions pinned while building the backend skeleton (Section 7 step 1) that firmware
and gateway work will need to match. Spec references are to the architecture doc.

## content_hash algorithm

CRC32, matching the `uint32` `content_hash` field in the `status` GATT characteristic
(spec 4.2). Computed backend-side as `zlib.crc32` over the canonical JSON encoding of a
design's `layout_json` (sorted keys, no whitespace) — see
`backend/app/services/hashing.py` and `rendering.py`.

## content_hash for diff updates — firmware cannot derive it, so the wire carries it

Originally this doc said firmware "computes CRC32 over the reassembled content it
applied," matching backend's canonical-JSON hash. That's true for a `full` message —
firmware receives the exact bytes backend hashed, so it can crc32 them directly. It's
**not true for a `diff` message**: firmware only receives a small value patch (element
IDs + new values), not the full layout, so it has no way to reproduce backend's
canonical-JSON CRC32 without also retaining a full copy of the layout tree and
re-serializing it identically to the backend on every diff — real, avoidable state and
complexity for firmware to carry just to reproduce a number the backend already knows.

Fixed by having the backend tell firmware what hash to adopt, instead of asking firmware
to derive it: `wrap_payload_with_target_hash` (`gateway/gateway/protocol.py`) prefixes
the chunked payload with the target `content_hash` (uint32, little-endian) before
`data`. This applies to *every* message, `full` and `diff` alike, for consistency.
Firmware doesn't compute anything — once CRC16 confirms the reassembled message arrived
intact (spec 4.3, unchanged), it applies `data` and sets `content_hash` to the 4-byte
prefix. No separate integrity check is needed for those 4 bytes; they're covered by the
same CRC16 as the rest of the message. Proven in
`gateway/tests/test_sync_mock.py::test_value_only_change_pushes_diff_not_full`.

Wire shape: the "payload" referred to in spec 4.3 (the thing chunked and CRC16'd) is now
`content_hash(4 bytes LE) + data`, where `data` is the canonical JSON for `full` or the
TLV diff for `diff` (below). The 7-byte chunk header itself is unchanged.

## Diff vs. full decision

A display is diff-eligible only when its `current_content_hash` and the desired hash
share the same **structure signature** — CRC32 of the layout with all `value`-type
elements' bound values stripped out. If the structure changed (elements added/removed/
moved/resized) or the display's current hash isn't cached, the backend always returns a
`full` payload. See `render_and_cache` / `build_value_diff` in
`backend/app/services/rendering.py`.

## Diff payload wire format (backend → gateway, JSON)

```json
{
  "type": "diff",
  "content_hash": 576286661,
  "data": { "values": { "2": "22.1C" } }
}
```

`data.values` maps `element_id` (string, since JSON object keys are always strings) to
the element's new bound value.

## Diff payload wire format (gateway → firmware, `data_transfer` 0x02 chunks)

Implemented in `gateway/gateway/protocol.py` (`encode_diff`/`decode_diff`), proven
against `MockDisplay` in `gateway/tests/test_sync_mock.py`:

```
byte 0       : number of value updates (uint8)
repeated per update:
  byte 0     : element_id (uint8)
  byte 1     : value length (uint8)
  bytes 2..N : value (UTF-8, not null-terminated)
```

The gateway translates the JSON diff payload from `/api/displays/{id}/payload` into this
binary form before chunking it per spec 4.3. This requires design elements to keep
stable small-integer IDs (already true — see `layout_json.elements[].id`), and a cap of
255 concurrently diffable elements per design, which is generous for an ePaper status
display. Revisit if that ever becomes a real constraint.

## CRC16 variant for chunk integrity (spec 4.3)

Spec 4.3 specifies a CRC16 field but not which variant. Pinned to **CRC-16/CCITT-FALSE**
(poly `0x1021`, init `0xFFFF`) — a common, simple choice with no lookup-table dependency,
implemented in `gateway/gateway/protocol.py::crc16_ccitt_false`. Firmware must use the
exact same variant or every chunked write will appear corrupted.

## Sync latency: convergence takes at least two wake cycles, not one

Surfaced while writing the gateway integration tests (`gateway/tests/test_sync_mock.py`).
Spec 5.1's loop has no step where the gateway re-reads `status` after pushing a payload
to confirm it landed — each connection is a single, short, self-contained transaction
(spec 3). So the sequence is:

- **Cycle N**: gateway reads the (stale) status, reports it, gets a payload back (since
  not yet in sync), and pushes it. The display now has the new content, but the backend
  still shows `current_content_hash` from before the push — it hasn't been told yet.
- **Cycle N+1**: gateway reads status again, now reflecting the push, and reports it.
  Only now does the backend see `current_content_hash == desired_content_hash`.

A push that silently fails CRC (spec 4.3) costs one extra cycle: the stale status gets
re-reported, the backend hands back the same payload again, and the gateway retries the
push — three cycles total to converge instead of two. This is consistent with spec 3's
"update latency is bounded by the wake interval" and doesn't need any special-casing in
`GatewayService` — it falls out naturally from always checking in when due and always
pushing whatever the backend currently says is pending.

## Full payload format

Currently the backend returns the canonical JSON snapshot of `layout_json` as `data`.
This is a placeholder — real ePaper bitmap rendering (converting `layout_json` into
actual pixel data for the panel's controller chip) is Section 7 build-order step 4 and
hasn't been built yet. The `full` payload's `data` field will change shape once that
exists; `type: "full"` and `content_hash` are stable.
