#include "ble_service.h"

#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "button.h"
#include "chunk_protocol.h"
#include "epaper.h"

LOG_MODULE_REGISTER(ble_service, CONFIG_LOG_DEFAULT_LEVEL);

/* Bumped for the status struct's new `charging` byte (see struct status_value below) —
 * the gateway doesn't branch on this number, it just tolerates a status read shorter
 * than expected (bleak_transport.py), but it's still real signal for "has this board
 * been reflashed since charge-status was added" if that's ever needed. */
#define FW_VERSION 2

/* Real 128-bit base UUID for this project (spec 4.2 asked for one to replace its
 * placeholder pattern; generated with uuidgen). The last 16 bits of the final segment
 * distinguish the service (0000) from each characteristic.
 *
 * Duplicated in gateway/gateway/uuids.py, which is how the gateway finds the service and
 * its characteristics — the two must change together, and changing them means reflashing
 * before the gateway will see the device again. */
#define BT_UUID_HOMESCREEN_SERVICE_VAL                                                           \
	BT_UUID_128_ENCODE(0x8fd9daef, 0xdd0f, 0x4243, 0x85e1, 0xf9b453750000)
#define BT_UUID_HOMESCREEN_STATUS_VAL                                                            \
	BT_UUID_128_ENCODE(0x8fd9daef, 0xdd0f, 0x4243, 0x85e1, 0xf9b453750001)
#define BT_UUID_HOMESCREEN_DATA_TRANSFER_VAL                                                     \
	BT_UUID_128_ENCODE(0x8fd9daef, 0xdd0f, 0x4243, 0x85e1, 0xf9b453750002)
#define BT_UUID_HOMESCREEN_COMMAND_VAL                                                           \
	BT_UUID_128_ENCODE(0x8fd9daef, 0xdd0f, 0x4243, 0x85e1, 0xf9b453750003)
#define BT_UUID_HOMESCREEN_BUTTON_EVENT_VAL                                                      \
	BT_UUID_128_ENCODE(0x8fd9daef, 0xdd0f, 0x4243, 0x85e1, 0xf9b453750004)

static struct bt_uuid_128 svc_uuid = BT_UUID_INIT_128(BT_UUID_HOMESCREEN_SERVICE_VAL);
static struct bt_uuid_128 status_uuid = BT_UUID_INIT_128(BT_UUID_HOMESCREEN_STATUS_VAL);
static struct bt_uuid_128 data_transfer_uuid =
	BT_UUID_INIT_128(BT_UUID_HOMESCREEN_DATA_TRANSFER_VAL);
static struct bt_uuid_128 command_uuid = BT_UUID_INIT_128(BT_UUID_HOMESCREEN_COMMAND_VAL);
static struct bt_uuid_128 button_event_uuid =
	BT_UUID_INIT_128(BT_UUID_HOMESCREEN_BUTTON_EVENT_VAL);

static const uint8_t adv_service_uuid[] = {BT_UUID_HOMESCREEN_SERVICE_VAL};
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_UUID128_ALL, adv_service_uuid, sizeof(adv_service_uuid)),
};

/* The 128-bit UUID already fills the primary advertising packet's 31-byte budget, so the
 * device name goes in the scan response instead (its own separate 31-byte budget) —
 * without this, scanners like LightBlue/nRF Connect see the device but can't show
 * "HomeScreen Display" for it, only a blank/generic entry. */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

struct __packed status_value {
	uint32_t content_hash;
	uint8_t battery_pct;
	uint16_t battery_mv;
	uint16_t fw_version;
	/* Appended, not inserted — docs/protocol.md: "the gateway parses exactly [the
	 * first] 9 bytes and ignores anything after them, so firmware may append fields
	 * without breaking an older gateway." Read straight off the charger IC's status
	 * pin by battery.c, not derived from battery_mv. 0/1 rather than bool: this
	 * struct is __packed onto the wire and read raw by a non-C gateway, so its
	 * layout is spelled out in fixed-width integer types throughout, not the C
	 * compiler's (unspecified-width) _Bool. */
	uint8_t charging;
};

