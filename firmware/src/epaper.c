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

	rasterizer_render(framebuffer, sizeof(framebuffer), layout_store_get(), current_rotate_180);
	return epd_ssd1683_push_full(framebuffer, sizeof(framebuffer)) == 0;
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
	rasterizer_render(framebuffer, sizeof(framebuffer), layout_store_get(), current_rotate_180);
	return epd_ssd1683_push_full(framebuffer, sizeof(framebuffer)) == 0;
}

void epaper_force_full_refresh(void)
{
	push_blank_frame();
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
	rasterizer_render(framebuffer, sizeof(framebuffer), layout_store_get(), current_rotate_180);
	epd_ssd1683_push_full(framebuffer, sizeof(framebuffer));
}
