/*
 * Host-native check that rasterizer.c's font_scale handling (layout_store.h's
 * font_scale field, added alongside the wire format's format-version-2 byte) actually
 * pixel-multiplies glyphs rather than being silently ignored, and that out-of-range
 * font_scale values get clamped instead of over/under-drawing or looping unboundedly.
 *
 * Compiled with the system C compiler against the shims in tests/shims/ — see
 * firmware/tests/run.sh.
 *
 * Run: firmware/tests/run.sh
 */

#include <stdio.h>
#include <string.h>

#include "epaper_ssd1683.h"
#include "layout_store.h"
#include "rasterizer.h"

static int failures = 0;

#define CHECK(cond, msg)                                                                         \
	do {                                                                                       \
		if (!(cond)) {                                                                    \
			printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);                   \
			failures++;                                                               \
		}                                                                                 \
	} while (0)

/* Mirrors set_pixel_black's non-rotated (rotate_180=false) x-mirror transform
 * (rasterizer.c) so tests can address pixels in the same (x, y) space the layout does. */
static bool pixel_is_black(const uint8_t *fb, int x, int y)
{
	int device_x = EPD_WIDTH - 1 - x;
	int device_y = y;
	size_t byte_index = (size_t)device_y * EPD_WIDTH_BYTES + (size_t)device_x / 8;
	return (fb[byte_index] & (0x80 >> (device_x % 8))) == 0; /* 0 = black */
}

static void render_single_char(uint8_t *fb, char c, uint8_t font_scale)
{
	layout_t layout;
	memset(&layout, 0, sizeof(layout));
	layout.count = 1;
	layout.elements[0].element_id = 1;
	layout.elements[0].x = 0;
	layout.elements[0].y = 0;
	layout.elements[0].font_scale = font_scale;
	layout.elements[0].text[0] = c;
	layout.elements[0].text_len = 1;

	rasterizer_render(fb, EPD_FRAMEBUFFER_SIZE, &layout, false);
}

/* 'I' (font_basic.h): {0x70,0x20,0x20,0x20,0x20,0x20,0x70,0x00} — column 2 (bit 0x20) is
 * black on every one of its 7 non-blank rows, giving a single unbroken vertical stroke
 * that's easy to measure at any scale. */
static void test_font_scale_1_matches_unscaled_glyph(void)
{
	static uint8_t fb[EPD_FRAMEBUFFER_SIZE];
	render_single_char(fb, 'I', 1);

	CHECK(pixel_is_black(fb, 2, 0), "scale 1: column 2 row 0 should be black ('I' top bar)");
	CHECK(pixel_is_black(fb, 2, 5), "scale 1: column 2 row 5 should be black ('I' stem)");
	CHECK(!pixel_is_black(fb, 2, 7), "scale 1: row 7 is the glyph's blank spacer row");
	CHECK(!pixel_is_black(fb, 3, 3), "scale 1: column 3 should stay white ('I' stem is 1px wide)");
}

static void test_font_scale_2_pixel_doubles_the_glyph(void)
{
	static uint8_t fb[EPD_FRAMEBUFFER_SIZE];
	render_single_char(fb, 'I', 2);

	/* The scale-1 stem pixel at (col=2, row=5) becomes a 2x2 block at (4..5, 10..11). */
	CHECK(pixel_is_black(fb, 4, 10), "scale 2: doubled stem pixel (4,10) should be black");
	CHECK(pixel_is_black(fb, 5, 10), "scale 2: doubled stem pixel (5,10) should be black");
	CHECK(pixel_is_black(fb, 4, 11), "scale 2: doubled stem pixel (4,11) should be black");
	CHECK(pixel_is_black(fb, 5, 11), "scale 2: doubled stem pixel (5,11) should be black");
	CHECK(!pixel_is_black(fb, 6, 10), "scale 2: one column past the doubled stem should stay white");
	CHECK(!pixel_is_black(fb, 4, 14), "scale 2: row 7's blank spacer should still be blank once doubled");
}

static void test_font_scale_advances_next_glyph_origin(void)
{
	static uint8_t fb[EPD_FRAMEBUFFER_SIZE];
	layout_t layout;
	memset(&layout, 0, sizeof(layout));
	layout.count = 1;
	layout.elements[0].element_id = 1;
	layout.elements[0].x = 0;
	layout.elements[0].y = 0;
	layout.elements[0].font_scale = 2;
	memcpy(layout.elements[0].text, "II", 2);
	layout.elements[0].text_len = 2;

	rasterizer_render(fb, EPD_FRAMEBUFFER_SIZE, &layout, false);

	/* At scale 2, each glyph cell is 16px wide (GLYPH_WIDTH=8 * scale=2), so the second
	 * 'I' 's stem should reappear at x=16+4=20, not x=8+4=12 (which is where an
	 * un-scaled advance would have placed it). */
	CHECK(pixel_is_black(fb, 20, 10), "second glyph's stem should be scale-advanced to x=20");
	CHECK(!pixel_is_black(fb, 12, 10), "un-scaled glyph spacing (x=12) should be blank once scaled");
}

static void test_font_scale_out_of_range_is_clamped_not_ignored(void)
{
	static uint8_t fb_zero[EPD_FRAMEBUFFER_SIZE];
	static uint8_t fb_huge[EPD_FRAMEBUFFER_SIZE];

	/* font_scale=0 is nonsensical (a zero-size glyph) — clamp to the scale-1 rendering
	 * rather than drawing nothing. */
	render_single_char(fb_zero, 'I', 0);
	CHECK(pixel_is_black(fb_zero, 2, 5), "font_scale=0 should clamp up to scale 1, not draw nothing");

	/* font_scale=200 would draw a glyph roughly 1600px tall if taken literally — clamp
	 * to FONT_SCALE_MAX (8) so a single malformed byte can't blow past the panel. */
	render_single_char(fb_huge, 'I', 200);
	CHECK(pixel_is_black(fb_huge, 16, 40), "font_scale=200 should clamp to scale 8, not scale 200");
	CHECK(!pixel_is_black(fb_huge, 16, 60),
	      "font_scale=200 clamped to 8 should not paint past an 8x-scaled glyph's height");
}

int main(void)
{
	test_font_scale_1_matches_unscaled_glyph();
	test_font_scale_2_pixel_doubles_the_glyph();
	test_font_scale_advances_next_glyph_origin();
	test_font_scale_out_of_range_is_clamped_not_ignored();

	if (failures == 0) {
		printf("All rasterizer native tests passed.\n");
		return 0;
	}
	printf("%d check(s) failed.\n", failures);
	return 1;
}
