# Gateway

Bridges the backend's sync API to the displays over BLE (spec 5). It runs as a plain
process on a machine with a Bluetooth radio — **not** in Docker: Docker Desktop on macOS
gives containers no access to the host's Bluetooth, which is why `docker-compose.yml`
covers only the backend and frontend.

Two implementations of the same `BleTransport` interface live here: `MockBleTransport`
(a simulated peripheral that speaks the real wire protocol, used by the tests) and
`BleakTransport` (real hardware). `GatewayService` — the actual sync loop — is identical
either way.

## Setup

```bash
python3 -m venv gateway/.venv && gateway/.venv/bin/pip install -r gateway/requirements.txt
```

### macOS: run it from a terminal app

**Run the gateway from Terminal.app or iTerm**, and approve the Bluetooth prompt the
first time.

macOS grants Bluetooth access to the app that *launched* the process, not to Python. Run
from an app that doesn't have it — an IDE's integrated terminal, an agent, a task runner
— and the OS kills the process outright: SIGABRT, no traceback, nothing on stdout, with
the reason buried in `~/Library/Logs/DiagnosticReports/Python-*.ips`.

That crash report claims the fix is adding `NSBluetoothAlwaysUsageDescription` to
Python's own `Info.plist`. It isn't, and you should not patch your interpreter: measured
against this project, stock Homebrew Python scans fine from Terminal.app and is killed
from a non-permitted app, with or without that key.

The gateway probes for this at startup and fails with an explanation rather than letting
the OS kill it silently. Set `HOMESCREEN_SKIP_BLUETOOTH_PRECHECK=1` to skip the probe.

Linux hosts have no equivalent restriction — BlueZ needs no special setup.

## First run, end to end

1. **Start the backend and frontend**: `docker compose up --build` (from the repo root).

2. **Create a gateway** at http://localhost:5173, and note the id it gets.

3. **Find your display's address** — power the board, then:

   ```bash
   gateway/.venv/bin/python -m gateway scan --seconds 30
   ```

   Firmware advertises for about 4 seconds once per wake interval (15 s during bring-up),
   so give it a full interval, or press a checklist button on the board to wake it
   immediately. Nothing found? Check the board appears as `HomeScreen Display` in any
   phone BLE scanner app first — that isolates firmware from gateway.

4. **Register the display** in the frontend using the address `scan` printed, assign it
   to your gateway, and assign it a design.

   > **On macOS the address is not the MAC on the board.** CoreBluetooth refuses to
   > expose BD_ADDR, so bleak substitutes a per-host CoreBluetooth UUID and that is what
   > goes in the display's `mac_address` field. It differs on every Mac and won't match a
   > Linux gateway's view of the same device — re-register if you move the gateway to
   > another host. On Linux the field holds a real MAC.

5. **Run the gateway**:

   ```bash
   gateway/.venv/bin/python -m gateway run --gateway-id 1
   ```

   It logs the displays assigned to it at startup, then one line per sync outcome. A
   display that advertises but isn't registered gets called out by address, which is
   usually how a wrong address in step 4 announces itself.

## What "working" looks like

Content converges after **two wake cycles**, not one — the gateway pushes on one
connection and only learns it landed when it reads `status` on the next
(docs/protocol.md, "Sync latency"). At the current 15-second `APP_WAKE_INTERVAL_S` that's
about half a minute from editing a design to seeing it on the panel, and a failed CRC
costs one more cycle.

15 s is a bring-up value chosen for a tolerable feedback loop, not a battery-life one —
the radio is advertising roughly a fifth of the time. Raise `APP_WAKE_INTERVAL_S` in
`firmware/src/main.c` once the path is proven.

If you raise it well past a minute, lower `--checkin-interval` to match or most wakes
will be no-ops: the gateway only opens a connection when a check-in is due or the backend
has flagged the display as pending.

## Configuration

Every flag has a `HOMESCREEN_`-prefixed environment variable; the flag wins.

| Flag | Env var | Default | Purpose |
| --- | --- | --- | --- |
| `--gateway-id` | `HOMESCREEN_GATEWAY_ID` | *(required)* | This gateway's backend row id |
| `--backend-url` | `HOMESCREEN_BACKEND_URL` | `http://localhost:8000` | Backend API base URL |
| `--checkin-interval` | `HOMESCREEN_CHECKIN_INTERVAL_S` | `60` | Forced check-in of an in-sync display |
| `--refresh-interval` | `HOMESCREEN_REFRESH_INTERVAL_S` | `30` | Re-pull of the assigned-display list |
| `--connect-timeout` | `HOMESCREEN_CONNECT_TIMEOUT_S` | `10` | Per-connection BLE timeout |
| `--advertisement-debounce` | `HOMESCREEN_ADVERTISEMENT_DEBOUNCE_S` | `2` | Ignore repeat adverts from one display |
| `--chunk-payload-size` | `HOMESCREEN_CHUNK_PAYLOAD_SIZE` | *(negotiated)* | Override the chunk body size |
| `--log-level` | `HOMESCREEN_LOG_LEVEL` | `INFO` | `DEBUG` also logs per-connection chunk sizes |

## Troubleshooting

| Symptom | Cause |
| --- | --- |
| Process dies instantly, no output | macOS Bluetooth permission — run it from Terminal.app, see above |
| `scan` finds nothing, phone scanner does | `gateway/gateway/uuids.py` and `firmware/src/ble_service.c` have drifted apart |
| "advertising our service but not registered" | The registered `mac_address` doesn't match what this host sees (macOS addresses again) |
| "data_transfer characteristic not found" | Connected to something that isn't running this firmware, or UUID drift |
| "MTU negotiation did not raise the write size" | Stuck at the 23-byte ATT default; transfers still work, just slowly |
| Panel never changes, no errors | Give it two wake cycles; check the backend shows the display as in sync afterwards |

## Tests

```bash
gateway/.venv/bin/python -m pytest gateway
```

No hardware or radio needed: the sync loop is exercised against `MockBleTransport` and
the real backend app in-process, and `BleakTransport`'s own logic (status struct layout,
MTU arithmetic, scan debounce, scan/connect interleaving) against fakes. What that
*can't* cover is bleak's behaviour against a real radio — that's what `scan` and a first
`run` are for.
