#include "ble_service.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "chunk_protocol.h"
#include "epaper.h"

LOG_MODULE_REGISTER(ble_service, CONFIG_LOG_DEFAULT_LEVEL);

#define FW_VERSION 1

/* Placeholder 128-bit base UUID (spec 4.2 explicitly calls this a placeholder pattern —
 * generate a real one before shipping, see firmware/README.md). Last 16 bits of the
 * final segment distinguish service (0000) from each characteristic. */
#define BT_UUID_HOMESCREEN_SERVICE_VAL                                                           \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abc0000)
#define BT_UUID_HOMESCREEN_STATUS_VAL                                                            \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abc0001)
#define BT_UUID_HOMESCREEN_DATA_TRANSFER_VAL                                                     \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abc0002)
#define BT_UUID_HOMESCREEN_COMMAND_VAL                                                           \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abc0003)

static struct bt_uuid_128 svc_uuid = BT_UUID_INIT_128(BT_UUID_HOMESCREEN_SERVICE_VAL);
static struct bt_uuid_128 status_uuid = BT_UUID_INIT_128(BT_UUID_HOMESCREEN_STATUS_VAL);
static struct bt_uuid_128 data_transfer_uuid =
	BT_UUID_INIT_128(BT_UUID_HOMESCREEN_DATA_TRANSFER_VAL);
static struct bt_uuid_128 command_uuid = BT_UUID_INIT_128(BT_UUID_HOMESCREEN_COMMAND_VAL);

static const uint8_t adv_service_uuid[] = {BT_UUID_HOMESCREEN_SERVICE_VAL};
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_UUID128_ALL, adv_service_uuid, sizeof(adv_service_uuid)),
};

struct __packed status_value {
	uint32_t content_hash;
	uint8_t battery_pct;
	uint16_t battery_mv;
	uint16_t fw_version;
};

static struct status_value current_status = {
	.content_hash = 0,
	.battery_pct = 0,
	.battery_mv = 0,
	.fw_version = FW_VERSION,
};

static chunk_reassembler_t reassembler;
static struct bt_conn *active_conn;
static ble_service_event_cb_t event_cb;

static ssize_t read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	struct status_value wire_value = {
		.content_hash = sys_cpu_to_le32(current_status.content_hash),
		.battery_pct = current_status.battery_pct,
		.battery_mv = sys_cpu_to_le16(current_status.battery_mv),
		.fw_version = sys_cpu_to_le16(current_status.fw_version),
	};
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &wire_value, sizeof(wire_value));
}

static void status_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	ARG_UNUSED(value);
	/* Notifications aren't relied on for correctness (the gateway always explicitly
	 * reads status per spec 5.1 step 3) — nothing to do here beyond letting the CCC
	 * descriptor exist so a notify-capable client doesn't error subscribing. */
}

static ssize_t write_data_transfer(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len, uint16_t offset,
				    uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	uint8_t msg_type;
	const uint8_t *wrapped_payload;
	size_t wrapped_payload_len;
	bool complete = chunk_reassembler_feed(&reassembler, buf, len, &msg_type,
						&wrapped_payload, &wrapped_payload_len);

	if (complete) {
		uint32_t target_hash;
		const uint8_t *data;
		size_t data_len;
		bool unwrapped = chunk_protocol_unwrap_target_hash(
			wrapped_payload, wrapped_payload_len, &target_hash, &data, &data_len);

		if (unwrapped) {
			bool applied = false;

			if (msg_type == CHUNK_MSG_TYPE_FULL) {
				applied = epaper_apply_full(data, data_len);
			} else if (msg_type == CHUNK_MSG_TYPE_DIFF) {
				applied = epaper_apply_diff(data, data_len);
			} else {
				LOG_WRN("data_transfer: unknown msg_type 0x%02x", msg_type);
			}

			/* content_hash is only bumped once the panel actually applied the
			 * update — see spec 4.3: this is what tells the gateway (via the
			 * next status read) that the sync succeeded. */
			if (applied) {
				ble_service_set_content_hash(target_hash);
			} else {
				LOG_WRN("data_transfer: epaper failed to apply update, "
					"content_hash left unchanged");
			}
		} else {
			LOG_WRN("data_transfer: reassembled message too short to contain "
				"target hash");
		}
	}

	/* Per BLE conventions, always ack the write itself regardless of whether the
	 * chunk protocol accepted it — rejecting the write would be a link-layer error,
	 * not how spec 4.3's "discard silently" is meant to surface. */
	return len;
}

static ssize_t write_command(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	uint8_t command = ((const uint8_t *)buf)[0];

	switch (command) {
	case BLE_SERVICE_COMMAND_FORCE_FULL_REFRESH:
		epaper_force_full_refresh();
		break;
	case BLE_SERVICE_COMMAND_SLEEP_NOW:
		if (conn != NULL) {
			bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
		break;
	case BLE_SERVICE_COMMAND_IDENTIFY:
		epaper_identify();
		break;
	default:
		LOG_WRN("command: unknown command 0x%02x", command);
		break;
	}

	return len;
}

BT_GATT_SERVICE_DEFINE(
	homescreen_svc, BT_GATT_PRIMARY_SERVICE(&svc_uuid),
	BT_GATT_CHARACTERISTIC(&status_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_READ, read_status, NULL, NULL),
	BT_GATT_CCC(status_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(&data_transfer_uuid.uuid, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE,
				NULL, write_data_transfer, NULL),
	BT_GATT_CHARACTERISTIC(&command_uuid.uuid, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL,
				write_command, NULL), );

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_WRN("connection failed (err %u)", err);
		return;
	}

	/* Always start a connection with a clean reassembly buffer — spec 4.1: never
	 * leave state ambiguous across a connection boundary. */
	chunk_reassembler_reset(&reassembler);
	active_conn = bt_conn_ref(conn);

	if (event_cb) {
		event_cb(BLE_SERVICE_EVENT_CONNECTED);
	}
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	chunk_reassembler_reset(&reassembler);
	if (active_conn) {
		bt_conn_unref(active_conn);
		active_conn = NULL;
	}

	if (event_cb) {
		event_cb(BLE_SERVICE_EVENT_DISCONNECTED);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};

int ble_service_init(void)
{
	chunk_reassembler_reset(&reassembler);
	return bt_enable(NULL);
}

void ble_service_set_event_callback(ble_service_event_cb_t cb)
{
	event_cb = cb;
}

int ble_service_start_advertising(void)
{
	return bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
}

int ble_service_stop_advertising(void)
{
	return bt_le_adv_stop();
}

void ble_service_set_content_hash(uint32_t content_hash)
{
	current_status.content_hash = content_hash;
}

void ble_service_set_battery(uint8_t battery_pct, uint16_t battery_mv)
{
	current_status.battery_pct = battery_pct;
	current_status.battery_mv = battery_mv;
}
