/*
 * Integration layer between the chunk protocol (spec 4.3) and the panel driver
 * (epaper_ssd1683.h). Deliberately has no direct hardware dependency of its own so it
 * stays natively testable (firmware/tests/) — the real driver, or the native test
 * stub, is linked in separately.
 *
 * Rasterizing a design's layout_json into actual pixels is Section 7 build-order step
 * 4 and doesn't exist yet (see firmware/README.md). Until it does, epaper_apply_full
 * pushes a blank (all-white) frame through the real hardware path — this still proves
 * out the full sync pipeline (chunk reassembly, hash handling, GATT, SPI/GPIO timing)
 * end to end on real hardware, it just doesn't draw your dashboard's content yet.
 * epaper_apply_diff has no framebuffer to patch without that rasterizer, so it decodes
 * and logs the value changes but doesn't push anything to the panel.
 */

#include "epaper.h"

#include <stdbool.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "chunk_protocol.h"
#include "epaper_ssd1683.h"

LOG_MODULE_REGISTER(epaper, CONFIG_LOG_DEFAULT_LEVEL);

static uint8_t framebuffer[EPD_FRAMEBUFFER_SIZE];

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
	ARG_UNUSED(data);
	LOG_INF("epaper: TODO rasterize %zu bytes of layout data (not implemented yet) — "
		"pushing a blank frame so the sync pipeline is still exercised on real "
		"hardware",
		len);
	return push_blank_frame();
}

bool epaper_apply_diff(const uint8_t *data, size_t len)
{
	chunk_diff_entry_t entries[CHUNK_MAX_DIFF_ENTRIES];
	int count = chunk_protocol_decode_diff(data, len, entries, CHUNK_MAX_DIFF_ENTRIES);

	if (count < 0) {
		LOG_WRN("epaper: malformed diff payload");
		return false;
	}

	LOG_INF("epaper: TODO apply %d changed value(s) to the framebuffer — no "
		"rasterizer/layout tracking exists yet, so this is decoded but not drawn",
		count);
	for (int i = 0; i < count; i++) {
		LOG_INF("epaper:   element_id=%u value_len=%u", entries[i].element_id,
			entries[i].value_len);
	}
	return true;
}

void epaper_force_full_refresh(void)
{
	push_blank_frame();
}

void epaper_identify(void)
{
	epd_ssd1683_identify();
}
