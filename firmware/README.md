# Firmware (Section 7 build-order step 3)

**Target hardware:** Seeed Studio XIAO nRF52840 (XIAO BLE) + ePaper Driver Board for
XIAO + a 4.2" 400x300 monochrome eInk panel (SSD1683 controller — e.g. GDEY042T81 /
GDEY042T91).

**Status: built, flashed, and brought up on real hardware.** The first bring-up on an
actual XIAO nRF52840 + SSD1683 panel found four defects (stack overflow on identify,
missing advertising name, slow panel work blocking the BT callback context, and a
connection-object exhaustion after disconnect) — all fixed in `Fix firmware bugs found in
first real-hardware bring-up`. The register-level command sequence in
`src/epaper_ssd1683.c` was checked against the SSD1683 datasheet (Solomon Systech, Rev
1.0, Jan 2021), and the pin mapping in `boards/xiao_ble.overlay` against Seeed's
published pin table.

What has *not* been proven end to end is a full sync driven by the real gateway rather
than a phone BLE app — `gateway/README.md` walks through that.

## What's implemented

- `src/main.c` — the duty-cycle state machine (spec 4.1): DEEP_SLEEP → ADVERTISING →
  (SYNCING, driven by BLE callbacks) → DEEP_SLEEP, with a watchdog covering a stuck
  SYNCING phase.
- `src/ble_service.c/.h` — the custom GATT service (spec 4.2): `status`, `data_transfer`,
  `command` characteristics.
- `src/chunk_protocol.c/.h` — the chunk reassembly + CRC16 + target-hash unwrap, matching
  `gateway/gateway/protocol.py` byte-for-byte (see `docs/protocol.md`). Checked natively
  against Python-generated reference vectors — see `tests/`.
- `src/battery.c/.h` — ADC read on the XIAO nRF52840's VBAT divider (P0.31/AIN7, enabled
  via P0.14 — spec 4.5). The enable pin is held low permanently rather than toggled
  per-read; see the comment in `battery_init()` for the hardware risk that avoids.
- `src/epaper_ssd1683.c/.h` — the real SSD1683 panel driver: reset, init sequence, full
  refresh, identify. Command bytes are transcribed from the datasheet's command table;
  the one value that *isn't* pinned to the primary source is the border waveform
  constant (0x3C = 0x05), taken from common third-party reference implementations —
  flagged explicitly in that file since it's the one register value here I couldn't
  verify from the datasheet itself.
- `src/epaper.c/.h` — the integration layer between the chunk protocol and the panel
  driver. `epaper_apply_full` parses the flat binary layout payload and re-rasterizes;
  `epaper_apply_diff` patches the retained layout and re-rasterizes from it.
- `src/layout_store.c/.h` + `src/rasterizer.c/.h` + `src/font_basic.h` — the minimal v1
  rasterizer: retains the last-applied layout in RAM so diffs have something to patch,
  and draws it into a 1bpp framebuffer with a fixed-width bitmap font. Deliberately
  narrow (no word-wrap, no text styling, no partial refresh) — see `rasterizer.h`.
- `boards/xiao_ble.overlay` — SPI3 + GPIO wiring for CS/DC/RST/BUSY and the battery ADC,
  per the pin mapping above.

## What you'll need to fill in / verify

1. **Tune `APP_WAKE_INTERVAL_S` / `APP_ADVERTISING_WINDOW_MS`** (plain `#define`s at the
   top of `main.c`, not yet Kconfig options) once you have real battery-life data — spec
   8 leaves this open on purpose. For gateway bring-up, drop the wake interval from 120 s
   to ~15 s first: content takes two wake cycles to converge (docs/protocol.md), so at
   the stock interval every test iteration costs about four minutes.
2. **Verify the VBAT divider ratio** (`VBAT_DIVIDER_RATIO` in `battery.c`, currently 3)
   against a real battery and multimeter — the ~1/3 figure is what's documented for this
   board, not something measured on this specific unit.

## Build

```bash
west build -b xiao_ble firmware
west flash
```

VS Code tasks for both are in `.vscode/tasks.json`.

## Why content_hash is a value from the wire, not a computed one

Firmware never computes `content_hash` itself — it adopts the 4-byte value prefixed onto
every `data_transfer` message by the gateway. See docs/protocol.md, "content_hash for
diff updates," for why: firmware can't reproduce the backend's canonical-JSON CRC32 from
a diff patch alone without retaining a full copy of the layout tree, so the backend just
tells it what to adopt, protected by the same CRC16 check as the rest of the message.
