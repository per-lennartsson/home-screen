"""
Startup checks for host-level things that stop BLE working before any of this project's
code gets a say.

Currently one check, for macOS, because the failure it catches is otherwise
undiagnosable: macOS kills the *process* — SIGABRT, no Python traceback, no output at all
if stdout is buffered — the instant CoreBluetooth is touched without Bluetooth
permission. The reason appears only in ~/Library/Logs/DiagnosticReports/Python-*.ips:

    "termination": {"namespace": "TCC", "details": ["This app has crashed because it
     attempted to access privacy-sensitive data without a usage description. The app's
     Info.plist must contain an NSBluetoothAlwaysUsageDescription key..."]}

That message is misleading about *whose* Info.plist matters. macOS attributes the request
to the responsible app — the one that launched the process chain — not to the Python
interpreter. Measured on this project: the same unmodified Homebrew Python scans fine
when launched from Terminal.app and is killed when launched from an app without
Bluetooth permission. Adding NSBluetoothAlwaysUsageDescription to Python's own
Python.app bundle changes nothing either way, so don't bother patching your interpreter.

Since the deciding factor is a TCC state we can't read, this doesn't try to predict it.
It runs a throwaway subprocess that touches CoreBluetooth and sees whether the OS kills
it — a real answer instead of a guess, at the cost of about a quarter of a second at
startup.
"""

from __future__ import annotations

import signal
import subprocess
import sys

# Starting a scan is what actually triggers the TCC check; merely importing bleak or
# constructing the scanner does not.
_PROBE_SOURCE = """
import asyncio
from bleak import BleakScanner


async def main():
    scanner = BleakScanner()
    await scanner.start()
    await scanner.stop()


asyncio.run(main())
"""

# Long enough to cover a cold bleak import on a slow disk, short enough not to look like
# a hang. If the probe outlives it (e.g. it's sitting behind a permission dialog the user
# hasn't answered), the check gives up and lets the real run proceed.
PROBE_TIMEOUT_S = 20.0

SKIP_ENV_VAR = "HOMESCREEN_SKIP_BLUETOOTH_PRECHECK"


class BluetoothPermissionError(RuntimeError):
    """Bluetooth would abort the process — raised before that can happen."""


def _probe_result(timeout_s: float) -> int | None:
    """Return the probe's exit status, or None if it couldn't be determined."""
    try:
        completed = subprocess.run(
            [sys.executable, "-c", _PROBE_SOURCE],
            capture_output=True,
            timeout=timeout_s,
        )
    except (subprocess.TimeoutExpired, OSError):
        return None
    return completed.returncode


def check_macos_bluetooth_permission(env, timeout_s: float = PROBE_TIMEOUT_S) -> None:
    """Raise BluetoothPermissionError if macOS would kill this process for using
    Bluetooth. Silent in every other case — including a probe that fails for some
    unrelated reason, where the real run's own error message will be more accurate than
    anything guessed here."""
    if sys.platform != "darwin" or env.get(SKIP_ENV_VAR):
        return

    returncode = _probe_result(timeout_s)
    if returncode != -signal.SIGABRT:
        return

    raise BluetoothPermissionError(
        f"""macOS killed a test process for using Bluetooth, and would kill this one too
(SIGABRT, no traceback — check ~/Library/Logs/DiagnosticReports/Python-*.ips).

Bluetooth access is granted to the app that launched this process, not to Python itself.
Whatever started it — an IDE, an agent, a task runner — either has no Bluetooth
permission or can't be granted it.

  Run the gateway from Terminal.app or iTerm instead, and approve the Bluetooth prompt.

That is the whole fix; nothing in this repo or your Python install needs changing. If
Terminal was already denied, re-enable it under System Settings -> Privacy & Security ->
Bluetooth. On Linux there's no equivalent restriction, so a Raspberry Pi gateway avoids
this entirely.

Set {SKIP_ENV_VAR}=1 to skip this check."""
    )
