# Firmware (Section 7 build-order step 3)

**Target hardware:** Seeed Studio XIAO nRF52840 (XIAO BLE) + ePaper Driver Board for
XIAO + a 4.2" 400x300 monochrome eInk panel (SSD1683 controller — e.g. GDEY042T81 /
GDEY042T91).

**Status: written, not built or flashed.** There's no nRF Connect SDK toolchain or real
hardware in the environment this was written in, so this compiles against my
understanding of the Zephyr/nRF Connect SDK APIs but hasn't been verified with `west
build`. The register-level command sequence in `src/epaper_ssd1683.c` was checked
against the actual SSD1683 datasheet (Solomon Systech, Rev 1.0, Jan 2021), and the pin
mapping in `boards/xiao_ble.overlay` against Seeed's published pin table and the
standard XIAO nRF52840 D-pin mapping — so the facts behind this are real, but the code
itself is still an untested first draft. Budget time to fix compile errors and Kconfig
drift against whatever NCS version you actually use.

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
  driver. **Content rasterization doesn't exist yet** (Section 7 build-order step 4):
  `epaper_apply_full` pushes a blank (all-white) frame through the real hardware path
  rather than your dashboard's actual content, which still proves the full sync
  pipeline end to end on real hardware — SPI/GPIO timing, chunk reassembly, hash
  handling, GATT — just not the rendering itself. `epaper_apply_diff` decodes and logs
  value changes but has nothing to draw them onto without a rasterizer/layout tracker.
- `boards/xiao_ble.overlay` — SPI3 + GPIO wiring for CS/DC/RST/BUSY and the battery ADC,
  per the pin mapping above.

## What you'll need to fill in / verify

1. **Write a text/value rasterizer** (JSON `layout_json` → 400x300 1bpp framebuffer,
   probably needing a small bitmap font) to make `epaper_apply_full`/`epaper_apply_diff`
   draw real content instead of a blank frame. This is genuinely separate, sizeable work
   — Section 7's own build order treats it as step 4, after this step.
2. **Generate a real 128-bit base UUID** (`uuidgen` or similar) and replace the
   placeholder in `ble_service.h` — spec 4.2 explicitly calls the given UUIDs a
   placeholder pattern.
3. **Verify the Kconfig options in `prj.conf`** and the devicetree overlay's SPI/pinctrl
   node names against your actual NCS/Zephyr version — both have moved before, and the
   overlay's `epd` node uses a placeholder `compatible` string that may need a matching
   `bindings/*.yaml` file depending on your toolchain's strictness (see the comment in
   the overlay).
4. **Tune `CONFIG_APP_WAKE_INTERVAL_S` / `CONFIG_APP_ADVERTISING_WINDOW_MS`** (defined at
   the top of `main.c` as plain `#define`s for now, not yet Kconfig options) once you
   have real battery-life data — spec 8 leaves this open on purpose.
5. **Verify the VBAT divider ratio** (`VBAT_DIVIDER_RATIO` in `battery.c`, currently 3)
   against a real battery and multimeter — the ~1/3 figure is what's documented for this
   board, not something measured on this specific unit.

## Build (once the above is done)

```bash
west build -b xiao_ble firmware
west flash
```

## Why content_hash is a value from the wire, not a computed one

Firmware never computes `content_hash` itself — it adopts the 4-byte value prefixed onto
every `data_transfer` message by the gateway. See docs/protocol.md, "content_hash for
diff updates," for why: firmware can't reproduce the backend's canonical-JSON CRC32 from
a diff patch alone without retaining a full copy of the layout tree, so the backend just
tells it what to adopt, protected by the same CRC16 check as the rest of the message.
