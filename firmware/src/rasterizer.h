/*
 * Minimal v1 rasterizer (see the plan for this feature / firmware/README.md): draws the
 * layout_store's retained elements into a 1bpp framebuffer. Deliberately narrow scope —
 * fixed-width bitmap font (font_basic.h), plain left-aligned text, no word-wrap, no
 * bold/underline/other props, no partial-refresh optimization (every apply is a full
 * redraw). Extending this to the design editor's full text styling is future work.
 */

#ifndef RASTERIZER_H_
#define RASTERIZER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "layout_store.h"

/* Clears framebuffer to all-white, then draws every element in `layout`: text left-
 * aligned at (x, y), and for checked+checkable elements, a horizontal strikethrough
 * line sized to that element's actual rendered text width. framebuffer must be at least
 * EPD_FRAMEBUFFER_SIZE (epaper_ssd1683.h) bytes, 1bpp MSB-first, bit=1 white/bit=0
 * black per the SSD1683's Write RAM(BW) convention — matches what epd_ssd1683_push_full
 * expects directly.
 *
 * rotate_180: per-display mounting setting (see epaper_set_rotation) — rotates the
 * whole rendered image 180° so it reads right-side up when the panel itself is mounted
 * rotated in its enclosure. */
void rasterizer_render(uint8_t *framebuffer, size_t framebuffer_len, const layout_t *layout,
			bool rotate_180);

#endif /* RASTERIZER_H_ */
