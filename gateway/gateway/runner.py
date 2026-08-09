"""
Process-level wiring for the gateway: build the backend client and BLE transport, keep
the assigned-display list fresh, and feed advertisements into `GatewayService`.

`GatewayService` (sync.py) deliberately knows nothing about where advertisements come
from or when it should re-pull its display list — that policy lives here, so the same
service object can be driven by the mock transport in tests and by real BLE in
production.
"""

from __future__ import annotations

import asyncio
import contextlib
import logging
import os
import time

import httpx

from gateway import uuids
from gateway.backend_client import BackendClient
from gateway.ble_transport import BleTransport
from gateway.config import GatewayConfig
from gateway.platform_checks import check_macos_bluetooth_permission
from gateway.sync import GatewayService

logger = logging.getLogger("gateway.runner")

# Backoff between attempts to reach a backend that isn't up yet. The gateway is very
# likely to be started before (or alongside) `docker compose up`, so treat this as normal
# rather than fatal.
BACKEND_RETRY_INTERVAL_S = 5.0

# How often to repeat the "saw an unregistered display" hint for the same address. Often
# enough to notice, rare enough not to bury the sync log.
UNKNOWN_DEVICE_LOG_INTERVAL_S = 60.0


class GatewayNotRegisteredError(RuntimeError):
    """The configured gateway_id doesn't exist in the backend — a config error, not a
    transient failure, so the process exits instead of retrying forever."""


async def wait_for_backend(backend: BackendClient, gateway_id: int) -> list[dict]:
    """Block until the backend answers, then return this gateway's displays.

    Separating this from the main loop means a typo'd gateway id or a backend that was
    never started fails with one clear line at startup, instead of as a stream of
    identical exceptions from inside the sync loop.
    """
    while True:
        try:
            return await backend.get_assigned_displays(gateway_id)
        except httpx.HTTPStatusError as exc:
            if exc.response.status_code == 404:
                raise GatewayNotRegisteredError(
                    f"backend has no gateway with id {gateway_id}. Register one first "
                    "(frontend, or POST /api/gateways) and pass its id via --gateway-id."
                ) from exc
            raise
        except httpx.HTTPError as exc:
            logger.warning(
                "backend not reachable (%s), retrying in %.0fs", exc, BACKEND_RETRY_INTERVAL_S
            )
            await asyncio.sleep(BACKEND_RETRY_INTERVAL_S)


def log_assigned_displays(displays: list[dict]) -> None:
    if not displays:
        logger.warning(
            "no displays are assigned to this gateway — nothing will be synced. Register a "
            "display (its address from `python -m gateway scan`) and assign it to this "
            "gateway in the frontend."
        )
        return

    logger.info("%d display(s) assigned to this gateway:", len(displays))
    for display in displays:
        logger.info(
            "  id=%s name=%r address=%s in_sync=%s",
            display["id"],
            display["name"],
            display["mac_address"],
            display.get("in_sync"),
        )


async def refresh_loop(service: GatewayService, interval_s: float) -> None:
    """Re-pull assignments forever. Failures are logged and retried on the next tick —
    the previous list stays in effect meanwhile, so a backend blip doesn't stop syncing
    displays the gateway already knows about."""
    while True:
        await asyncio.sleep(interval_s)
        try:
            await service.refresh_assigned_displays()
        except Exception:
            logger.exception("failed to refresh assigned displays; keeping the previous list")


async def run_service(service: GatewayService, transport: BleTransport, config: GatewayConfig) -> None:
    refresher = asyncio.create_task(refresh_loop(service, config.refresh_interval_s))
    unknown_logged_at: dict[str, float] = {}

    try:
        async for address in transport.scan():
            if address not in service.devices:
                # handle_advertisement would silently ignore this. Say something instead:
                # on macOS the address is a CoreBluetooth UUID rather than the MAC on the
                # board (see bleak_transport.py), so "my display is advertising and
                # nothing happens" is the single most likely bring-up failure, and this
                # line is the fix for it.
                now = time.monotonic()
                last = unknown_logged_at.get(address)
                if last is None or (now - last) >= UNKNOWN_DEVICE_LOG_INTERVAL_S:
                    unknown_logged_at[address] = now
                    logger.info(
                        "ignoring %s: advertising our service but not registered to this "
                        "gateway. Register it with mac_address=%s if this is your display.",
                        address,
                        address,
                    )
                continue

            await service.handle_advertisement(address)
    finally:
        refresher.cancel()
        with contextlib.suppress(asyncio.CancelledError):
            await refresher


