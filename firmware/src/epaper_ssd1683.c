/*
 * SSD1683 panel driver — real hardware, electrically untested. Command byte values are
 * transcribed from the SSD1683 datasheet's command table (Solomon Systech, Rev 1.0, Jan
 * 2021, sections 7-8); see the comments on each command for the exact register meaning.
 * Border waveform (0x3C = 0x05 for full refresh) is a commonly-used value from
 * third-party SSD1683 reference implementations rather than something the datasheet
 * itself prescribes as the one correct choice — flagged specifically because it's the
 * one register value here I couldn't pin to the primary source.
 *
 * Hardware wiring: boards/xiao_ble.overlay (Seeed ePaper Driver Board pin table +
 * XIAO nRF52840 D-pin mapping).
 */

#include "epaper_ssd1683.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(epaper_ssd1683, CONFIG_LOG_DEFAULT_LEVEL);

#define GPIO_NODE DT_PATH(zephyr_user)
#define SPI_NODE DT_NODELABEL(epd)

#if DT_NODE_HAS_STATUS(SPI_NODE, okay) && DT_NODE_HAS_PROP(GPIO_NODE, dc_gpios) &&                \
	DT_NODE_HAS_PROP(GPIO_NODE, rst_gpios) && DT_NODE_HAS_PROP(GPIO_NODE, busy_gpios)
#define HAVE_EPD_HARDWARE 1

static const struct spi_dt_spec epd_spi =
	SPI_DT_SPEC_GET(SPI_NODE, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0);
static const struct gpio_dt_spec dc_gpio = GPIO_DT_SPEC_GET(GPIO_NODE, dc_gpios);
static const struct gpio_dt_spec rst_gpio = GPIO_DT_SPEC_GET(GPIO_NODE, rst_gpios);
static const struct gpio_dt_spec busy_gpio = GPIO_DT_SPEC_GET(GPIO_NODE, busy_gpios);

#define BUSY_TIMEOUT_MS 10000 /* generous — a full refresh typically takes 1-2s */

static int epd_write_bytes(bool is_data, const uint8_t *buf, size_t len)
{
	/* DC is GPIO_ACTIVE_HIGH in the overlay, matching the datasheet directly:
	 * D/C#=low selects command, D/C#=high selects data. */
	gpio_pin_set_dt(&dc_gpio, is_data ? 1 : 0);

	struct spi_buf tx_buf = {.buf = (void *)buf, .len = len};
	struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	return spi_write_dt(&epd_spi, &tx);
}

static inline int epd_cmd(uint8_t cmd)
{
	return epd_write_bytes(false, &cmd, 1);
}

static inline int epd_data(const uint8_t *data, size_t len)
{
	return epd_write_bytes(true, data, len);
}

static inline int epd_data1(uint8_t byte)
{
	return epd_data(&byte, 1);
}

static void epd_wait_busy(void)
{
	/* BUSY is GPIO_ACTIVE_HIGH in the overlay, matching the datasheet: BUSY pad
	 * outputs high during every long-running operation (master activation, SW
	 * reset, etc.) and low once ready. */
	int64_t start = k_uptime_get();
	while (gpio_pin_get_dt(&busy_gpio) > 0) {
		if (k_uptime_get() - start > BUSY_TIMEOUT_MS) {
			LOG_WRN("epaper_ssd1683: BUSY timeout after %dms", BUSY_TIMEOUT_MS);
			return;
		}
		k_msleep(5);
	}
}

static void epd_hw_reset(void)
{
	/* RST is GPIO_ACTIVE_LOW in the overlay: logical 1 asserts (drives physically
	 * low), logical 0 releases. */
	gpio_pin_set_dt(&rst_gpio, 1);
	k_msleep(10);
	gpio_pin_set_dt(&rst_gpio, 0);
	k_msleep(10);
	epd_wait_busy();
}

static int epd_init_sequence(void)
{
	epd_hw_reset();

	epd_cmd(0x12); /* SW RESET */
	epd_wait_busy();

	/* Driver Output Control (0x01): MUX[8:0] = EPD_HEIGHT-1 = 299 = 0x12B, gate
	 * scan defaults (GD=0, SM=0, TB=0). */
	epd_cmd(0x01);
	epd_data1(0x2B);
	epd_data1(0x01);
	epd_data1(0x00);

	/* Data Entry Mode Setting (0x11): ID[1:0]=11 (Y increment, X increment),
	 * AM=0 (address counter updates in X direction) — datasheet POR default,
	 * set explicitly rather than relied on. */
	epd_cmd(0x11);
	epd_data1(0x03);

	/* Set RAM X-address Start/End (0x44), in 8-pixel units: 0 to
	 * (EPD_WIDTH/8)-1 = 49 = 0x31. */
	epd_cmd(0x44);
	epd_data1(0x00);
	epd_data1(0x31);

	/* Set RAM Y-address Start/End (0x45), per-line: 0 to EPD_HEIGHT-1 = 299 =
	 * 0x12B, each bound split into (low byte, high bit) pairs. */
	epd_cmd(0x45);
	epd_data1(0x00);
	epd_data1(0x00);
	epd_data1(0x2B);
	epd_data1(0x01);

	/* Border Waveform Control (0x3C) — see file header comment: this specific
	 * value is from common third-party reference implementations, not something
	 * pinned in the primary datasheet source used for the rest of this driver. */
	epd_cmd(0x3C);
	epd_data1(0x05);

	/* Temperature Sensor Control (0x18): use the internal sensor (0x80) — no
	 * external sensor is wired on this board. */
	epd_cmd(0x18);
	epd_data1(0x80);

	/* Set RAM X/Y address counters (0x4E/0x4F) to the window origin. */
	epd_cmd(0x4E);
	epd_data1(0x00);
	epd_cmd(0x4F);
	epd_data1(0x00);
	epd_data1(0x00);

	epd_wait_busy();
	return 0;
}

