/*
 * Custom GATT service (spec 4.2): status (read/notify), data_transfer (write),
 * command (write). UUIDs here are a placeholder base — generate a real one
 * (`uuidgen` or similar) before this ships; see firmware/README.md.
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

#endif /* BLE_SERVICE_H_ */