async def run(config: GatewayConfig, transport: BleTransport | None = None) -> None:
    """Entry point for the `run` command. `transport` is injectable so this can be
    exercised against MockBleTransport without a radio."""
    if transport is None:
        # Only when a real radio is about to be used — an injected transport (tests, or
        # a mock bring-up run) touches no Bluetooth and shouldn't be gated on it.
        check_macos_bluetooth_permission(os.environ)

        from gateway.bleak_transport import BleakTransport

        transport = BleakTransport(
            connect_timeout_s=config.connect_timeout_s,
            advertisement_debounce_s=config.advertisement_debounce_s,
            mtu_payload_size_override=config.chunk_payload_size,
        )

    # Generous per-request timeout: the backend re-renders and re-hashes designs inside
    # some of these calls, and a slow response is still better than a failed transaction.
    async with httpx.AsyncClient(base_url=config.backend_url, timeout=30.0) as http_client:
        backend = BackendClient(http_client)

        logger.info(
            "gateway id=%s backend=%s service_uuid=%s",
            config.gateway_id,
            config.backend_url,
            uuids.SERVICE_UUID,
        )
        displays = await wait_for_backend(backend, config.gateway_id)
        log_assigned_displays(displays)

        service = GatewayService(
            gateway_id=config.gateway_id,
            backend=backend,
            transport=transport,
            checkin_interval_s=config.checkin_interval_s,
        )
        await service.refresh_assigned_displays()

        await run_service(service, transport, config)


async def scan(seconds: float) -> None:
    """Print every nearby device advertising our service UUID, then exit.

    This is the intended way to find the value to put in a display's `mac_address` field,
    since on macOS that value is a CoreBluetooth UUID that appears nowhere on the
    hardware (see bleak_transport.py).
    """
    check_macos_bluetooth_permission(os.environ)

    from bleak import BleakScanner

    # address -> best name seen so far. "Best" means: a name from this scan's own
    # advertisement data beats BLEDevice.name, which on macOS is whatever CoreBluetooth
    # has cached for that peripheral — often a name from firmware flashed to the board
    # months ago. The device name lives in our scan response rather than the primary
    # advertising packet (see ble_service.c), so the first packet of a burst frequently
    # arrives without it and the real name shows up a packet or two later.
    seen: dict[str, str | None] = {}

    def on_detection(device, advertisement_data) -> None:
        first_sighting = device.address not in seen
        if first_sighting:
            seen[device.address] = None
            print(f"  {device.address}  rssi={advertisement_data.rssi:>4}")
        if advertisement_data.local_name:
            seen[device.address] = advertisement_data.local_name

    print(
        f"Scanning {seconds:g}s for devices advertising {uuids.SERVICE_UUID}\n"
        f"(displays only advertise for a few seconds per wake interval — press the "
        f"button on the board to wake one immediately)\n"
    )
    scanner = BleakScanner(detection_callback=on_detection, service_uuids=[uuids.SERVICE_UUID])
    await scanner.start()
    try:
        await asyncio.sleep(seconds)
    finally:
        await scanner.stop()

    if not seen:
        print(
            "\nNo displays found. Check that the board is powered and advertising "
            f"(it should appear as {uuids.DEVICE_NAME!r} in any BLE scanner app), and that "
            "gateway/gateway/uuids.py matches firmware/src/ble_service.c."
        )
    else:
        print(f"\nFound {len(seen)} display(s):")
        for address, name in seen.items():
            if name == uuids.DEVICE_NAME:
                description = name
            elif name is None:
                description = "(no name in any advertisement seen — it rides in the scan response)"
            else:
                description = f"{name!r} — not this firmware's name; stale OS cache?"
            print(f"  {address}  {description}")
        print(
            "\nRegister one in the frontend using its address above as the MAC address, "
            "and assign it to this gateway."
        )
