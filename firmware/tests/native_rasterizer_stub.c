/*
 * Host-native stand-in for src/rasterizer.c, which cannot be compiled here: it renders
 * through LVGL and the generated fonts, and neither is available to the system compiler
 * used by run.sh.
 *
 * test_chunk_protocol.c links epaper.c, which calls rasterizer_render() on every apply.
 * The tests there are about chunk reassembly, CRC, and the layout store — not pixels — so
 * this only has to satisfy the linker and leave the framebuffer in the same all-white
 * state a real render starts from.
 */

#include "rasterizer.h"

#include <string.h>

#include "epaper_ssd1683.h"

void rasterizer_render(uint8_t *framebuffer, size_t framebuffer_len, const layout_t *layout,
			bool rotate_180)
{
	(void)layout;
	(void)rotate_180;
	memset(framebuffer, 0xFF, framebuffer_len); /* 1 = white, as the real renderer does */
}
