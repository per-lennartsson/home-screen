#include "rasterizer.h"

#include <stdbool.h>
#include <string.h>

#include "epaper_ssd1683.h"
#include "font_basic.h"

#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 8

#define GLYPH_INDEX_DEGREE 0xFE
#define GLYPH_INDEX_NONE 0xFF

static void set_pixel_black(uint8_t *fb, size_t fb_len, int x, int y, bool rotate_180)
{
	if (x < 0 || y < 0 || x >= EPD_WIDTH || y >= EPD_HEIGHT) {
		return; /* silently clip — an out-of-range layout must never corrupt memory */
	}
	/* First real-hardware bring-up showed the panel comes out horizontally mirrored:
	 * the SSD1683's Driver Output Control (0x01) only exposes a gate (Y) scan
	 * direction (GD/SM/TB) — there's no source (X) scan direction bit, so the S0..S399
	 * wiring direction can't be corrected from a register. Mirroring x here, once, is
	 * equivalent to flipping the whole rendered image before it's written to RAM.
	 *
	 * rotate_180 (per-display setting, spec: ble_service.h's ROTATE_180 command) asks
	 * for the whole image rotated 180° on top of that — e.g. the enclosure's uneven
	 * bezel needs to sit at the other edge. Working through the two reflections: a
	 * point that always ends up at device column (WIDTH-1-x) when not rotated ends up
	 * at device column x once the panel itself is physically rotated 180° in its mount
	 * (the mount's rotation is what cancels the hardware mirror here, not this code) —
	 * so rotate_180 skips the x mirror entirely and flips y instead. */
	int device_x = rotate_180 ? x : (EPD_WIDTH - 1 - x);
	int device_y = rotate_180 ? (EPD_HEIGHT - 1 - y) : y;
	size_t byte_index = (size_t)device_y * EPD_WIDTH_BYTES + (size_t)device_x / 8;
	if (byte_index >= fb_len) {
		return;
	}
	fb[byte_index] &= (uint8_t)~(0x80 >> (device_x % 8));
}

static uint8_t to_glyph_index(char c)
{
	unsigned char uc = (unsigned char)c;

	if (uc >= 'a' && uc <= 'z') {
		/* v1's font has no lowercase glyphs (font_basic.h) — render as uppercase
		 * rather than doubling the table; the retained text itself keeps its real
		 * case, only glyph selection is affected. */
		uc = (unsigned char)(uc - 'a' + 'A');
	}
	if (uc == 0xB0) {
		/* Degree sign — Latin-1 code point 0xB0, the one non-ASCII character this
		 * pipeline carries through intact (gateway/gateway/protocol.py encodes
		 * text as Latin-1, not ASCII, specifically for this). One extra glyph
		 * (font_basic.h's font_glyph_degree) rather than widening the main
		 * contiguous table all the way from 'Z' (0x5A) up to 0xB0. */
		return GLYPH_INDEX_DEGREE;
	}
	if (uc < FONT_FIRST_CHAR || uc > FONT_LAST_CHAR) {
		return GLYPH_INDEX_NONE; /* no glyph for this character — draw_glyph() falls back to a placeholder */
	}
	return (uint8_t)(uc - FONT_FIRST_CHAR);
}

static void draw_glyph(uint8_t *fb, size_t fb_len, int origin_x, int origin_y, char c, bool rotate_180)
{
	uint8_t index = to_glyph_index(c);

	if (index == GLYPH_INDEX_NONE) {
		/* A small outline box — visually distinct from any real glyph, so a
		 * character outside the v1 font's coverage is obviously a gap during
		 * bring-up rather than silently misrendering as some other letter. */
		for (int dx = 1; dx <= 5; dx++) {
			set_pixel_black(fb, fb_len, origin_x + dx, origin_y, rotate_180);
			set_pixel_black(fb, fb_len, origin_x + dx, origin_y + 6, rotate_180);
		}
		for (int dy = 0; dy <= 6; dy++) {
			set_pixel_black(fb, fb_len, origin_x + 1, origin_y + dy, rotate_180);
			set_pixel_black(fb, fb_len, origin_x + 5, origin_y + dy, rotate_180);
		}
		return;
	}

	const uint8_t *rows = (index == GLYPH_INDEX_DEGREE) ? font_glyph_degree : font_glyphs[index];
	for (int row = 0; row < GLYPH_HEIGHT; row++) {
		uint8_t bits = rows[row];
		for (int col = 0; col < GLYPH_WIDTH; col++) {
			if (bits & (0x80 >> col)) {
				set_pixel_black(fb, fb_len, origin_x + col, origin_y + row, rotate_180);
			}
		}
	}
}

static void draw_text(uint8_t *fb, size_t fb_len, int x, int y, const char *text, uint8_t text_len,
		       bool rotate_180)
{
	for (uint8_t i = 0; i < text_len; i++) {
		draw_glyph(fb, fb_len, x + i * GLYPH_WIDTH, y, text[i], rotate_180);
	}
}

static void draw_strikethrough(uint8_t *fb, size_t fb_len, int x, int y, uint8_t text_len, bool rotate_180)
{
	int width = text_len * GLYPH_WIDTH;
	int mid_y = y + GLYPH_HEIGHT / 2;

	for (int dx = 0; dx < width; dx++) {
		set_pixel_black(fb, fb_len, x + dx, mid_y, rotate_180);
		set_pixel_black(fb, fb_len, x + dx, mid_y + 1, rotate_180); /* 2px — visible on a 400x300 panel */
	}
}

void rasterizer_render(uint8_t *framebuffer, size_t framebuffer_len, const layout_t *layout, bool rotate_180)
{
	memset(framebuffer, 0xFF, framebuffer_len); /* 1 = white, per SSD1683 RAM convention */

	for (uint8_t i = 0; i < layout->count; i++) {
		const layout_element_t *el = &layout->elements[i];

		draw_text(framebuffer, framebuffer_len, el->x, el->y, el->text, el->text_len, rotate_180);

		if (el->checkable && el->checked) {
			draw_strikethrough(framebuffer, framebuffer_len, el->x, el->y, el->text_len, rotate_180);
		}
	}
}
