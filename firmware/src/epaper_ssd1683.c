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

/* How long to give BUSY to *rise* after an operation is started, before concluding it
 * never will. The SSD1683 asserts BUSY within microseconds of Master Activation, so this
 * only has to cover GPIO/scheduling slop, not the operation itself. */
#define BUSY_RISE_TIMEOUT_MS 50

/* Fallback wait when BUSY never rises at all — long enough for the datasheet's worst-case
 * full refresh, since without a working BUSY line there is nothing to poll and returning
 * early corrupts the *next* operation. */
#define BUSY_BLIND_WAIT_MS 4000

/**
 * Wait for the panel to finish whatever it is doing.
 *
 * @param expect_assert true only for operations the controller signals with a
 *        multi-second BUSY assert — in practice just Master Activation (a display
 *        refresh). Those get a blind-wait safety net if BUSY somehow fails to rise,
 *        because returning early there corrupts the *next* operation.
 *
 *        false for everything else: hardware/software reset and register writes. Those
 *        either finish in single-digit milliseconds or never drive BUSY at all, so
 *        "BUSY isn't high" legitimately means "already idle, carry on". Blind-waiting
 *        those cost 8 seconds of every boot for no benefit.
 */
static void epd_wait_busy(bool expect_assert)
{
	/* BUSY is GPIO_ACTIVE_HIGH in the overlay, matching the datasheet: BUSY pad
	 * outputs high during every long-running operation (master activation, SW
	 * reset, etc.) and low once ready.
	 *
	 * This used to be a bare `while (busy) sleep`, which silently degenerated into a
	 * no-op: if BUSY hadn't been sampled as high yet, the loop exited immediately and
	 * the caller carried on mid-refresh. That is invisible when pushes are seconds
	 * apart (the gateway's 15s wake cycle), and fatal when they aren't — two pushes
	 * back-to-back at boot produced a blank panel, because the second one's Write RAM
	 * + Master Activation landed while the first refresh was still running and the
	 * controller discarded it. So: wait for the rising edge first, then the falling
	 * one. */
	int64_t start = k_uptime_get();
	bool rose = false;

	while (k_uptime_get() - start < BUSY_RISE_TIMEOUT_MS) {
		if (gpio_pin_get_dt(&busy_gpio) > 0) {
			rose = true;
			break;
		}
		k_msleep(1);
	}

	if (!rose) {
		if (!expect_assert) {
			/* Normal and expected: a reset that completed inside the settling
			 * delay before we started polling, or a run of register writes that
			 * never drives BUSY at all. The panel is idle; carry on. */
			LOG_DBG("epaper_ssd1683: BUSY idle, nothing to wait for");
			return;
		}
		/* A refresh was started but BUSY never rose — that should be impossible on a
		 * working board, since a full update holds BUSY high for seconds. Wait out a
		 * worst-case refresh blind rather than letting the next Write RAM land
		 * mid-update, which is what produced a blank panel before the D5 fix. */
		LOG_WRN("epaper_ssd1683: BUSY never asserted after a refresh within %dms — "
			"waiting %dms blind (check busy-gpios in boards/xiao_ble.overlay)",
			BUSY_RISE_TIMEOUT_MS, BUSY_BLIND_WAIT_MS);
		k_msleep(BUSY_BLIND_WAIT_MS);
		return;
	}

	while (gpio_pin_get_dt(&busy_gpio) > 0) {
		if (k_uptime_get() - start > BUSY_TIMEOUT_MS) {
			LOG_WRN("epaper_ssd1683: BUSY timeout after %dms", BUSY_TIMEOUT_MS);
			return;
		}
		k_msleep(5);
	}

	LOG_DBG("epaper_ssd1683: busy for %lldms", k_uptime_get() - start);
}

/* Boot-time health check on the BUSY line. This is what found the pin was wrong: it
 * reported 0 high samples out of 978 across a software reset, which is not something a
 * timing race can explain, and BUSY turned out to be on D5 rather than the D2 Seeed's
 * published pin table claimed (see boards/xiao_ble.overlay). On a correctly wired board
 * it now reports a ~5ms assert.
 *
 * A software reset (0x12) is the cheapest operation the datasheet says drives BUSY high,
 * so issue one and sample the pin hard for a short window. Three outcomes:
 *   - brief assert : healthy — this is the expected result.
 *   - stuck low    : wrong pin in the overlay, or the trace isn't connected.
 *   - stuck high   : reads asserted even at rest — likely floating, or an active-low
 *                    BUSY, meaning the overlay's GPIO_ACTIVE_HIGH flag is wrong.
 *
 * Kept permanently rather than deleted once fixed: it costs one extra SW reset at init
 * (which the init sequence performs anyway), and it turns "the panel is blank" into a
 * one-line diagnosis instead of the several flash-and-squint cycles it took this time. */
static void epd_probe_busy_line(void)
{
	int before = gpio_pin_get_dt(&busy_gpio);
	int high_samples = 0;
	int total_samples = 0;
	int64_t first_high_ms = -1;
	int64_t start = k_uptime_get();

	epd_cmd(0x12); /* SW RESET — datasheet: BUSY is driven high for its duration */

	while (k_uptime_get() - start < 200) {
		total_samples++;
		if (gpio_pin_get_dt(&busy_gpio) > 0) {
			high_samples++;
			if (first_high_ms < 0) {
				first_high_ms = k_uptime_get() - start;
			}
		}
		k_busy_wait(200); /* tight poll — a busy pulse could be sub-millisecond */
	}

	LOG_INF("epaper_ssd1683: BUSY probe — before=%d, high on %d/%d samples over 200ms, "
		"first high at %lldms",
		before, high_samples, total_samples, first_high_ms);

	if (high_samples == 0) {
		LOG_WRN("epaper_ssd1683: BUSY never asserted during a SW reset — check "
			"busy-gpios (D2/P0.28) in boards/xiao_ble.overlay against the board");
	}

	/* Leave the controller settled before the caller's real init sequence runs. */
	k_msleep(50);
}

static void epd_hw_reset(void)
{
	/* RST is GPIO_ACTIVE_LOW in the overlay: logical 1 asserts (drives physically
	 * low), logical 0 releases. */
	gpio_pin_set_dt(&rst_gpio, 1);
	k_msleep(10);
	gpio_pin_set_dt(&rst_gpio, 0);
	k_msleep(10);
	epd_wait_busy(false);
}

static int epd_init_sequence(void)
{
	epd_hw_reset();

	epd_cmd(0x12); /* SW RESET */
	epd_wait_busy(false);

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

	epd_wait_busy(false);
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
	epd_wait_busy(true);
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

	epd_probe_busy_line();
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