static struct status_value current_status = {
	.content_hash = 0,
	.battery_pct = 0,
	.battery_mv = 0,
	.fw_version = FW_VERSION,
	.charging = 0,
};

static chunk_reassembler_t reassembler;
static struct bt_conn *active_conn;
static ble_service_event_cb_t event_cb;

static uint16_t current_wake_interval_s = BLE_SERVICE_DEFAULT_WAKE_INTERVAL_S;

uint16_t ble_service_get_wake_interval_s(void)
{
	return current_wake_interval_s;
}

static ssize_t read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	struct status_value wire_value = {
		.content_hash = sys_cpu_to_le32(current_status.content_hash),
		.battery_pct = current_status.battery_pct,
		.battery_mv = sys_cpu_to_le16(current_status.battery_mv),
		.fw_version = sys_cpu_to_le16(current_status.fw_version),
		.charging = current_status.charging,
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

static ssize_t read_button_event(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	/* Always reads live, unlike status/battery's cached-setter pattern: the mask
	 * must be atomically read-and-cleared at the moment the gateway actually reads
	 * it (button_consume_pending_mask), not pre-snapshotted before advertising —
	 * otherwise a press arriving between that snapshot and the actual GATT read
	 * would be lost a cycle early. */
	uint8_t mask = button_consume_pending_mask();
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &mask, sizeof(mask));
}

/* write_data_transfer() and write_command() both run on the Bluetooth stack's own
 * callback context. epaper_apply_full()/epaper_apply_diff() (like epaper_identify() and
 * epaper_force_full_refresh() below) drive real SPI transfers plus multi-second BUSY-pin
 * wait loops (see BUSY_TIMEOUT_MS in epaper_ssd1683.c: "typically 1-2s", worst case
 * 10s) — blocking that long inside a GATT write callback delays the ATT write response
 * long enough that the central times out and drops the connection before ever seeing it.
 * 241d357 fixed this for the command characteristic (identify/force_full_refresh) after
 * it disconnected the phone mid-refresh on real hardware, but missed data_transfer,
 * which has exactly the same problem on the far more common path: every full-payload
 * push disconnected mid-transfer on the very next hardware test after that fix, with
 * bleak reporting a plain "disconnected" while writing the completing chunk.
 *
 * Deferring to Zephyr's *default* system workqueue isn't enough on its own — the BT host
 * also uses that same queue to return a just-disconnected connection object to its pool,
 * so a slow epaper op stuck there starves that cleanup too, which is what made
 * bt_le_adv_start() keep failing with -ENOMEM after every disconnect (confirmed against
 * real hardware, and matches github.com/zephyrproject-rtos/zephyr issue #90111 and Nordic
 * DevZone thread 109942 on the same failure mode). A dedicated queue/thread keeps a slow
 * or stuck panel refresh from ever blocking Bluetooth's own housekeeping. */
/* 8K, not the 2K this started at. Everything submitted to this queue ends up in
 * rasterizer_render(), which renders through LVGL — and LVGL's software renderer needs
 * far more stack than the 8x8 bitmap font it replaced. At 2K it overflowed on the first
 * real gateway sync: HW_STACK_PROTECTION turned that into an MPU fault, the SoC reset
 * mid-connection, and the gateway saw a bare `BleakError: disconnected` while writing a
 * command — no on-device log, because the console dies with the fault.
 *
 * Sized to match CONFIG_MAIN_STACK_SIZE (prj.conf), which had to be raised for the same
 * reason: LVGL's init runs pre-main() on the main thread. Neither figure is measured —
 * both are "comfortably more than LVGL seemed to need". CONFIG_THREAD_ANALYZER would
 * give real high-water marks if this ever needs trimming for RAM. */
