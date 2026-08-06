#include "rasterizer.h"

#include <string.h>

#include "epaper_ssd1683.h"
#include "font_basic.h"

#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 8

static void set_pixel_black(uint8_t *fb, size_t fb_len, int x, int y)
{
	if (x < 0 || y < 0 || x >= EPD_WIDTH || y >= EPD_HEIGHT) {
		return; /* silently clip — an out-of-range layout must never corrupt memory */
	}
	size_t byte_index = (size_t)y * EPD_WIDTH_BYTES + (size_t)x / 8;
	if (byte_index >= fb_len) {
		return;
	}
	fb[byte_index] &= (uint8_t)~(0x80 >> (x % 8));
}

static uint8_t to_glyph_index(char c)
{
	if (c >= 'a' && c <= 'z') {
		/* v1's font has no lowercase glyphs (font_basic.h) — render as uppercase
		 * rather than doubling the table; the retained text itself keeps its real
		 * case, only glyph selection is affected. */
		c = (char)(c - 'a' + 'A');
	}
	if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) {
		return 0xFF; /* no glyph for this character — draw_glyph() falls back to a placeholder */
	}
	return (uint8_t)(c - FONT_FIRST_CHAR);
}

static void draw_glyph(uint8_t *fb, size_t fb_len, int origin_x, int origin_y, char c)
{
	uint8_t index = to_glyph_index(c);

	if (index == 0xFF) {
		/* A small outline box — visually distinct from any real glyph, so a
		 * character outside the v1 font's coverage is obviously a gap during
		 * bring-up rather than silently misrendering as some other letter. */
		for (int dx = 1; dx <= 5; dx++) {
			set_pixel_black(fb, fb_len, origin_x + dx, origin_y);
			set_pixel_black(fb, fb_len, origin_x + dx, origin_y + 6);
		}
		for (int dy = 0; dy <= 6; dy++) {
			set_pixel_black(fb, fb_len, origin_x + 1, origin_y + dy);
			set_pixel_black(fb, fb_len, origin_x + 5, origin_y + dy);
		}
		return;
	}

	const uint8_t *rows = font_glyphs[index];
	for (int row = 0; row < GLYPH_HEIGHT; row++) {
		uint8_t bits = rows[row];
		for (int col = 0; col < GLYPH_WIDTH; col++) {
			if (bits & (0x80 >> col)) {
				set_pixel_black(fb, fb_len, origin_x + col, origin_y + row);
			}
		}
	}
}

static void draw_text(uint8_t *fb, size_t fb_len, int x, int y, const char *text, uint8_t text_len)
{
	for (uint8_t i = 0; i < text_len; i++) {
		draw_glyph(fb, fb_len, x + i * GLYPH_WIDTH, y, text[i]);
	}
}

static void draw_strikethrough(uint8_t *fb, size_t fb_len, int x, int y, uint8_t text_len)
{
	int width = text_len * GLYPH_WIDTH;
	int mid_y = y + GLYPH_HEIGHT / 2;

	for (int dx = 0; dx < width; dx++) {
		set_pixel_black(fb, fb_len, x + dx, mid_y);
		set_pixel_black(fb, fb_len, x + dx, mid_y + 1); /* 2px — visible on a 400x300 panel */
	}
}

void rasterizer_render(uint8_t *framebuffer, size_t framebuffer_len, const layout_t *layout)
{
	memset(framebuffer, 0xFF, framebuffer_len); /* 1 = white, per SSD1683 RAM convention */

	for (uint8_t i = 0; i < layout->count; i++) {
		const layout_element_t *el = &layout->elements[i];

		draw_text(framebuffer, framebuffer_len, el->x, el->y, el->text, el->text_len);

		if (el->checkable && el->checked) {
			draw_strikethrough(framebuffer, framebuffer_len, el->x, el->y, el->text_len);
		}
	}
}
