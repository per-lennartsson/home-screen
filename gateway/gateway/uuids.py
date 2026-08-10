"""
GATT UUIDs for the custom service (spec 4.2). These must stay byte-for-byte identical to
the BT_UUID_HOMESCREEN_* macros in firmware/src/ble_service.c — the gateway looks
characteristics up by UUID, so a mismatch shows up as "characteristic not found" on every
connection, not as anything more diagnosable.

A real generated base UUID, not the spec's placeholder pattern — the last 16 bits pick
the service (0000) or one of the characteristics. Changing it means changing both files
in the same commit and reflashing, or the gateway stops seeing the device.
"""

from __future__ import annotations

SERVICE_UUID = "8fd9daef-dd0f-4243-85e1-f9b453750000"
STATUS_CHAR_UUID = "8fd9daef-dd0f-4243-85e1-f9b453750001"
DATA_TRANSFER_CHAR_UUID = "8fd9daef-dd0f-4243-85e1-f9b453750002"
COMMAND_CHAR_UUID = "8fd9daef-dd0f-4243-85e1-f9b453750003"
BUTTON_EVENT_CHAR_UUID = "8fd9daef-dd0f-4243-85e1-f9b453750004"

# Advertised in the scan response (see the `sd[]` array in firmware/src/ble_service.c).
# Only used for human-readable logging during discovery — the service UUID above is what
# actually filters advertisements, since the name is not in the primary packet.
DEVICE_NAME = "HomeScreen Display"

# Command characteristic values — mirrors BLE_SERVICE_COMMAND_* in ble_service.h.
# FORCE_FULL_REFRESH/SLEEP_NOW/IDENTIFY are unused by the sync loop itself, here only so
# a diagnostic tool can trigger them without redefining the constants. ROTATE_NORMAL/
# ROTATE_180 *are* used by the sync loop (sync.py) — sent on every sync so a display's
# rotate_180 setting is self-healing after a firmware reset, the same way layout content
# already is.
COMMAND_FORCE_FULL_REFRESH = 0x01
COMMAND_SLEEP_NOW = 0x02
COMMAND_IDENTIFY = 0x03
COMMAND_ROTATE_NORMAL = 0x04
COMMAND_ROTATE_180 = 0x05
