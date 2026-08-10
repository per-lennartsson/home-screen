#include "rasterizer.h"

#include <stdbool.h>
#include <string.h>

#include "epaper_ssd1683.h"
#include "font_basic.h"

#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 8

/* font_scale range a layout_element_t can carry (layout_store.h) — mirrors
 * gateway/gateway/protocol.py's FONT_SCALE_MIN/MAX. Not a wire-format constraint (the
 * byte is a plain uint8_t), just the range the v1 bitmap font can be sanely
 * pixel-multiplied to on a 400x300 panel; clamped here rather than at parse time
 * (layout_store.c) for the same reason x/y aren't range-checked there either —
 * set_pixel_black() below clips safely regardless. */
#define FONT_SCALE_MIN 1
#define FONT_SCALE_MAX 8

#define GLYPH_INDEX_DEGREE 0xFE
#define GLYPH_INDEX_A_RING 0xFD
#define GLYPH_INDEX_A_DIAERESIS 0xFC
#define GLYPH_INDEX_O_DIAERESIS 0xFB
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
	if (uc == 0xC5 || uc == 0xE5) {
		/* Å/å — Latin-1 0xC5/0xE5. Swedish room/label text (this project's primary
		 * UI language) uses these routinely, so they get the same one-off glyph
		 * treatment as the degree sign above, not the placeholder box. */
		return GLYPH_INDEX_A_RING;
	}
	if (uc == 0xC4 || uc == 0xE4) {
		return GLYPH_INDEX_A_DIAERESIS; /* Ä/ä */
	}
	if (uc == 0xD6 || uc == 0xF6) {
		return GLYPH_INDEX_O_DIAERESIS; /* Ö/ö */
	}
	if (uc < FONT_FIRST_CHAR || uc > FONT_LAST_CHAR) {
		return GLYPH_INDEX_NONE; /* no glyph for this character — draw_glyph() falls back to a placeholder */
	}
	return (uint8_t)(uc - FONT_FIRST_CHAR);
}

static uint8_t clamp_font_scale(uint8_t font_scale)
{
	if (font_scale < FONT_SCALE_MIN) {
		return FONT_SCALE_MIN;
	}
	if (font_scale > FONT_SCALE_MAX) {
		return FONT_SCALE_MAX;
	}
	return font_scale;
}

/* Draws one glyph pixel as a scale x scale block — the v1 font is one hand-authored
 * 8x8 bitmap (font_basic.h); larger font_scale values pixel-multiply it rather than
 * switching to a second, separately-authored font. */
static void draw_scaled_pixel(uint8_t *fb, size_t fb_len, int x, int y, uint8_t scale, bool rotate_180)
{
	for (int dy = 0; dy < scale; dy++) {
		for (int dx = 0; dx < scale; dx++) {
			set_pixel_black(fb, fb_len, x + dx, y + dy, rotate_180);
		}
	}
}

static void draw_glyph(uint8_t *fb, size_t fb_len, int origin_x, int origin_y, char c, uint8_t scale,
			bool rotate_180)
{
	uint8_t index = to_glyph_index(c);

	if (index == GLYPH_INDEX_NONE) {
		/* A small outline box — visually distinct from any real glyph, so a
		 * character outside the v1 font's coverage is obviously a gap during
		 * bring-up rather than silently misrendering as some other letter. */
		for (int dx = 1; dx <= 5; dx++) {
			draw_scaled_pixel(fb, fb_len, origin_x + dx * scale, origin_y, scale, rotate_180);
			draw_scaled_pixel(fb, fb_len, origin_x + dx * scale, origin_y + 6 * scale, scale,
					   rotate_180);
		}
		for (int dy = 0; dy <= 6; dy++) {
			draw_scaled_pixel(fb, fb_len, origin_x + scale, origin_y + dy * scale, scale, rotate_180);
			draw_scaled_pixel(fb, fb_len, origin_x + 5 * scale, origin_y + dy * scale, scale,
					   rotate_180);
		}
		return;
	}

	const uint8_t *rows;

	switch (index) {
	case GLYPH_INDEX_DEGREE:
		rows = font_glyph_degree;
		break;
	case GLYPH_INDEX_A_RING:
		rows = font_glyph_a_ring;
		break;
	case GLYPH_INDEX_A_DIAERESIS:
		rows = font_glyph_a_diaeresis;
		break;
	case GLYPH_INDEX_O_DIAERESIS:
		rows = font_glyph_o_diaeresis;
		break;
	default:
		rows = font_glyphs[index];
		break;
	}
	for (int row = 0; row < GLYPH_HEIGHT; row++) {
		uint8_t bits = rows[row];
		for (int col = 0; col < GLYPH_WIDTH; col++) {
			if (bits & (0x80 >> col)) {
				draw_scaled_pixel(fb, fb_len, origin_x + col * scale, origin_y + row * scale,
						   scale, rotate_180);
			}
		}
	}
}

static void draw_text(uint8_t *fb, size_t fb_len, int x, int y, const char *text, uint8_t text_len,
		       uint8_t scale, bool rotate_180)
{
	for (uint8_t i = 0; i < text_len; i++) {
		draw_glyph(fb, fb_len, x + i * GLYPH_WIDTH * scale, y, text[i], scale, rotate_180);
	}
}

static void draw_strikethrough(uint8_t *fb, size_t fb_len, int x, int y, uint8_t text_len, uint8_t scale,
				bool rotate_180)
{
	int width = text_len * GLYPH_WIDTH * scale;
	int mid_y = y + (GLYPH_HEIGHT * scale) / 2;
	int thickness = 2 * scale; /* 2px at scale 1 — visible on a 400x300 panel — scaling with the text */

	for (int dx = 0; dx < width; dx++) {
		for (int dy = 0; dy < thickness; dy++) {
			set_pixel_black(fb, fb_len, x + dx, mid_y + dy, rotate_180);
		}
	}
}

void rasterizer_render(uint8_t *framebuffer, size_t framebuffer_len, const layout_t *layout, bool rotate_180)
{
	memset(framebuffer, 0xFF, framebuffer_len); /* 1 = white, per SSD1683 RAM convention */

	for (uint8_t i = 0; i < layout->count; i++) {
		const layout_element_t *el = &layout->elements[i];
		uint8_t scale = clamp_font_scale(el->font_scale);

		draw_text(framebuffer, framebuffer_len, el->x, el->y, el->text, el->text_len, scale, rotate_180);

		if (el->checkable && el->checked) {
			draw_strikethrough(framebuffer, framebuffer_len, el->x, el->y, el->text_len, scale,
					    rotate_180);
		}
	}
}
