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
  bytes 2..N : value (Latin-1, not null-terminated — firmware draws one glyph column
               per byte, so this must match the full payload's encoding; see below)
```

The gateway translates the JSON diff payload from `/api/displays/{id}/payload` into this
binary form before chunking it per spec 4.3. This requires design elements to keep
stable small-integer IDs (already true — see `layout_json.elements[].id`), and a cap of
255 concurrently diffable elements per design, which is generous for an ePaper status
display. Revisit if that ever becomes a real constraint.

## GATT layout: UUIDs and the `status` value encoding

Both halves of this are duplicated between `firmware/src/ble_service.c` and
`gateway/gateway/uuids.py` / `gateway/gateway/bleak_transport.py`, so they belong here.

**UUIDs.** A real generated base, replacing the placeholder pattern spec 4.2 shipped
with: service `8fd9daef-dd0f-4243-85e1-f9b453750000`, then `...0001` status, `...0002`
data_transfer, `...0003` command, `...0004` button_event. The service UUID is what the
gateway filters advertisements on; the device name (`HomeScreen Display`) rides in the
scan response instead of the primary packet, since the 128-bit UUID already fills the
primary packet's 31-byte budget. Regenerating the base means changing both files in the
same commit and reflashing.

**`status` value** — `struct status_value`, `__packed`, little-endian, 9 bytes as of
fw_version 1, 10 as of fw_version 2:

```
bytes 0-3 : content_hash  (uint32)
byte  4   : battery_pct   (uint8)
bytes 5-6 : battery_mv    (uint16)
bytes 7-8 : fw_version    (uint16)
byte  9   : charging      (uint8, 0/1 — fw_version 2+ only)
```

The `__packed` matters: without it the compiler would pad `battery_mv` to a 2-byte
boundary and every field after `content_hash` would shift by one. The gateway parses
the first 9 bytes unconditionally and ignores anything after them unless present, so
firmware may append fields without breaking an older gateway — `charging` is the first
field to actually use that: `bleak_transport.py`'s `read_status()` treats a 9-byte read
(fw_version 1, no charger-IC GPIO wired up yet) as `charging: None` — "unknown," not
"not charging" — rather than requiring the 10th byte. `charging` is read straight off
the BQ25101 charger IC's status pin (`firmware/src/battery.c`), not derived from
`battery_mv`; the backend's own voltage-trend inference
(`app/services/battery.py`'s `charging_flags`) is the fallback for `None`.

**`button_event` value** — a single byte, bit *i* = checklist row *i*'s button pressed
since the last read. Reading it clears the mask firmware-side, so the gateway must read
it exactly once per connection (`sync.py` does this before fetching the payload, so a
press made just before a wake shows up in the same cycle's content).

**`data_transfer`** is write-with-response only (no WRITE_WITHOUT_RESP property). That's
deliberate rather than incidental: firmware's reassembler requires chunks in strictly
ascending index order (`firmware/src/chunk_protocol.h`), which write-with-response
guarantees by making the gateway wait for each ack.

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

The backend returns the canonical JSON snapshot of `layout_json` as `data` (backend →
gateway, HTTP). The gateway translates that into the flat binary encoding firmware
actually parses before chunking it — `encode_full_layout` in
`gateway/gateway/protocol.py`, mirrored by `layout_store_apply_full` in
`firmware/src/layout_store.c`:

```
byte 0    : format version (currently 3)
byte 1    : element count (uint8)
repeated per element:
  byte 0    : element_id (uint8)
  bytes 1-2 : x (uint16 LE)
  bytes 3-4 : y (uint16 LE)
  bytes 5-6 : w (uint16 LE) — the element's box width. Added in format 3: `align` below
              cannot be resolved without knowing the box being aligned within, and
              before this the device had no idea how wide an element was.
  byte 7    : flags — bit0 checkable, bit1 checked, bit2 underline, bit3 strikethrough,
              bits4-5 alignment (0 left, 1 center, 2 right). Note there is no bold bit;
              weight lives in font_id below.
  byte 8    : font_id (uint8) — index into the shared font ladder: the size's index in
              {12, 16, 20, 24, 32, 48}, plus 6 for semibold. Added in format 3,
              replacing format 2's font_scale.
  byte 9    : text_len (uint8)
  bytes 10..: text (Latin-1, not null-terminated — one byte per glyph. The generated
              fonts cover 0x20-0x7E and all of 0xA0-0xFF, so every byte the encoding can
              produce is renderable; bytes outside Latin-1 are replaced with "?" at
              encode time.)
```

### Why font_id and not a pixel size

The device can only render sizes it has a generated font for. Those fonts are produced by
`tools/fonts/generate.mjs` from a single TTF, which in the same pass emits the glyph
metrics the design editor measures with (`frontend/src/lib/font_metrics.json`) — that
shared origin is what makes the editor's preview match the panel instead of approximating
it. Naming a font by index keeps the set closed: there is no way to author a size the
device cannot faithfully draw.

The three places that define the ladder must be changed together: `SIZES` in
`tools/fonts/generate.mjs`, `hs_font_sizes[]` in `firmware/src/fonts/hs_fonts.c`, and
`FONT_SIZES` in `gateway/gateway/protocol.py`.

Format 2 is not accepted by a firmware build that speaks format 3 — the per-element
record is a different length, so misparsing it would produce garbage rather than degraded
output. Since the gateway and firmware are deployed together and the gateway re-pushes a
full layout whenever content differs, a rejected payload costs one wake cycle, not a
stuck display.

JSON stops at the gateway because firmware has no JSON parser. Capped at 16 elements and
32 text bytes each, matching firmware's fixed-size in-RAM layout store; anything beyond
those caps is truncated gateway-side rather than being rejected on-device.

Rasterizing that into pixels is firmware's job (`src/rasterizer.c`), which renders through
LVGL using the generated fonts in `firmware/src/fonts/`. Size, weight, left/center/right
alignment, underline and strikethrough are all drawn. Word wrap is not: text is clipped to
the element's box, matching the design editor, which does not wrap either.

One conversion happens at this boundary worth knowing about: the wire carries Latin-1, one
byte per glyph, but LVGL decodes label text as UTF-8. `rasterizer.c` transcodes before
handing text to LVGL — without it every byte ≥ 0x80 (Å/Ä/Ö/å/ä/ö and the degree sign)
would be an invalid UTF-8 lead byte.