#define EPAPER_WORKQ_STACK_SIZE 8192
#define EPAPER_WORKQ_PRIORITY 10

static struct k_work_q epaper_work_q;
K_THREAD_STACK_DEFINE(epaper_work_q_stack, EPAPER_WORKQ_STACK_SIZE);

static struct k_work identify_work;
static struct k_work full_refresh_work;
static struct k_work rotate_normal_work;
static struct k_work rotate_180_work;

static void identify_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	epaper_identify();
}

static void full_refresh_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	epaper_force_full_refresh();
}

static void rotate_normal_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	epaper_set_rotation(false);
}

static void rotate_180_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	epaper_set_rotation(true);
}

/* Unlike identify/full_refresh above, applying a data_transfer message needs the payload
 * to still exist once the deferred work actually runs — and `data` in write_data_transfer
 * points into the reassembler's own static buffer (chunk_protocol.h: "only valid until
 * the next feed() call"), which a new connection's first write is free to start
 * overwriting immediately. So the payload is copied into its own staging buffer, sized
 * to chunk_reassembler_feed's own enforced ceiling (chunk_protocol.c rejects any write
 * that would exceed CHUNK_MAX_PAYLOAD_LEN before it happens, so data_len can never
 * exceed this).
 *
 * k_work_is_pending() before submitting guards the one real hazard: a second complete
 * message arriving while the previous one is still being applied. That can only happen
 * across two different connections (one gateway sync loop pushes at most one message
 * per connection — see gateway/gateway/sync.py), and the current design covers exactly
 * one connection at a time (main.c), so in practice this is a rare "reconnected
 * unusually fast" case, not a normal one. Rather than queue a second update behind the
 * first — which would need its own staging slot — it's dropped, matching this codebase's
 * established "can't safely act on it right now, discard, the next check-in retries"
 * convention (chunk_protocol.c, layout_store.c, epaper.c all do the same). */
#define EPAPER_APPLY_STAGING_LEN (CHUNK_MAX_PAYLOAD_LEN - CHUNK_TARGET_HASH_LEN)

static struct {
	struct k_work work;
	uint8_t msg_type;
	uint8_t data[EPAPER_APPLY_STAGING_LEN];
	size_t data_len;
	uint32_t target_hash;
} epaper_apply_staging;

