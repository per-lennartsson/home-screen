#include "battery.h"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(battery, CONFIG_LOG_DEFAULT_LEVEL);

/* Wired up in boards/xiao_ble.overlay's `zephyr,user` node: io-channels = P0.31/AIN7
 * (the VBAT divider output), battery-enable-gpios = P0.14 (the divider's active-low
 * enable pin) — see that file's header comment for the hardware sourcing. Falls back to
 * a placeholder reading on any other board until a matching overlay exists. */
#define BATTERY_NODE DT_PATH(zephyr_user)

#if DT_NODE_HAS_PROP(BATTERY_NODE, io_channels)
static const struct adc_dt_spec battery_adc = ADC_DT_SPEC_GET(BATTERY_NODE);
#define HAVE_BATTERY_ADC 1
#endif

#if DT_NODE_HAS_PROP(BATTERY_NODE, battery_enable_gpios)
static const struct gpio_dt_spec battery_enable =
	GPIO_DT_SPEC_GET(BATTERY_NODE, battery_enable_gpios);
#define HAVE_BATTERY_ENABLE 1
#endif

/* BQ25101 CHG output via the XIAO module's own wiring (see xiao_ble.overlay) — absent
 * on any board whose overlay doesn't define it, same "degrade instead of failing to
 * build" treatment as HAVE_BATTERY_ADC/HAVE_BATTERY_ENABLE above. */
#if DT_NODE_HAS_PROP(BATTERY_NODE, charge_status_gpios)
static const struct gpio_dt_spec charge_status =
	GPIO_DT_SPEC_GET(BATTERY_NODE, charge_status_gpios);
#define HAVE_CHARGE_STATUS 1
#endif

/* BQ25101 ISET select via the XIAO module's HICHG pin (see xiao_ble.overlay) — same
 * "degrade instead of failing to build" treatment as the others above. Absent, the
 * charger just stays at its power-on-default 50 mA rate. */
#if DT_NODE_HAS_PROP(BATTERY_NODE, charge_current_gpios)
static const struct gpio_dt_spec charge_current =
	GPIO_DT_SPEC_GET(BATTERY_NODE, charge_current_gpios);
#define HAVE_CHARGE_CURRENT 1
#endif

/* battery_voltage_mv = adc_reading_mv * VBAT_DIVIDER_RATIO. XIAO nRF52840's VBAT
 * divider is commonly cited as ~1/3 (documented as "divide by about 1/3" / ratio
 * 1510/510 on the Seeed forum) — treated as a placeholder ratio like any other board's
 * divider until measured against a real battery on real hardware. */
#define VBAT_DIVIDER_RATIO 3

/* Linear map between empty/full thresholds for a single-cell LiPo, in the same "tune
 * once real data exists" category as the wake interval (spec 8). */
#define BATTERY_EMPTY_MV 3300
#define BATTERY_FULL_MV 4200

int battery_init(void)
{
#if HAVE_BATTERY_ENABLE
	if (!gpio_is_ready_dt(&battery_enable)) {
		LOG_ERR("battery: enable GPIO not ready");
		return -ENODEV;
	}
	/* Configured once and left enabled permanently, never toggled per-read. A
	 * known XIAO nRF52840 hardware issue: leaving this pin (or misconfiguring it)
	 * high while P0.31 is set up as an analog input risks the ADC pin seeing the
	 * full battery rail past its input limit. Holding it low for the device's
	 * entire lifetime avoids that sequencing risk entirely, at the cost of the
	 * divider's small continuous current draw — an explicit tradeoff, not an
	 * oversight; revisit if that current cost turns out to matter against real
	 * battery-life data. */
	int err = gpio_pin_configure_dt(&battery_enable, GPIO_OUTPUT_ACTIVE);
	if (err) {
		LOG_ERR("battery: failed to enable VBAT divider (%d)", err);
		return err;
	}
#endif

#if HAVE_CHARGE_STATUS
	if (!gpio_is_ready_dt(&charge_status)) {
		LOG_ERR("battery: charge-status GPIO not ready");
		return -ENODEV;
	}
	/* Plain input, no pull — the charger IC actively drives this line (see
	 * xiao_ble.overlay), so an internal pull would only fight it. */
	int charge_err = gpio_pin_configure_dt(&charge_status, GPIO_INPUT);
	if (charge_err) {
		LOG_ERR("battery: failed to configure charge-status GPIO (%d)", charge_err);
		return charge_err;
	}
#endif

#if HAVE_CHARGE_CURRENT
	if (!gpio_is_ready_dt(&charge_current)) {
		LOG_ERR("battery: charge-current GPIO not ready");
		return -ENODEV;
	}
	/* Configured once and left driven low permanently, selecting the BQ25101's
	 * 100 mA rate instead of the 50 mA it powers on with — no reason to ever fall
	 * back to the slower one, so this never gets toggled per-read like the ADC
	 * divider enable above intentionally isn't either. */
	int charge_current_err = gpio_pin_configure_dt(&charge_current, GPIO_OUTPUT_ACTIVE);
	if (charge_current_err) {
		LOG_ERR("battery: failed to select charge current (%d)", charge_current_err);
		return charge_current_err;
	}
#endif

#if HAVE_BATTERY_ADC
	if (!adc_is_ready_dt(&battery_adc)) {
		LOG_ERR("battery: ADC device not ready");
		return -ENODEV;
	}
	return adc_channel_setup_dt(&battery_adc);
#else
	LOG_WRN("battery: no ADC channel in devicetree — battery_read() will report a "
		"fixed placeholder value until a board overlay defines one");
	return 0;
#endif
}

static uint8_t battery_pct_from_mv(uint16_t battery_mv)
{
	int32_t clamped_mv = battery_mv;

	if (clamped_mv < BATTERY_EMPTY_MV) {
		clamped_mv = BATTERY_EMPTY_MV;
	}
	if (clamped_mv > BATTERY_FULL_MV) {
		clamped_mv = BATTERY_FULL_MV;
	}
	return (uint8_t)(100 * (clamped_mv - BATTERY_EMPTY_MV) / (BATTERY_FULL_MV - BATTERY_EMPTY_MV));
}

int battery_read(uint8_t *out_battery_pct, uint16_t *out_battery_mv, bool *out_charging)
{
	uint16_t battery_mv;

#if HAVE_CHARGE_STATUS
	int charge_val = gpio_pin_get_dt(&charge_status);
	*out_charging = (charge_val > 0); /* ACTIVE_LOW spec: 1 = active = charging */
#else
	*out_charging = false;
#endif

#if HAVE_BATTERY_ADC
	uint16_t raw;
	struct adc_sequence sequence = {
		.buffer = &raw,
		.buffer_size = sizeof(raw),
	};

	int err = adc_sequence_init_dt(&battery_adc, &sequence);
	if (err) {
		return err;
	}

	err = adc_read_dt(&battery_adc, &sequence);
	if (err) {
		return err;
	}

	int32_t adc_mv = raw;
	err = adc_raw_to_millivolts_dt(&battery_adc, &adc_mv);
	if (err) {
		return err;
	}

	battery_mv = (uint16_t)(adc_mv * VBAT_DIVIDER_RATIO);
#else
	battery_mv = 3700; /* placeholder until a real ADC channel is wired up */
#endif

	*out_battery_mv = battery_mv;
	*out_battery_pct = battery_pct_from_mv(battery_mv);
	return 0;
}
