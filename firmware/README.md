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

Note that published pin table proved wrong in at least one place: it puts the panel's
BUSY line on D2, and it is actually on **D5**, confirmed against the physical board. That
mistake was invisible for a long time because `epd_wait_busy()` also had a bug that made
it return immediately, so a BUSY line that never asserted looked exactly like one that was
already idle. Both are fixed; the remaining unverified entries in that table should be
treated with suspicion. Fixing it also forced row 1's checklist button off D5 onto D2 —
a soldering change, not just a devicetree one.

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
  per-read; see the comment in `battery_init()` for the hardware risk that avoids. Also
  reads charge status straight off the onboard BQ25101 charger IC's status pin (P0.17,
  the same signal that drives the board's charging LED) — a plain GPIO level, no ADC or
  averaging involved. Also drives P0.13 (HICHG) low permanently at boot to select the
  charger's 100 mA rate instead of the 50 mA it powers on with.
- `src/epaper_ssd1683.c/.h` — the real SSD1683 panel driver: reset, init sequence, full
  refresh, identify. Command bytes are transcribed from the datasheet's command table;
  the one value that *isn't* pinned to the primary source is the border waveform
  constant (0x3C = 0x05), taken from common third-party reference implementations —
  flagged explicitly in that file since it's the one register value here I couldn't
  verify from the datasheet itself.
- `src/epaper.c/.h` — the integration layer between the chunk protocol and the panel
  driver. `epaper_apply_full` parses the flat binary layout payload and re-rasterizes;
  `epaper_apply_diff` patches the retained layout and re-rasterizes from it.
- `src/layout_store.c/.h` — retains the last-applied layout in RAM so diffs have
  something to patch, and parses the flat binary wire format (`docs/protocol.md`).
- `src/rasterizer.c/.h` + `src/fonts/` — draws that layout into a 1bpp framebuffer using
  LVGL and real proportional fonts. The fonts, and the glyph-metrics table the design
  editor measures with, are both generated from one TTF by `tools/fonts/generate.mjs` —
  that shared origin is what makes the browser preview match the panel rather than
  approximate it. Renders in horizontal strips (`LV_DISPLAY_RENDER_MODE_PARTIAL`)
  because a full-panel RGB565 buffer would be 240KB and does not fit in RAM.
  Still no word-wrap and no partial refresh — every apply is a full redraw and a full
  panel refresh.
- `boards/xiao_ble.overlay` — SPI3 + GPIO wiring for CS/DC/RST/BUSY, the battery ADC, and
  charge status, per the pin mapping above.

## What you'll need to fill in / verify

1. **Raise the default wake interval** (`BLE_SERVICE_DEFAULT_WAKE_INTERVAL_S` in
   `ble_service.h`) once the gateway path is proven. The interval itself is no longer a
   fixed `#define` main.c sleeps on — it's runtime-configurable per display from the
   frontend (backend `wake_interval_s` → gateway `COMMAND_SET_WAKE_INTERVAL_S`, asserted
   every sync same as `rotate_180`), clamped to
   `[BLE_SERVICE_MIN_WAKE_INTERVAL_S, BLE_SERVICE_MAX_WAKE_INTERVAL_S]`. The *default* a
   freshly reset display starts at is still 15 s, chosen so bring-up iterations take about
   half a minute instead of four — content needs two wake cycles to converge
   (docs/protocol.md) — but that leaves the radio advertising roughly a fifth of the time,
   which is not a battery-life setting. Spec 8 leaves the real value open pending
   measurements.
2. **Verify the VBAT divider ratio** (`VBAT_DIVIDER_RATIO` in `battery.c`, currently 3)
   against a real battery and multimeter — the ~1/3 figure is what's documented for this
   board, not something measured on this specific unit.
3. **Verify P0.17 as the charge-status pin** (`charge-status-gpios` in
   `boards/xiao_ble.overlay`) against your actual board revision — sourced from the Seeed
   forum, not a schematic in this repo, same caveat as the VBAT divider ratio above.