static void epaper_apply_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	bool applied = false;

	if (epaper_apply_staging.msg_type == CHUNK_MSG_TYPE_FULL) {
		applied = epaper_apply_full(epaper_apply_staging.data, epaper_apply_staging.data_len);
	} else if (epaper_apply_staging.msg_type == CHUNK_MSG_TYPE_DIFF) {
		applied = epaper_apply_diff(epaper_apply_staging.data, epaper_apply_staging.data_len);
	}

	/* content_hash is only bumped once the panel actually applied the update — see
	 * spec 4.3: this is what tells the gateway (via the next status read) that the
	 * sync succeeded. */
	if (applied) {
		ble_service_set_content_hash(epaper_apply_staging.target_hash);
	} else {
		LOG_WRN("data_transfer: epaper failed to apply update, content_hash left unchanged");
	}
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

		if (!unwrapped) {
			LOG_WRN("data_transfer: reassembled message too short to contain "
				"target hash");
		} else if (msg_type != CHUNK_MSG_TYPE_FULL && msg_type != CHUNK_MSG_TYPE_DIFF) {
			LOG_WRN("data_transfer: unknown msg_type 0x%02x", msg_type);
		} else if (k_work_is_pending(&epaper_apply_staging.work)) {
			LOG_WRN("data_transfer: previous update still applying, dropping this "
				"one — next check-in retries");
		} else {
			epaper_apply_staging.msg_type = msg_type;
			epaper_apply_staging.target_hash = target_hash;
			epaper_apply_staging.data_len = data_len;
			memcpy(epaper_apply_staging.data, data, data_len);
			k_work_submit_to_queue(&epaper_work_q, &epaper_apply_staging.work);
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
	/* Every command below except SET_WAKE_INTERVAL_S is a bare 1-byte write; that one
	 * carries a 2-byte value after the command byte, so the length check happens per
	 * command in the switch instead of a single upfront `len != 1`. */
	if (len < 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	const uint8_t *data = buf;
	uint8_t command = data[0];

	switch (command) {
	case BLE_SERVICE_COMMAND_FORCE_FULL_REFRESH:
		k_work_submit_to_queue(&epaper_work_q, &full_refresh_work);
		break;
	case BLE_SERVICE_COMMAND_SLEEP_NOW:
		if (conn != NULL) {
			bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
		break;
	case BLE_SERVICE_COMMAND_IDENTIFY:
		k_work_submit_to_queue(&epaper_work_q, &identify_work);
		break;
	case BLE_SERVICE_COMMAND_ROTATE_NORMAL:
		k_work_submit_to_queue(&epaper_work_q, &rotate_normal_work);
		break;
	case BLE_SERVICE_COMMAND_ROTATE_180:
		k_work_submit_to_queue(&epaper_work_q, &rotate_180_work);
		break;
	case BLE_SERVICE_COMMAND_SET_WAKE_INTERVAL_S: {
		if (len != 1 + sizeof(uint16_t)) {
			LOG_WRN("command: SET_WAKE_INTERVAL_S needs a 2-byte value, got len=%u",
				len);
			break;
		}
		uint16_t requested_s = sys_get_le16(&data[1]);

		if (requested_s < BLE_SERVICE_MIN_WAKE_INTERVAL_S) {
			requested_s = BLE_SERVICE_MIN_WAKE_INTERVAL_S;
		} else if (requested_s > BLE_SERVICE_MAX_WAKE_INTERVAL_S) {
			requested_s = BLE_SERVICE_MAX_WAKE_INTERVAL_S;
		}
		current_wake_interval_s = requested_s;
		break;
	}
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
	BT_GATT_CHARACTERISTIC(&button_event_uuid.uuid, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
				read_button_event, NULL, NULL),
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

	k_work_queue_init(&epaper_work_q);
	k_work_queue_start(&epaper_work_q, epaper_work_q_stack, K_THREAD_STACK_SIZEOF(epaper_work_q_stack),
			    EPAPER_WORKQ_PRIORITY, NULL);

	k_work_init(&identify_work, identify_work_handler);
	k_work_init(&full_refresh_work, full_refresh_work_handler);
	k_work_init(&rotate_normal_work, rotate_normal_work_handler);
	k_work_init(&rotate_180_work, rotate_180_work_handler);
	k_work_init(&epaper_apply_staging.work, epaper_apply_work_handler);
	return bt_enable(NULL);
}

void ble_service_set_event_callback(ble_service_event_cb_t cb)
{
	event_cb = cb;
}

int ble_service_start_advertising(void)
{
	/* Stop defensively first, ignoring the result (ok to fail with "not advertising").
	 * BT_LE_ADV_CONN_FAST_1 auto-stops advertising once a connection forms, but on real
	 * hardware that teardown wasn't reliably complete by the time the very next cycle
	 * called bt_le_adv_start() again after a disconnect — observed as bt_le_adv_start()
	 * permanently failing with -ENOMEM (stale advertising-set resource never released)
	 * on every cycle from then on, with no crash and no way to recover without a reset. */
	bt_le_adv_stop();
	return bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}

int ble_service_stop_advertising(void)
{
	return bt_le_adv_stop();
}

void ble_service_set_content_hash(uint32_t content_hash)
{
	current_status.content_hash = content_hash;
}

void ble_service_set_battery(uint8_t battery_pct, uint16_t battery_mv, bool charging)
{
	current_status.battery_pct = battery_pct;
	current_status.battery_mv = battery_mv;
	current_status.charging = charging ? 1 : 0;
}
