/*
 * Draws the layout_store's retained elements into a 1bpp framebuffer, using LVGL and the
 * generated Montserrat fonts in src/fonts/ (see fonts/hs_fonts.h).
 *
 * The point of going through LVGL is not the widget toolkit — it is that the design
 * editor and the panel now lay text out with the same glyph advances, from fonts and a
 * metrics table produced by the same run of tools/fonts/generate.mjs. Before this, a
 * fixed-width 8px bitmap font here and the browser's system sans-serif there could never
 * agree on where a string ended.
 *
 * Supported per element: size and weight (via font_id), left/center/right alignment
 * within the element's box, underline, and strikethrough. Still out of scope: word wrap
 * (text is clipped to the element box), and partial refresh — every apply is a full
 * redraw and a full panel refresh.
 */

#ifndef RASTERIZER_H_
#define RASTERIZER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "layout_store.h"

/* Clears framebuffer to all-white, then draws every element in `layout`. framebuffer must
 * be at least EPD_FRAMEBUFFER_SIZE (epaper_ssd1683.h) bytes, 1bpp MSB-first, bit=1
 * white/bit=0 black per the SSD1683's Write RAM(BW) convention — matches what
 * epd_ssd1683_push_full expects directly.
 *
 * A checked+checkable element is struck through, as is any element carrying the explicit
 * strikethrough style.
 *
 * rotate_180: per-display mounting setting (see epaper_set_rotation) — rotates the whole
 * rendered image 180° so it reads right-side up when the panel itself is mounted rotated
 * in its enclosure.
 *
 * Requires LVGL to be initialized, which CONFIG_LV_Z_AUTO_INIT does before main().
 * Not callable from the native host tests (tests/run.sh) — it needs LVGL. */
void rasterizer_render(uint8_t *framebuffer, size_t framebuffer_len, const layout_t *layout,
			bool rotate_180);

#endif /* RASTERIZER_H_ */