4. **Verify P0.13 as the charge-current-select pin** (`charge-current-gpios` in
   `boards/xiao_ble.overlay`) against your actual board revision — same Seeed-forum
   sourcing and caveat as P0.17 above. If it's wrong, the failure mode is quiet (still
   charges, just not at the intended rate), so it's worth confirming with a multimeter on
   the charge current rather than trusting the pin number alone.

## Build

The nRF Connect SDK (v3.4.0) and its bundled toolchain are installed at
`/opt/nordic/ncs` — that's where `west`, the `arm-zephyr-eabi` compiler, and
`ZEPHYR_BASE` all come from. None of that is on `PATH` in a plain shell (only inside
the nRF Connect VS Code extension's integrated terminal, which sets it automatically),
so from a raw terminal/CI/agent shell, export it first:

```bash
export ZEPHYR_BASE=/opt/nordic/ncs/v3.4.0/zephyr
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr/gnu
export ZEPHYR_SDK_INSTALL_DIR=/opt/nordic/ncs/toolchains/ccc010f809/opt/zephyr-sdk
export PATH="/opt/nordic/ncs/toolchains/ccc010f809/bin:/opt/nordic/ncs/toolchains/ccc010f809/usr/bin:/opt/nordic/ncs/toolchains/ccc010f809/usr/local/bin:/opt/nordic/ncs/toolchains/ccc010f809/opt/bin:/opt/nordic/ncs/toolchains/ccc010f809/nrfutil/bin:/opt/nordic/ncs/toolchains/ccc010f809/opt/zephyr-sdk/gnu/arm-zephyr-eabi/bin:$PATH"
```

(`ccc010f809` is this machine's installed toolchain hash — check
`/opt/nordic/ncs/toolchains/toolchains.json` if that ever changes. This block is just
the `env` object from `.vscode/tasks.json` flattened into exports; keep the two in
sync.)

Then, from the repo root:

```bash
west build -b xiao_ble firmware -d firmware/build
```

Builds are incremental by default (only touched `.c` files get recompiled) — add
`-p always` for a pristine rebuild if CMake/Kconfig inputs changed, not just source.

### Flash (UF2 bootloader — no debug probe needed)

The XIAO nRF52840's stock bootloader is UF2: double-tap the reset button and the board
re-enumerates as a USB mass-storage drive named **XIAO-SENSE** (`diskutil list` /
`/Volumes/XIAO-SENSE` on macOS) instead of running the application. Copying a `.uf2`
file onto that drive flashes and auto-resets into it — that's what the `uf2` runner
below automates:

```bash
west flash -d firmware/build -r uf2
```

If the board isn't already in bootloader mode when this runs, double-tap reset first —
the runner waits for the drive to appear. Once it copies the file, the drive
disappears on its own as the board reboots into the new firmware; that disappearance
(`diskutil list | grep -i xiao` coming up empty) is the confirmation the flash landed,
since there's no separate "flash succeeded" message on the UF2 path.

If SWD pads are wired to a J-Link/debug probe instead, `-r nrfjprog` flashes over that
(faster, no manual reset dance) — see the "west flash (J-Link/debug probe)" VS Code
task below.

### VS Code tasks

Both of the above (plus the pristine-rebuild and J-Link variants) are wired up in
`.vscode/tasks.json`, which already carries the env block above — build with
**nRF: west build**, flash with **nRF: west flash (UF2 bootloader)** (default test
task, chained after a build).

## Why content_hash is a value from the wire, not a computed one

Firmware never computes `content_hash` itself — it adopts the 4-byte value prefixed onto
every `data_transfer` message by the gateway. See docs/protocol.md, "content_hash for
diff updates," for why: firmware can't reproduce the backend's canonical-JSON CRC32 from
a diff patch alone without retaining a full copy of the layout tree, so the backend just
tells it what to adopt, protected by the same CRC16 check as the rest of the message.
