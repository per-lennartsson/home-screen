/*
 * Native-test-only stand-in for epaper_ssd1683.c (the real SPI/GPIO driver, not
 * compiled here — see firmware/tests/run.sh). Lets epaper.c's protocol-decoding logic
 * link and run on the host without pulling in Zephyr's driver subsystem.
 */
#include "epaper_ssd1683.h"

int epd_ssd1683_init(void)
{
	return 0;
}

int epd_ssd1683_push_full(const uint8_t *framebuffer, size_t len)
{
	(void)framebuffer;
	return (len == EPD_FRAMEBUFFER_SIZE) ? 0 : -1;
}

int epd_ssd1683_identify(void)
{
	return 0;
}

int epd_ssd1683_sleep(void)
{
	return 0;
}
