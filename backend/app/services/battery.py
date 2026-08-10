"""Battery-life estimation from a display's logged BatteryReading history.

Rather than model current draw from a theoretical budget (BLE radio-on time, ADC wake
time, e-paper refresh energy...) — none of which is measured on real hardware yet —
this extrapolates from the display's *own* observed drain rate, the same "real data
over assumed constants" bias as battery.c's BATTERY_EMPTY_MV/FULL_MV comment.
"""

from typing import Literal

from sqlalchemy import select
from sqlalchemy.orm import Session

from app.models.db import BatteryReading, Display

# Mirrors firmware/src/battery.c's BATTERY_EMPTY_MV — the millivolt floor battery_pct is
# computed against. Duplicated here (not read from the device) because the backend only
# ever sees the resulting battery_mv/battery_pct over BLE, never the firmware constant.
BATTERY_EMPTY_MV = 3300

# Below this many matching readings, a drain slope is more noise than signal.
MIN_READINGS_FOR_ESTIMATE = 2

# Below this many elapsed hours between the oldest and newest matching reading, single-
# sample ADC jitter (battery.c does no averaging) can dominate the slope.
MIN_SPAN_HOURS_FOR_ESTIMATE = 1.0


class BatteryEstimate:
    def __init__(
        self,
        status: Literal["ok", "modeled", "insufficient_data", "not_draining"],
        wake_interval_s: int,
        drain_mv_per_hour: float | None = None,
        estimated_days_remaining: float | None = None,
        sample_count: int = 0,
        span_hours: float | None = None,
    ):
        self.status = status
        self.wake_interval_s = wake_interval_s
        self.drain_mv_per_hour = drain_mv_per_hour
        self.estimated_days_remaining = estimated_days_remaining
        self.sample_count = sample_count
        self.span_hours = span_hours


def _drain_rate(readings: list[BatteryReading]) -> tuple[float, float] | None:
    """oldest-vs-newest mV/hour slope for a single already-sorted, same-interval
    reading list, or None if there isn't enough of it to trust."""
    if len(readings) < MIN_READINGS_FOR_ESTIMATE:
        return None
    oldest, newest = readings[0], readings[-1]
    span_hours = (newest.recorded_at - oldest.recorded_at).total_seconds() / 3600
    if span_hours < MIN_SPAN_HOURS_FOR_ESTIMATE:
        return None
    return (oldest.battery_mv - newest.battery_mv) / span_hours, span_hours


def _rates_by_interval(readings: list[BatteryReading]) -> dict[int, tuple[float, int, float]]:
    """Buckets readings by the wake_interval_s they were logged under and computes each
    bucket's own drain rate — one number per wake interval this display has actually
    run at, {interval_s: (drain_mv_per_hour, sample_count, span_hours)}."""
    by_interval: dict[int, list[BatteryReading]] = {}
    for r in readings:
        by_interval.setdefault(r.wake_interval_s, []).append(r)

    rates: dict[int, tuple[float, int, float]] = {}
    for interval_s, rs in by_interval.items():
        rs.sort(key=lambda r: r.recorded_at)
        result = _drain_rate(rs)
        if result is not None:
            drain, span_hours = result
            rates[interval_s] = (drain, len(rs), span_hours)
    return rates


def _raw_stats(readings: list[BatteryReading]) -> tuple[int, float | None]:
    """count/span for an already-sorted reading list regardless of whether it clears
    _drain_rate's trust threshold — used so "insufficient_data" can report what's
    actually been logged (e.g. "36 readings, 0.5h so far") instead of always 0, which
    reads as "nothing logged" even when there's plenty, just not enough elapsed time."""
    if len(readings) < 2:
        return len(readings), None
    span_hours = (readings[-1].recorded_at - readings[0].recorded_at).total_seconds() / 3600
    return len(readings), span_hours


def _fit_drain_model(rates: dict[int, tuple[float, int, float]]) -> tuple[float, float] | None:
    """Fits drain_mv_per_hour = a + b * wakes_per_hour (wakes_per_hour = 3600 /
    interval_s) by least squares over each wake interval's own measured drain rate —
    the model behind projecting an estimate for an interval this display has never
    actually run at. a is the roughly wake-frequency-independent background drain
    (leakage etc); b is the per-wake-event cost (radio/ADC/refresh). Needs at least two
    distinct intervals' worth of data; returns None otherwise."""
    points = [(3600 / interval_s, drain) for interval_s, (drain, _, _) in rates.items()]
    if len(points) < 2:
        return None

    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    xbar = sum(xs) / len(xs)
    ybar = sum(ys) / len(ys)
    denom = sum((x - xbar) ** 2 for x in xs)
    if denom == 0:
        return None

    b = sum((x - xbar) * (y - ybar) for x, y in zip(xs, ys)) / denom
    a = ybar - b * xbar
    return a, b


def estimate_remaining(db: Session, display: Display, wake_interval_s: int | None = None) -> BatteryEstimate:
    """Estimates remaining battery life at `wake_interval_s` (defaults to the display's
    current setting). If this display has logged enough readings *at that exact
    interval*, uses its own measured drain rate directly ("ok"). Otherwise, if it has
    measured drain rates at two or more *other* intervals, projects one via
    _fit_drain_model ("modeled") — this is what lets you ask "what if I switched to a
    15-minute interval" without ever having run at 15 minutes. Falls back to
    "insufficient_data" if neither is possible."""
    target_interval = wake_interval_s if wake_interval_s is not None else display.wake_interval_s

    readings = db.scalars(
        select(BatteryReading).where(BatteryReading.display_id == display.id).order_by(BatteryReading.recorded_at)
    ).all()
    if not readings:
        return BatteryEstimate(status="insufficient_data", wake_interval_s=target_interval)

    latest_mv = readings[-1].battery_mv
    rates = _rates_by_interval(readings)
    target_readings = sorted(
        (r for r in readings if r.wake_interval_s == target_interval), key=lambda r: r.recorded_at
    )
    raw_count, raw_span_hours = _raw_stats(target_readings)

    if target_interval in rates:
        drain_mv_per_hour, sample_count, span_hours = rates[target_interval]
        status: Literal["ok", "modeled"] = "ok"
    else:
        model = _fit_drain_model(rates)
        if model is None:
            # Report what's actually logged at this interval (raw_count/raw_span_hours)
            # rather than 0 — there may be plenty of readings, just not enough elapsed
            # time yet to clear MIN_SPAN_HOURS_FOR_ESTIMATE, and "0 readings" reads very
            # differently from "36 readings, only 0.5h of them so far".
            return BatteryEstimate(
                status="insufficient_data",
                wake_interval_s=target_interval,
                sample_count=raw_count,
                span_hours=raw_span_hours,
            )
        a, b = model
        drain_mv_per_hour = a + b * (3600 / target_interval)
        sample_count = raw_count
        span_hours = raw_span_hours
        status = "modeled"

    if drain_mv_per_hour <= 0:
        return BatteryEstimate(
            status="not_draining",
            wake_interval_s=target_interval,
            drain_mv_per_hour=drain_mv_per_hour,
            sample_count=sample_count,
            span_hours=span_hours,
        )

    hours_remaining = max(latest_mv - BATTERY_EMPTY_MV, 0) / drain_mv_per_hour
    return BatteryEstimate(
        status=status,
        wake_interval_s=target_interval,
        drain_mv_per_hour=drain_mv_per_hour,
        estimated_days_remaining=hours_remaining / 24,
        sample_count=sample_count,
        span_hours=span_hours,
    )
