"""
Per-device sync loop (spec 5.1): decide whether a check-in is due, connect, read status,
report to backend, fetch payload, push it if needed, disconnect. One flaky display must
never block another (spec 5.2) — every step is wrapped so a failure here just logs and
lets the next advertisement retry; there is no retry within a single connection (spec
5.1 step 8).
"""

from __future__ import annotations

import logging
import time
from dataclasses import dataclass

from gateway import protocol, uuids
from gateway.backend_client import BackendClient
from gateway.ble_transport import BleConnection, BleTransport

logger = logging.getLogger("gateway.sync")


@dataclass
class DeviceState:
    mac_address: str
    display_id: int
    pending: bool = True  # unknown -> assume it might need something until proven otherwise
    last_checkin_monotonic: float | None = None
    rotate_180: bool = False
    wake_interval_s: int = 15


class GatewayService:
    def __init__(
        self,
        gateway_id: int,
        backend: BackendClient,
        transport: BleTransport,
        checkin_interval_s: float = 60.0,
    ):
        self.gateway_id = gateway_id
        self.backend = backend
        self.transport = transport
        self.checkin_interval_s = checkin_interval_s
        self.devices: dict[str, DeviceState] = {}

    async def refresh_assigned_displays(self) -> None:
        """Pull the known-display list from the backend (spec 5.2) and flag any that
        the backend already knows are out of sync, so the next advertisement doesn't
        have to wait for the check-in timer."""
        displays = await self.backend.get_assigned_displays(self.gateway_id)
        seen_macs = set()
        for d in displays:
            seen_macs.add(d["mac_address"])
            state = self.devices.get(d["mac_address"])
            if state is None:
                state = DeviceState(mac_address=d["mac_address"], display_id=d["id"])
                self.devices[d["mac_address"]] = state
            state.display_id = d["id"]
            state.rotate_180 = d["rotate_180"]
            state.wake_interval_s = d["wake_interval_s"]
            if not d["in_sync"]:
                state.pending = True

        for mac in list(self.devices):
            if mac not in seen_macs:
                del self.devices[mac]  # reassigned away from this gateway

    async def handle_advertisement(self, mac_address: str) -> None:
        state = self.devices.get(mac_address)
        if state is None:
            return  # not one of ours (spec 5.1 step 1)

        checkin_due = (
            state.last_checkin_monotonic is None
            or (time.monotonic() - state.last_checkin_monotonic) >= self.checkin_interval_s
        )
        if not checkin_due and not state.pending:
            return  # nothing to do this cycle (spec 5.1 step 2)

        await self._sync_device(state)

    async def _sync_device(self, state: DeviceState) -> None:
        try:
            async with self.transport.connect(state.mac_address) as conn:
                # Asserted every sync, not just when it changes — cheap (one byte) and
                # self-healing after a firmware reset, same reasoning as re-pushing
                # content on a stale hash rather than trying to track "did this
                # already land" across power cycles (epaper.c's epaper_set_rotation
                # no-ops on-device if it's already the current value).
                await conn.write_command(
                    uuids.COMMAND_ROTATE_180 if state.rotate_180 else uuids.COMMAND_ROTATE_NORMAL
                )
                await conn.write_command(
                    uuids.COMMAND_SET_WAKE_INTERVAL_S, state.wake_interval_s.to_bytes(2, "little")
                )

                status = await conn.read_status()
                state.last_checkin_monotonic = time.monotonic()

                await self.backend.report_status(
                    state.display_id, status["content_hash"], status["battery_pct"], status["battery_mv"]
                )

                # Read/report a pending button press before fetching the payload — not
                # after — so if a row was just toggled, the payload this same connection
                # gets back already reflects it, instead of waiting a full extra wake
                # cycle for the press to show up on screen. No retry if this fails: the
                # whole connection aborts via the except below and the next advertisement
                # retries, same as any other step here (firmware already cleared its
                # mask on the read regardless — see docs/protocol.md's no-ack model,
                # same one `status` already uses).
                button_mask = await conn.read_button_event()
                if button_mask:
                    await self.backend.report_button_event(state.display_id, button_mask)

                payload = await self.backend.get_payload(state.display_id)
                if payload.get("in_sync"):
                    state.pending = False
                    logger.info("sync outcome=in_sync display_id=%s mac=%s", state.display_id, state.mac_address)
                    return

                await self._push_payload(conn, payload)
                # Optimistic: firmware only bumps content_hash on a verified reassembly
                # (spec 4.3). If the push silently failed, pending stays true here and
                # the next check-in's status read will show the stale hash and retry —
                # this flag just avoids waiting a full checkin_interval_s for that retry.
                state.pending = False
                logger.info(
                    "sync outcome=pushed type=%s display_id=%s mac=%s",
                    payload["type"],
                    state.display_id,
                    state.mac_address,
                )
        except Exception:
            logger.exception("sync outcome=failed display_id=%s mac=%s", state.display_id, state.mac_address)

    async def _push_payload(self, conn: BleConnection, payload: dict) -> None:
        if payload["type"] == "full":
            # Firmware has no JSON parser (docs/protocol.md) — encode_full_layout is
            # the flat binary format it actually decodes, not the backend's canonical
            # JSON snapshot.
            data_bytes = protocol.encode_full_layout(payload["data"])
            msg_type = protocol.MSG_TYPE_FULL
        else:
            values = {int(k): v for k, v in payload["data"]["values"].items()}
            data_bytes = protocol.encode_diff(values)
            msg_type = protocol.MSG_TYPE_DIFF

        # Firmware can't derive content_hash from a diff patch alone (see
        # docs/protocol.md) — the backend-computed target hash rides along in the
        # payload itself, protected by the same CRC16 check as the rest of the message.
        wrapped = protocol.wrap_payload_with_target_hash(payload["content_hash"], data_bytes)
        for chunk in protocol.encode_chunks(msg_type, wrapped, conn.mtu_payload_size):
            await conn.write_data_transfer(chunk)
