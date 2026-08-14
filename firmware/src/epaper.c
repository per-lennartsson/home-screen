/*
 * Integration layer between the chunk protocol (spec 4.3) and the panel driver
 * (epaper_ssd1683.h). Deliberately has no direct hardware dependency of its own so it
 * stays natively testable (firmware/tests/) — the real driver, or the native test
 * stub, is linked in separately.
 *
 * Rasterizes via layout_store.c (retains the last-applied full layout) + rasterizer.c
 * (draws it into `framebuffer`) — a minimal v1 rasterizer, not the full design-editor
 * text styling (see rasterizer.h's scope note). epaper_apply_full parses the flat binary
 * full-layout payload (docs/protocol.md — firmware has no JSON parser) and re-rasterizes
 * everything; epaper_apply_diff patches the retained layout's affected element(s) and
 * re-rasterizes the whole framebuffer from that updated state (full refresh only in v1,
 * even for a diff — see the plan for this feature's "known limitations").
 *
 * Also owns the system-status overlays (epaper_set_charging/epaper_set_connection_lost,
 * epaper.h) — small icons composited on top of the rasterized layout on every push, so
 * they stay visible independent of the gateway. See render_and_push below.
 */

#include "epaper.h"

#include <stdbool.h>
#include <string.h>

#include <zephyr/logging/log.h>

#include "chunk_protocol.h"
#include "epaper_ssd1683.h"
#include "layout_store.h"
#include "rasterizer.h"

LOG_MODULE_REGISTER(epaper, CONFIG_LOG_DEFAULT_LEVEL);

static uint8_t framebuffer[EPD_FRAMEBUFFER_SIZE];
static bool current_rotate_180;

/* System-status overlays (epaper_set_charging/epaper_set_connection_lost, epaper.h) —
 * small fixed icons, independent of layout_store/rasterizer, so they can be drawn even
 * when the gateway has never sent anything at all. Hand-authored as row strings ('.' =
 * background, anything else = ink) rather than packed bitmaps: at 16x16 this is easier
 * to review and hand-edit than binary, and small enough that the format's overhead
 * doesn't matter. */
#define OVERLAY_ICON_SIZE 16
#define OVERLAY_ICON_MARGIN 8

static bool overlay_charging;
static bool overlay_connection_lost;

/* Lightning bolt, top-left corner. */
static const char *const icon_charging[OVERLAY_ICON_SIZE] = {
	"......####......", ".....####.......", "....####........", "...####.........",
	"..####..........", ".############...", "##############..", "..........####..",
	".........####...", "........####....", ".......####.....", "......####......",
	".....####.......", "....####........", "...####.........", "..####..........",
};

/* Bold X, top-right corner — "not connected". */
static const char *const icon_connection_lost[OVERLAY_ICON_SIZE] = {
	"##............##", "###..........###", ".###........###.", "..###......###..",
	"...###....###...", "....###..###....", ".....######.....", "......####......",
	"......####......", ".....######.....", "....###..###....", "...###....###...",
	"..###......###..", ".###........###.", "###..........###", "##............##",
};

static void set_overlay_pixel(int x, int y)
{
	if (x < 0 || y < 0 || x >= EPD_WIDTH || y >= EPD_HEIGHT) {
		return; /* silently clip, same convention as rasterizer.c's set_pixel */
	}

	/* Y flips for rotate_180, X never does — matches rasterizer.c's set_pixel; see its
	 * comment for why the SSD1683's wiring doesn't need X mirroring. */
	int device_y = current_rotate_180 ? (EPD_HEIGHT - 1 - y) : y;
	size_t byte_index = (size_t)device_y * EPD_WIDTH_BYTES + (size_t)x / 8;

	if (byte_index >= sizeof(framebuffer)) {
		return;
	}

	uint8_t mask = (uint8_t)(0x80 >> (x % 8));
	framebuffer[byte_index] &= (uint8_t)~mask; /* 0 = black */
}

static void draw_icon(const char *const rows[], int origin_x, int origin_y)
{
	for (int row = 0; row < OVERLAY_ICON_SIZE; row++) {
		for (int col = 0; col < OVERLAY_ICON_SIZE; col++) {
			if (rows[row][col] != '.') {
				set_overlay_pixel(origin_x + col, origin_y + row);
			}
		}
	}
}

static void draw_overlays(void)
{
	if (overlay_charging) {
		draw_icon(icon_charging, OVERLAY_ICON_MARGIN, OVERLAY_ICON_MARGIN);
	}
	if (overlay_connection_lost) {
		draw_icon(icon_connection_lost, EPD_WIDTH - OVERLAY_ICON_MARGIN - OVERLAY_ICON_SIZE,
			  OVERLAY_ICON_MARGIN);
	}
}