static int epd_display_refresh(void)
{
	/* Display Update Control 2 (0x22) = 0xFF: the datasheet's full black/white
	 * sequence — enable clock, enable analog, load temperature value, load LUT,
	 * DISPLAY, disable analog, disable OSC. Then Master Activation (0x20) starts
	 * it; BUSY stays high until the refresh completes. */
	epd_cmd(0x22);
	epd_data1(0xFF);
	epd_cmd(0x20);
	epd_wait_busy();
	return 0;
}

int epd_ssd1683_init(void)
{
	if (!spi_is_ready_dt(&epd_spi)) {
		LOG_ERR("epaper_ssd1683: SPI device not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&dc_gpio) || !gpio_is_ready_dt(&rst_gpio) ||
	    !gpio_is_ready_dt(&busy_gpio)) {
		LOG_ERR("epaper_ssd1683: control GPIOs not ready");
		return -ENODEV;
	}

	int err = gpio_pin_configure_dt(&dc_gpio, GPIO_OUTPUT_INACTIVE);
	err |= gpio_pin_configure_dt(&rst_gpio, GPIO_OUTPUT_INACTIVE);
	err |= gpio_pin_configure_dt(&busy_gpio, GPIO_INPUT);
	if (err) {
		LOG_ERR("epaper_ssd1683: GPIO configuration failed (%d)", err);
		return err;
	}

	return epd_init_sequence();
}

int epd_ssd1683_push_full(const uint8_t *framebuffer, size_t len)
{
	if (len != EPD_FRAMEBUFFER_SIZE) {
		LOG_ERR("epaper_ssd1683: framebuffer must be exactly %d bytes, got %zu",
			EPD_FRAMEBUFFER_SIZE, len);
		return -EINVAL;
	}

	epd_cmd(0x24); /* Write RAM (Black/White) */
	epd_data(framebuffer, len);

	return epd_display_refresh();
}

int epd_ssd1683_identify(void)
{
	/* static, not a local: EPD_FRAMEBUFFER_SIZE is 15000 bytes, which overflows every
	 * thread stack in this app (BT RX 1K, system workqueue 2K) many times over. As a
	 * local this reliably tripped the MPU stack guard and reset the SoC the moment the
	 * identify command arrived — and because the USB CDC console dies with the fault,
	 * the reset looked like an unexplained reboot with no fault dump. Safe as a static
	 * because epaper work is only ever submitted to the single-threaded epaper
	 * workqueue (see ble_service.c), so there's no concurrent caller to race with. */
	static uint8_t buf[EPD_FRAMEBUFFER_SIZE];

	memset(buf, 0x00, sizeof(buf)); /* all black */
	int err = epd_ssd1683_push_full(buf, sizeof(buf));
	if (err) {
		return err;
	}

	k_msleep(500);

	memset(buf, 0xFF, sizeof(buf)); /* all white */
	return epd_ssd1683_push_full(buf, sizeof(buf));
}

int epd_ssd1683_sleep(void)
{
	/* Deep Sleep Mode (0x10), mode 1 (A[1:0]=01): lowest power state that still
	 * retains RAM content. Datasheet: exiting requires a hardware reset. */
	epd_cmd(0x10);
	epd_data1(0x01);
	return 0;
}

#else /* !HAVE_EPD_HARDWARE */

int epd_ssd1683_init(void)
{
	LOG_WRN("epaper_ssd1683: no SPI/GPIO devicetree nodes found for this board — "
		"see boards/xiao_ble.overlay. All operations are no-ops.");
	return 0;
}

int epd_ssd1683_push_full(const uint8_t *framebuffer, size_t len)
{
	ARG_UNUSED(framebuffer);
	if (len != EPD_FRAMEBUFFER_SIZE) {
		return -EINVAL;
	}
	return 0;
}

int epd_ssd1683_identify(void)
{
	return 0;
}

int epd_ssd1683_sleep(void)
{
	return 0;
}

#endif /* HAVE_EPD_HARDWARE */
