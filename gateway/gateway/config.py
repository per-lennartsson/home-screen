"""
Runtime configuration for the gateway process. Every setting is available as both a CLI
flag and a `HOMESCREEN_*` environment variable (flag wins), so the gateway can be run
ad-hoc during bring-up and from a launchd/systemd unit later without changing anything.

`HOMESCREEN_` matches the prefix the backend already uses (HOMESCREEN_DATA_DIR).
"""

from __future__ import annotations

import argparse
import os
from collections.abc import Mapping
from dataclasses import dataclass

DEFAULT_BACKEND_URL = "http://localhost:8000"

# How often each display is contacted even when the backend has nothing pending for it
# (spec 5.1 step 2) — this is the battery/liveness heartbeat, not the content path.
DEFAULT_CHECKIN_INTERVAL_S = 60.0

# How often the gateway re-pulls its display assignments. Also how quickly a design
# change made in the frontend starts being pushed: until this refresh marks the display
# pending, the gateway waits for the check-in timer instead.
DEFAULT_REFRESH_INTERVAL_S = 30.0

DEFAULT_CONNECT_TIMEOUT_S = 10.0
DEFAULT_ADVERTISEMENT_DEBOUNCE_S = 2.0
DEFAULT_SCAN_SECONDS = 30.0


@dataclass(frozen=True)
class GatewayConfig:
    gateway_id: int
    backend_url: str = DEFAULT_BACKEND_URL
    checkin_interval_s: float = DEFAULT_CHECKIN_INTERVAL_S
    refresh_interval_s: float = DEFAULT_REFRESH_INTERVAL_S
    connect_timeout_s: float = DEFAULT_CONNECT_TIMEOUT_S
    advertisement_debounce_s: float = DEFAULT_ADVERTISEMENT_DEBOUNCE_S
    chunk_payload_size: int | None = None
    log_level: str = "INFO"


def _env_default(env: Mapping[str, str], name: str, fallback):
    return env.get(f"HOMESCREEN_{name}", fallback)


def build_parser(env: Mapping[str, str] | None = None) -> argparse.ArgumentParser:
    env = os.environ if env is None else env

    parser = argparse.ArgumentParser(
        prog="python -m gateway",
        description="BLE gateway: bridges the backend's sync API to nRF52840 ePaper displays.",
    )
    subparsers = parser.add_subparsers(dest="command")

    run = subparsers.add_parser("run", help="run the sync loop (the normal mode)")
    run.add_argument(
        "--gateway-id",
        type=int,
        default=_env_default(env, "GATEWAY_ID", None),
        help="id of this gateway's row in the backend (create one in the frontend first)",
    )
    run.add_argument(
        "--backend-url",
        default=_env_default(env, "BACKEND_URL", DEFAULT_BACKEND_URL),
        help=f"base URL of the backend API (default: {DEFAULT_BACKEND_URL})",
    )
    run.add_argument(
        "--checkin-interval",
        type=float,
        default=float(_env_default(env, "CHECKIN_INTERVAL_S", DEFAULT_CHECKIN_INTERVAL_S)),
        help="seconds between forced check-ins of an already-in-sync display",
    )
    run.add_argument(
        "--refresh-interval",
        type=float,
        default=float(_env_default(env, "REFRESH_INTERVAL_S", DEFAULT_REFRESH_INTERVAL_S)),
        help="seconds between re-pulls of the assigned-display list from the backend",
    )
    run.add_argument(
        "--connect-timeout",
        type=float,
        default=float(_env_default(env, "CONNECT_TIMEOUT_S", DEFAULT_CONNECT_TIMEOUT_S)),
        help="per-connection BLE timeout in seconds",
    )
    run.add_argument(
        "--advertisement-debounce",
        type=float,
        default=float(
            _env_default(env, "ADVERTISEMENT_DEBOUNCE_S", DEFAULT_ADVERTISEMENT_DEBOUNCE_S)
        ),
        help="ignore repeat advertisements from the same display for this many seconds",
    )
    run.add_argument(
        "--chunk-payload-size",
        type=int,
        default=_maybe_int(_env_default(env, "CHUNK_PAYLOAD_SIZE", None)),
        help="override the negotiated chunk body size (bytes); diagnostic use only",
    )
    run.add_argument(
        "--log-level",
        default=_env_default(env, "LOG_LEVEL", "INFO"),
        help="DEBUG, INFO, WARNING, ...",
    )

    scan = subparsers.add_parser(
        "scan",
        help="list nearby displays and exit — use this to get the address to register",
    )
    scan.add_argument(
        "--seconds",
        type=float,
        default=DEFAULT_SCAN_SECONDS,
        help=(
            "how long to scan. Displays advertise for a few seconds once per wake "
            f"interval, so allow at least one full interval (default: {DEFAULT_SCAN_SECONDS:g})"
        ),
    )
    scan.add_argument("--log-level", default=_env_default(env, "LOG_LEVEL", "INFO"))

    return parser


def _maybe_int(value) -> int | None:
    if value is None or value == "":
        return None
    return int(value)


def parse_args(argv: list[str] | None = None, env: Mapping[str, str] | None = None):
    parser = build_parser(env)
    # Bare `python -m gateway` means "run" — the overwhelmingly common case, and it keeps
    # the pre-subcommand invocation style working for anything already scripted.
    args = parser.parse_args(argv if argv else ["run"])
    if args.command == "run" and args.gateway_id is None:
        parser.error("--gateway-id is required (or set HOMESCREEN_GATEWAY_ID)")
    return args


def config_from_args(args) -> GatewayConfig:
    return GatewayConfig(
        gateway_id=int(args.gateway_id),
        backend_url=args.backend_url,
        checkin_interval_s=args.checkin_interval,
        refresh_interval_s=args.refresh_interval,
        connect_timeout_s=args.connect_timeout,
        advertisement_debounce_s=args.advertisement_debounce,
        chunk_payload_size=args.chunk_payload_size,
        log_level=args.log_level,
    )
