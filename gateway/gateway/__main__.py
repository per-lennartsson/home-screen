"""
CLI entry point: `python -m gateway run` (or just `python -m gateway`) and
`python -m gateway scan`. See gateway/README.md.
"""

from __future__ import annotations

import asyncio
import logging
import sys

from gateway import runner
from gateway.config import config_from_args, parse_args
from gateway.platform_checks import BluetoothPermissionError


def _configure_logging(level: str) -> None:
    logging.basicConfig(
        level=getattr(logging, level.upper(), logging.INFO),
        format="%(asctime)s %(levelname)-7s %(name)s: %(message)s",
    )
    # bleak's own logging is extremely verbose at DEBUG and drowns out the sync log,
    # which is the thing you actually want to read during bring-up.
    logging.getLogger("bleak").setLevel(logging.INFO)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    _configure_logging(args.log_level)

    try:
        if args.command == "scan":
            asyncio.run(runner.scan(args.seconds))
        else:
            asyncio.run(runner.run(config_from_args(args)))
    except KeyboardInterrupt:
        return 130
    except runner.GatewayNotRegisteredError as exc:
        logging.getLogger("gateway").error("%s", exc)
        return 2
    except BluetoothPermissionError as exc:
        # Printed rather than logged: it's a multi-line instruction with commands to
        # copy, and the log formatter's timestamp/level prefix makes that harder to read.
        print(f"\n{exc}\n", file=sys.stderr)
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
