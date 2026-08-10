/*
 * Custom GATT service (spec 4.2): status (read/notify), button_event (read),
 * data_transfer (write), command (write). The UUIDs are defined in ble_service.c and
 * duplicated in gateway/gateway/uuids.py, which is how the gateway locates the service
 * and its characteristics — the two must change together, and changing them means
 * reflashing before the gateway will see the device again.
 */

#ifndef BLE_SERVICE_H_
#define BLE_SERVICE_H_

#include <stdint.h>

enum ble_service_event {
	BLE_SERVICE_EVENT_CONNECTED,
	BLE_SERVICE_EVENT_DISCONNECTED,
};

typedef void (*ble_service_event_cb_t)(enum ble_service_event event);

/* Command characteristic values (spec 4.2). */
#define BLE_SERVICE_COMMAND_FORCE_FULL_REFRESH 0x01
#define BLE_SERVICE_COMMAND_SLEEP_NOW 0x02
#define BLE_SERVICE_COMMAND_IDENTIFY 0x03
/* Per-display mounting orientation (epaper_set_rotation) — mirrors
 * gateway/gateway/uuids.py's COMMAND_ROTATE_NORMAL/COMMAND_ROTATE_180. Sent by the
 * gateway on every sync (see sync.py), not just when it changes. */
#define BLE_SERVICE_COMMAND_ROTATE_NORMAL 0x04
#define BLE_SERVICE_COMMAND_ROTATE_180 0x05

/* Unlike the plain single-byte commands above, this one's write carries a 2-byte
 * little-endian value (seconds) after the command byte, so write_command's usual
 * `len != 1` rejection doesn't apply to it — see write_command's switch in
 * ble_service.c. Mirrors gateway/gateway/uuids.py's COMMAND_SET_WAKE_INTERVAL_S; sent
 * by the gateway on every sync, same self-healing reasoning as ROTATE_* above. */
#define BLE_SERVICE_COMMAND_SET_WAKE_INTERVAL_S 0x06

/* main.c's DEEP_SLEEP duration (ble_service_get_wake_interval_s()) is clamped to this
 * range on write rather than rejected — an out-of-range request still leaves the
 * display in a working state instead of erroring the whole command write. MAX also
 * bounds WATCHDOG_TIMEOUT_MS (main.c), so raising it means accepting a looser watchdog
 * window for every cycle, not just long-interval ones. */
#define BLE_SERVICE_MIN_WAKE_INTERVAL_S 5
#define BLE_SERVICE_MAX_WAKE_INTERVAL_S 3600
#define BLE_SERVICE_DEFAULT_WAKE_INTERVAL_S 15

int ble_service_init(void);

/* Called once from main; fires on every connect/disconnect so main.c's state machine
 * can transition ADVERTISING -> SYNCING -> DEEP_SLEEP without polling. */
void ble_service_set_event_callback(ble_service_event_cb_t cb);

int ble_service_start_advertising(void);
int ble_service_stop_advertising(void);

/* Updates what the `status` characteristic reports on its next read. Called by main.c
 * once a data_transfer message has been applied, and by battery.c after each reading. */
void ble_service_set_content_hash(uint32_t content_hash);
void ble_service_set_battery(uint8_t battery_pct, uint16_t battery_mv);

/* Read by main.c's DEEP_SLEEP loop on every cycle. Starts at
 * BLE_SERVICE_DEFAULT_WAKE_INTERVAL_S and is updated in place by an incoming
 * BLE_SERVICE_COMMAND_SET_WAKE_INTERVAL_S write (write_command in ble_service.c) — not
 * persisted across a power cycle, same as rotate_180's RAM-only state, since the
 * gateway re-asserts both every sync. */
uint16_t ble_service_get_wake_interval_s(void);

/* button_event characteristic: 1 byte, bit i (0-4) = checklist row i's button pressed
 * since the mask was last read. Read-only, atomically cleared on read. No setter here —
 * unlike status/battery, its read callback calls button_consume_pending_mask() (see
 * button.h) directly at read time, so there's no cached value for main.c to push. */

#endif /* BLE_SERVICE_H_ */
