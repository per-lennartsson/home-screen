#include "rasterizer.h"

#include <string.h>

#include <lvgl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "epaper_ssd1683.h"
#include "fonts/hs_fonts.h"

LOG_MODULE_REGISTER(rasterizer, CONFIG_LOG_DEFAULT_LEVEL);

/* Height of the strip LVGL renders into before each flush.
 *
 * This is why the render mode is PARTIAL rather than FULL: a full-panel RGB565 buffer
 * would be EPD_WIDTH * EPD_HEIGHT * 2 = 240KB, which does not fit in the nRF52840's 256KB
 * of RAM at all. LVGL instead renders the screen in horizontal bands and calls flush_cb
 * once per band, so the peak cost is one strip. 32 rows costs 400*32*2 = 25KB.
 *
 * The panel is only pushed once, after the whole screen has been rendered (see
 * epaper.c) — the strips accumulate into the caller's 1bpp framebuffer, they are not
 * individually sent to the display. So a smaller strip trades a little more per-band
 * overhead for RAM, and nothing else. */
#define STRIP_HEIGHT 32

/* RGB565 rather than LVGL's I1 (1-bit) format even though the panel is monochrome: I1 is
 * a far less exercised path in LVGL's software renderer, and the conversion to 1bpp has
 * to happen in flush_cb regardless (the SSD1683's bit-per-pixel layout is not LVGL's).
 * The cost is a transient strip buffer, not power or refresh time. */
static uint16_t strip_buf[EPD_WIDTH * STRIP_HEIGHT] __aligned(4);

static lv_display_t *display;

/* flush_cb has no way to receive caller context, so the current render target is stashed
 * here for the duration of one rasterizer_render() call. Safe because all panel work is
 * serialized onto the single epaper workqueue (see ble_service.c) — there is never a
 * second concurrent render. */
static uint8_t *target_fb;
static size_t target_fb_len;
static bool target_rotate_180;

/* Midpoint of the RGB565 channel sums; anything darker becomes a black pixel. */
#define RGB565_LUMA_MIDPOINT ((31 + 63 + 31) / 2)

static void set_pixel(int x, int y, bool black)
{
	if (x < 0 || y < 0 || x >= EPD_WIDTH || y >= EPD_HEIGHT) {
		return; /* silently clip — an out-of-range layout must never corrupt memory */
	}

	/* The panel comes out horizontally mirrored: the SSD1683's Driver Output Control
	 * (0x01) only exposes a gate (Y) scan direction, so the S0..S399 source wiring
	 * direction cannot be corrected from a register and has to be undone here.
	 *
	 * rotate_180 asks for the whole image rotated 180° on top of that. Working through
	 * the two reflections: a point that lands at device column (WIDTH-1-x) unrotated
	 * lands at column x once the panel is physically rotated in its mount — the mount's
	 * rotation cancels the hardware mirror — so rotate_180 skips the x mirror and flips
	 * y instead. */
	int device_x = target_rotate_180 ? x : (EPD_WIDTH - 1 - x);
	int device_y = target_rotate_180 ? (EPD_HEIGHT - 1 - y) : y;
	size_t byte_index = (size_t)device_y * EPD_WIDTH_BYTES + (size_t)device_x / 8;

	if (byte_index >= target_fb_len) {
		return;
	}

	uint8_t mask = (uint8_t)(0x80 >> (device_x % 8));

	if (black) {
		target_fb[byte_index] &= (uint8_t)~mask; /* 0 = black */
	} else {
		target_fb[byte_index] |= mask; /* 1 = white */
	}
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	const uint16_t *pixels = (const uint16_t *)px_map;
	int32_t w = lv_area_get_width(area);
	int32_t h = lv_area_get_height(area);

	for (int32_t y = 0; y < h; y++) {
		for (int32_t x = 0; x < w; x++) {
			/* Row stride is the area's own width, not the display width: under
			 * RENDER_MODE_PARTIAL each flush receives a buffer holding just this
			 * band, packed tightly. (Under RENDER_MODE_FULL it would be the whole
			 * display buffer instead — the two are not interchangeable.) */
			uint16_t px = pixels[y * w + x];
			uint8_t r = (px >> 11) & 0x1F;
			uint8_t g = (px >> 5) & 0x3F;
			uint8_t b = px & 0x1F;

			set_pixel(area->x1 + x, area->y1 + y, (r + g + b) < RGB565_LUMA_MIDPOINT);
		}
	}

	lv_display_flush_ready(disp);
}

