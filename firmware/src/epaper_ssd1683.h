/*
 * Low-level SSD1683 panel driver — the actual v1 hardware: a 4.2" 400x300 monochrome
 * eInk panel on the Seeed ePaper Driver Board for XIAO. Command sequence verified
 * against the SSD1683 datasheet (Solomon Systech, Rev 1.0, Jan 2021); electrically
 * untested (no hardware available to flash — see firmware/README.md).
 *
 * Deliberately Zephyr-independent at the header level (no zephyr headers included) so
 * epaper.c can include this and stay natively testable (firmware/tests/) — only
 * epaper_ssd1683.c pulls in real SPI/GPIO driver headers. The native test build links
 * against tests/native_epd_stub.c instead of this file's real implementation.
 */

#ifndef EPAPER_SSD1683_H_
#define EPAPER_SSD1683_H_

#include <stddef.h>
#include <stdint.h>

#define EPD_WIDTH 400
#define EPD_HEIGHT 300
#define EPD_WIDTH_BYTES (EPD_WIDTH / 8)
#define EPD_FRAMEBUFFER_SIZE (EPD_WIDTH_BYTES * EPD_HEIGHT)

/* Resets and initializes the controller (spec 4.1 calls this out as part of firmware
 * bring-up). Returns 0 on success. */
int epd_ssd1683_init(void);

/* Pushes an EPD_FRAMEBUFFER_SIZE-byte 1bpp buffer (bit=1 -> white, bit=0 -> black, per
 * the datasheet's Write RAM(BW) convention; MSB-first within each byte, packed 8
 * horizontal pixels per byte) as a full refresh. Blocks until the panel's BUSY line
 * clears. Returns 0 on success. */
int epd_ssd1683_push_full(const uint8_t *framebuffer, size_t len);

/* Physically identifies this display (full black, then full white) so it's easy to
 * spot among several — backs the `command` characteristic's 0x03 IDENTIFY (spec 4.2). */
int epd_ssd1683_identify(void);

/* Puts the controller into deep sleep between duty cycles. Not currently called from
 * main.c's loop (the whole SoC goes to sleep between cycles per spec 4.1, taking the
 * SPI bus/GPIOs down with it) — kept for whoever tunes power behavior with real
 * battery-life data. */
int epd_ssd1683_sleep(void);

#endif /* EPAPER_SSD1683_H_ */
