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
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "battery.h"
#include "ble_service.h"
#include "button.h"
#include "epaper.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define FW_VERSION 1

/* Not yet a Kconfig option — see firmware/README.md item 1. */
#define APP_ADVERTISING_WINDOW_MS 4000

/* DEEP_SLEEP's duration itself is no longer a compile-time constant here — it's
 * ble_service_get_wake_interval_s(), runtime-configurable via
 * BLE_SERVICE_COMMAND_SET_WAKE_INTERVAL_S (ble_service.h) so the backend/gateway can
 * tune it per display without reflashing. It starts at
 * BLE_SERVICE_DEFAULT_WAKE_INTERVAL_S — a bring-up value, not a battery-life one:
 * content takes two wake cycles to converge (docs/protocol.md, "Sync latency"), so at a
 * much longer interval every test iteration would cost several minutes.
 *
 * Generous enough to cover one full advertising+sync cycle plus slack, short enough to
 * catch a genuinely stuck SYNCING phase (spec 4.1's watchdog requirement). Fed exactly
 * once per completed cycle, right before going back to sleep — not continuously — so a
 * hang anywhere in a cycle (not just SYNCING) eventually forces the reset spec 4.1 asks
 * for. The nRF's watchdog timeout can't be changed once wdt_setup() runs (its CRV
 * register is write-once-per-reset), so this is sized off
 * BLE_SERVICE_MAX_WAKE_INTERVAL_S — the worst case the wake interval could ever be set
 * to — rather than whatever it's currently configured to. That means a short-interval
 * display's watchdog window is looser than the "2x this cycle" reasoning alone would
 * give it, but a wake interval that long is itself an intentional long sleep, not a
 * hang, so a loose bound there costs nothing but watchdog precision. */
#define WATCHDOG_TIMEOUT_MS (2 * BLE_SERVICE_MAX_WAKE_INTERVAL_S * 1000)

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

/* Logs why the last reset happened, then clears the cause register so the next one is
 * unambiguous. On the nRF52840, a value of 0 (none of the named bits set) means POR or
 * brownout — the SoC doesn't distinguish those two from each other, only from
 * watchdog/software/lockup resets, which do set a bit. Worth keeping permanently: a
 * battery-powered device that reboots in the field leaves nothing else to go on, and an
 * MPU stack-guard fault (see epd_ssd1683_identify) kills the USB console before it can
 * flush the fault dump, so this line is often the only surviving evidence. */
static void log_reset_cause(void)
{
	uint32_t cause = 0;

	hwinfo_get_reset_cause(&cause);
	hwinfo_clear_reset_cause();

	LOG_INF("reset cause: 0x%08x%s%s%s%s%s%s", cause,
		(cause & RESET_PIN) ? " PIN" : "", (cause & RESET_SOFTWARE) ? " SOFTWARE" : "",
		(cause & RESET_WATCHDOG) ? " WATCHDOG" : "",
		(cause & RESET_CPU_LOCKUP) ? " CPU_LOCKUP" : "",
		(cause & RESET_LOW_POWER_WAKE) ? " LOW_POWER_WAKE" : "",
		(cause == 0) ? " (none set -> POR or brownout)" : "");
}

/* How long to wait at boot for a host to open the USB CDC console (prj.conf's USB block).
 * Bounded, and deliberately short: in normal service this display runs on a battery with
 * nothing plugged into USB, where DTR never asserts and this wait is dead time on every
 * reset. Long enough for macOS to enumerate the device and a terminal to attach when
 * someone is actually debugging, negligible otherwise. Kept in step with
 * CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS so the buffered boot log flushes just after
 * the terminal is attached rather than just before. */
#define CONSOLE_WAIT_TIMEOUT_MS 2000
#define CONSOLE_WAIT_POLL_MS 50

static void wait_for_console(void)
{
	const struct device *console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(console)) {
		return;
	}

	for (int waited = 0; waited < CONSOLE_WAIT_TIMEOUT_MS; waited += CONSOLE_WAIT_POLL_MS) {
		uint32_t dtr = 0;

		if (uart_line_ctrl_get(console, UART_LINE_CTRL_DTR, &dtr) == 0 && dtr) {
			return;
		}
		k_msleep(CONSOLE_WAIT_POLL_MS);
	}
}

int main(void)
{
	wait_for_console();

	LOG_INF("homescreen display firmware starting (fw_version=%d)", FW_VERSION);
	log_reset_cause();

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
		k_sem_take(&wake_sem, K_MSEC(ble_service_get_wake_interval_s() * 1000));
	}

	return 0;
}