static lv_display_t *get_display(void)
{
	if (display) {
		return display;
	}

	display = lv_display_create(EPD_WIDTH, EPD_HEIGHT);
	if (!display) {
		LOG_ERR("lv_display_create failed — out of LVGL heap? (CONFIG_LV_Z_MEM_POOL_SIZE)");
		return NULL;
	}

	/* Color format before buffers: lv_display_set_buffers() validates the buffer size
	 * against the format, and asserts if it is set afterwards. */
	lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
	lv_display_set_buffers(display, strip_buf, NULL, sizeof(strip_buf),
			       LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(display, flush_cb);

	return display;
}

/* The wire format carries Latin-1, one byte per glyph (docs/protocol.md), but LVGL
 * decodes label text as UTF-8 — so every byte >= 0x80 would be an invalid UTF-8 lead byte
 * and get dropped or misdecoded. That is precisely the range holding Å/Ä/Ö/å/ä/ö and the
 * degree sign, i.e. the characters this project's Swedish UI depends on.
 *
 * Latin-1 code points are identical to Unicode's first 256, so the transcode is the plain
 * two-byte UTF-8 encoding. Returns a NUL-terminated string in `out`.
 */
static void latin1_to_utf8(const char *text, uint8_t text_len, char *out, size_t out_size)
{
	size_t written = 0;

	for (uint8_t i = 0; i < text_len; i++) {
		unsigned char c = (unsigned char)text[i];

		if (c < 0x80) {
			if (written + 1 >= out_size) {
				break;
			}
			out[written++] = (char)c;
		} else {
			if (written + 2 >= out_size) {
				break;
			}
			out[written++] = (char)(0xC0 | (c >> 6));
			out[written++] = (char)(0x80 | (c & 0x3F));
		}
	}
	out[written] = '\0';
}

static lv_text_align_t lv_align_for(layout_align_t align)
{
	switch (align) {
	case LAYOUT_ALIGN_CENTER:
		return LV_TEXT_ALIGN_CENTER;
	case LAYOUT_ALIGN_RIGHT:
		return LV_TEXT_ALIGN_RIGHT;
	case LAYOUT_ALIGN_LEFT:
	default:
		return LV_TEXT_ALIGN_LEFT;
	}
}

static void draw_element(lv_obj_t *screen, const layout_element_t *el)
{
	/* Worst case every Latin-1 byte becomes two UTF-8 bytes, plus the terminator. */
	char utf8[LAYOUT_MAX_TEXT_LEN * 2 + 1];
	lv_obj_t *label = lv_label_create(screen);

	if (!label) {
		LOG_WRN("rasterizer: could not create label for element %u", el->element_id);
		return;
	}

	latin1_to_utf8(el->text, el->text_len, utf8, sizeof(utf8));
	lv_label_set_text(label, utf8);

	lv_obj_set_style_text_font(label, hs_font_get(el->font_id), 0);
	lv_obj_set_style_text_color(label, lv_color_black(), 0);
	lv_obj_set_style_text_align(label, lv_align_for(el->align), 0);

	/* Alignment needs a box to align within. Designs saved before format 3 carry no
	 * width, so fall back to sizing to the text — which makes every alignment behave as
	 * left, the same as it rendered before. */
	if (el->w > 0) {
		lv_obj_set_width(label, el->w);
	} else {
		lv_obj_set_width(label, LV_SIZE_CONTENT);
	}

	/* Clip rather than wrap: the design editor lays out absolutely-positioned boxes and
	 * does not wrap either, so wrapping here would put text where the editor never
	 * showed it. */
	lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);

	lv_text_decor_t decor = LV_TEXT_DECOR_NONE;

	if (el->underline) {
		decor |= LV_TEXT_DECOR_UNDERLINE;
	}
	/* Two independent reasons to strike text through: the explicit editor style, and a
	 * checklist row that has been ticked off (physically, via button.c, or from the
	 * backend). */
	if (el->strikethrough || (el->checkable && el->checked)) {
		decor |= LV_TEXT_DECOR_STRIKETHROUGH;
	}
	lv_obj_set_style_text_decor(label, decor, 0);

	lv_obj_set_pos(label, el->x, el->y);
}

void rasterizer_render(uint8_t *framebuffer, size_t framebuffer_len, const layout_t *layout,
			bool rotate_180)
{
	lv_display_t *disp = get_display();

	memset(framebuffer, 0xFF, framebuffer_len); /* 1 = white, per SSD1683 RAM convention */

	if (!disp) {
		return; /* already logged; a blank frame beats a corrupt one */
	}

	target_fb = framebuffer;
	target_fb_len = framebuffer_len;
	target_rotate_180 = rotate_180;

	lv_obj_t *screen = lv_display_get_screen_active(disp);

	/* Rebuild the object tree from scratch each render. The alternative — retaining
	 * labels and mutating them — would have to track element identity across full
	 * layout replacements, and this runs a handful of times an hour on a screen that
	 * takes seconds to refresh. */
	lv_obj_clean(screen);
	lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_all(screen, 0, 0);
	/* An element positioned near the panel edge makes the screen's content larger than
	 * the screen, and LVGL would then render scrollbars into the framebuffer — ink that
	 * the design editor never showed. There is nothing to scroll on an e-paper panel. */
	lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
	lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

	for (uint8_t i = 0; i < layout->count; i++) {
		draw_element(screen, &layout->elements[i]);
	}

	lv_refr_now(disp);

	target_fb = NULL;
	target_fb_len = 0;
}
