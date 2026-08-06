/*
 * Duty-cycle state machine (spec 4.1):
 *
 *   DEEP_SLEEP --RTC wake--> ADVERTISING --connected--> SYNCING --done--> DEEP_SLEEP
 *        ^                        |                                          |
 *        '---- advertising window elapses, no connection --------------------'
 *
 * SYNCING itself has no code here — it's entirely driven by the GATT read/write
 * callbacks in ble_service.c while a connection is open. This file only needs to know
 * when a cycle has ended (on_ble_event's DISCONNECTED case) to go back to sleep.
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "battery.h"
#include "ble_service.h"
#include "button.h"
#include "epaper.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define FW_VERSION 1

/* Not yet Kconfig options — see firmware/README.md item 4. Tune once real battery-life
 * data exists (spec 8 leaves the exact interval open on purpose). */
#define APP_WAKE_INTERVAL_S 120
#define APP_ADVERTISING_WINDOW_MS 4000

/* Generous enough to cover one full advertising+sync cycle plus slack, short enough to
 * catch a genuinely stuck SYNCING phase (spec 4.1's watchdog requirement). Fed exactly
 * once per completed cycle, right before going back to sleep — not continuously — so a
 * hang anywhere in a cycle (not just SYNCING) eventually forces the reset spec 4.1 asks
 * for. The nRF's watchdog can't be paused once started, so "feed once per full cycle"
 * is the only way to get a timeout that tolerates the intentional long sleep but still
 * catches a stuck cycle. */
#define WATCHDOG_TIMEOUT_MS (2 * APP_WAKE_INTERVAL_S * 1000)

static K_SEM_DEFINE(sync_done_sem, 0, 1);

/* Given by a button ISR (button.c) on any accepted press, so DEEP_SLEEP can end early
 * instead of always waiting the full RTC interval — see the sleep call at the bottom of
 * the main loop below. */
static K_SEM_DEFINE(wake_sem, 0, 1);

static const struct device *wdt_dev;
static int wdt_channel_id = -1;

static void on_ble_event(enum ble_service_event event)
{
	if (event == BLE_SERVICE_EVENT_DISCONNECTED) {
		/* Clean or unexpected disconnect — either way, spec 4.1 says always
		 * return to DEEP_SLEEP, never leave the radio in an ambiguous state. */
		k_sem_give(&sync_done_sem);
	}
}

static int watchdog_setup(void)
{
	wdt_dev = DEVICE_DT_GET_ANY(nordic_nrf_watchdog);
	if (wdt_dev == NULL || !device_is_ready(wdt_dev)) {
		LOG_ERR("watchdog device not ready");
		return -ENODEV;
	}

	struct wdt_timeout_cfg wdt_config = {
		.window.min = 0,
		.window.max = WATCHDOG_TIMEOUT_MS,
		.callback = NULL, /* no callback — let it reset the chip directly */
		.flags = WDT_FLAG_RESET_SOC,
	};

	wdt_channel_id = wdt_install_timeout(wdt_dev, &wdt_config);
	if (wdt_channel_id < 0) {
		LOG_ERR("failed to install watchdog timeout (%d)", wdt_channel_id);
		return wdt_channel_id;
	}

	return wdt_setup(wdt_dev, 0);
}

static void watchdog_feed(void)
{
	if (wdt_dev != NULL && wdt_channel_id >= 0) {
		wdt_feed(wdt_dev, wdt_channel_id);
	}
}

static void advertise_and_wait_for_sync(void)
{
	int err = ble_service_start_advertising();
	if (err) {
		LOG_ERR("failed to start advertising (%d)", err);
		return;
	}

	k_sem_reset(&sync_done_sem);
	int took_sem = k_sem_take(&sync_done_sem, K_MSEC(APP_ADVERTISING_WINDOW_MS));

	if (took_sem != 0) {
		/* Window elapsed with nobody connecting — stop advertising ourselves.
		 * If a connection *did* happen, advertising already stopped on connect
		 * (BT_LE_ADV_CONN_FAST_1's behavior) and sync_done_sem was given by
		 * on_ble_event() once the gateway disconnected. */
		ble_service_stop_advertising();
	}
}

int main(void)
{
	LOG_INF("homescreen display firmware starting (fw_version=%d)", FW_VERSION);

	if (watchdog_setup() != 0) {
		LOG_WRN("continuing without watchdog protection");
	}

	battery_init();
	epaper_init();
	if (button_init(&wake_sem) != 0) {
		LOG_WRN("continuing without checklist buttons");
	}

	int err = ble_service_init();
	if (err) {
		LOG_ERR("bt_enable failed (%d) - halting", err);
		return err;
	}
	ble_service_set_event_callback(on_ble_event);

	while (1) {
		uint8_t battery_pct;
		uint16_t battery_mv;
		if (battery_read(&battery_pct, &battery_mv) == 0) {
			ble_service_set_battery(battery_pct, battery_mv);
		} else {
			LOG_WRN("battery read failed, status will report last known value");
		}

		advertise_and_wait_for_sync();

		/* Cycle complete, synced or not — feed the watchdog and go back to
		 * sleep. Sleeping via k_sem_take (not a plain k_msleep) lets a checklist
		 * button press end DEEP_SLEEP early instead of always waiting the full RTC
		 * interval, while still relying on CONFIG_PM's idle thread to select the
		 * SoC's deepest RTC-wake-capable sleep state for however long we do wait;
		 * see prj.conf for why this isn't a manual sys_poweroff()/true System OFF
		 * call — that distinction is also what makes a plain GPIO interrupt enough
		 * to wake us here, with no extra devicetree "wakeup-source" plumbing. */
		watchdog_feed();
		k_sem_reset(&wake_sem);
		k_sem_take(&wake_sem, K_MSEC(APP_WAKE_INTERVAL_S * 1000));
	}

	return 0;
}
