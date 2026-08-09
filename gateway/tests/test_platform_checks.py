"""
The macOS Bluetooth precheck. Worth testing carefully despite being a startup nicety:
wrong in the permissive direction and the process dies with no output at all, wrong in
the strict direction and it blocks a setup that works.

An earlier version of this check guessed from the Python bundle's Info.plist and was
wrong in exactly that second way — measured on real hardware, the same unmodified Python
scans fine from Terminal.app and is killed when launched from an app without Bluetooth
permission. Hence the probe: only an actual SIGABRT counts as proof.

sys.platform is patched rather than skipped-on-Linux so these run the same everywhere.
"""

from __future__ import annotations

import signal
import sys

import pytest

from gateway import platform_checks
from gateway.platform_checks import (
    SKIP_ENV_VAR,
    BluetoothPermissionError,
    check_macos_bluetooth_permission,
)


@pytest.fixture
def darwin(monkeypatch):
    monkeypatch.setattr(sys, "platform", "darwin")


@pytest.fixture
def probe(monkeypatch):
    """Replace the subprocess probe; records whether it was called."""
    calls = []

    def fake(timeout_s):
        calls.append(timeout_s)
        return fake.returncode

    fake.returncode = 0
    fake.calls = calls
    monkeypatch.setattr(platform_checks, "_probe_result", fake)
    return fake


def test_raises_when_the_probe_is_killed_by_the_os(darwin, probe):
    probe.returncode = -signal.SIGABRT

    with pytest.raises(BluetoothPermissionError) as excinfo:
        check_macos_bluetooth_permission({})

    message = str(excinfo.value)
    assert "Terminal.app" in message, "the error must carry the actual fix, not just the diagnosis"


def test_passes_when_the_probe_succeeds(darwin, probe):
    probe.returncode = 0

    check_macos_bluetooth_permission({})


def test_passes_when_the_probe_fails_for_an_unrelated_reason(darwin, probe):
    """Bluetooth switched off, a broken bleak install — the real run's own error will be
    more accurate than anything inferred here, so don't pre-empt it."""
    probe.returncode = 1

    check_macos_bluetooth_permission({})


def test_passes_when_the_probe_result_is_unknown(darwin, probe):
    """A probe that timed out (e.g. sitting behind an unanswered permission dialog)
    must not block startup."""
    probe.returncode = None

    check_macos_bluetooth_permission({})


def test_skips_on_non_macos(monkeypatch, probe):
    monkeypatch.setattr(sys, "platform", "linux")

    check_macos_bluetooth_permission({})

    assert probe.calls == [], "no reason to spawn a probe where the restriction can't exist"


def test_env_var_bypasses_the_check(darwin, probe):
    probe.returncode = -signal.SIGABRT

    check_macos_bluetooth_permission({SKIP_ENV_VAR: "1"})

    assert probe.calls == []


def test_probe_runs_the_current_interpreter(monkeypatch):
    """The probe has to test the interpreter that will actually do the scanning."""
    captured = {}

    class Completed:
        returncode = 0

    def fake_run(cmd, **kwargs):
        captured["cmd"] = cmd
        captured["timeout"] = kwargs.get("timeout")
        return Completed()

    monkeypatch.setattr(platform_checks.subprocess, "run", fake_run)

    assert platform_checks._probe_result(7.5) == 0
    assert captured["cmd"][0] == sys.executable
    assert "BleakScanner" in captured["cmd"][2]
    assert captured["timeout"] == 7.5


def test_probe_reports_unknown_when_it_times_out(monkeypatch):
    import subprocess

    def fake_run(cmd, **kwargs):
        raise subprocess.TimeoutExpired(cmd, 1)

    monkeypatch.setattr(platform_checks.subprocess, "run", fake_run)

    assert platform_checks._probe_result(1.0) is None