/* Shared tail end of every panel update: re-rasterize the retained layout, composite
 * whichever status overlays are currently active on top of it, then push. Overlays are
 * redrawn from scratch here rather than persisted across calls, since rasterizer_render
 * always clears the framebuffer first — this is what lets a fresh gateway push simply
 * win over a stale connection-lost badge in the same corner, with no special-casing. */
static bool render_and_push(void)
{
	rasterizer_render(framebuffer, sizeof(framebuffer), layout_store_get(), current_rotate_180);
	draw_overlays();
	return epd_ssd1683_push_full(framebuffer, sizeof(framebuffer)) == 0;
}

static bool push_blank_frame(void)
{
	memset(framebuffer, 0xFF, sizeof(framebuffer)); /* 1 = white, per SSD1683 RAM convention */
	return epd_ssd1683_push_full(framebuffer, sizeof(framebuffer)) == 0;
}

int epaper_init(void)
{
	int err = epd_ssd1683_init();
	if (err) {
		LOG_ERR("epaper: hardware init failed (%d)", err);
		return err;
	}

	/* Bring-up smoke test: proves the panel actually refreshes on real hardware,
	 * independent of whether content rendering exists yet. */
	if (!push_blank_frame()) {
		LOG_WRN("epaper: initial blank-frame push failed");
	}
	return 0;
}

bool epaper_apply_full(const uint8_t *data, size_t len)
{
	if (!layout_store_apply_full(data, len)) {
		LOG_WRN("epaper: malformed full layout payload, keeping previous content");
		return false;
	}

	/* Real data from the gateway is proof the connection just worked — clear any
	 * stale connection-lost badge in this same push rather than waiting for
	 * main.c's end-of-cycle bookkeeping (epaper_set_connection_lost) to catch up. */
	overlay_connection_lost = false;
	return render_and_push();
}

bool epaper_apply_diff(const uint8_t *data, size_t len)
{
	chunk_diff_entry_t entries[CHUNK_MAX_DIFF_ENTRIES];
	int count = chunk_protocol_decode_diff(data, len, entries, CHUNK_MAX_DIFF_ENTRIES);

	if (count < 0) {
		LOG_WRN("epaper: malformed diff payload");
		return false;
	}

	for (int i = 0; i < count; i++) {
		if (!layout_store_apply_diff_entry(&entries[i])) {
			LOG_WRN("epaper: diff references unknown element_id=%u, ignoring",
				entries[i].element_id);
		}
	}

	/* v1 scope: full refresh even for a diff — no partial-refresh optimization yet
	 * (see rasterizer.h's scope note). Still correct, just not the fastest/lowest-
	 * power path a future version could take. */
	overlay_connection_lost = false; /* see epaper_apply_full's identical comment */
	return render_and_push();
}

/* How many full black<->white passes to flash before redrawing content. epaper_apply_full
 * and epaper_apply_diff both already push through epd_display_refresh's DUC2=0xFF
 * waveform on *every* update (see this file's header comment — there's no separate
 * partial-refresh path in v1), so a single extra push of the same waveform (what this
 * function used to do: one blank frame, then redraw) is electrically identical to an
 * ordinary content update and does nothing extra to the accumulated ghosting. What
 * actually shakes out the residual DC bias in the pixels is several full inversions in a
 * row — the same technique epd_ssd1683_identify's black-then-white blink uses, just
 * repeated. Tune this if ghosting still isn't fully gone in practice.
 */
#define FULL_REFRESH_FLASH_CYCLES 3

void epaper_force_full_refresh(void)
{
	for (int i = 0; i < FULL_REFRESH_FLASH_CYCLES; i++) {
		memset(framebuffer, 0x00, sizeof(framebuffer)); /* 0 = black */
		epd_ssd1683_push_full(framebuffer, sizeof(framebuffer));
		memset(framebuffer, 0xFF, sizeof(framebuffer)); /* 1 = white */
		epd_ssd1683_push_full(framebuffer, sizeof(framebuffer));
	}

	/* Redraw whatever layout_store already has retained so the display ends up showing
	 * its current content again, just cleanly. */
	render_and_push();
}

void epaper_identify(void)
{
	epd_ssd1683_identify();
}

void epaper_set_rotation(bool rotate_180)
{
	if (rotate_180 == current_rotate_180) {
		return; /* gateway resends this every sync, whether or not it changed */
	}
	current_rotate_180 = rotate_180;
	render_and_push();
}

void epaper_set_charging(bool charging)
{
	if (charging == overlay_charging) {
		return; /* main.c calls this every wake cycle; only redraw on an actual flip */
	}
	overlay_charging = charging;
	render_and_push();
}

void epaper_set_connection_lost(bool lost)
{
	if (lost == overlay_connection_lost) {
		return;
	}
	overlay_connection_lost = lost;
	render_and_push();
}
