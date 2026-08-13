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

# Below this many wake-to-wake gaps of a given kind (check-in-only or push), one noisy
# ADC sample away from the deep-sleep floor can dominate the average as easily as it can
# dominate MIN_READINGS_FOR_ESTIMATE's per-interval slope above.
MIN_SAMPLES_FOR_POWER_BREAKDOWN = 2

# As of fw_version 2, battery.c reads real charge status off the charger IC's status
# pin (see BatteryReading.reported_charging) — but plenty of logged history predates
# that (older firmware, or rows logged before this column existed), so this trend-based
# inference stays as the fallback charging_flags() uses whenever the real signal is
# unavailable (None). A single-step delta is too noisy to use directly (ADC jitter alone
# spans dozens of mV between adjacent readings even while genuinely discharging — see the
# readings around any stable stretch); instead, compare the mean of the readings in a
# real time window right after this one against the mean right before it. Charging drives
# that comparison strongly and consistently positive; discharge noise doesn't (validated
# against a live fleet's overnight data: discharge noise tops out under ~15 mV/hour on
# this window, genuine charging clears 40+).
CHARGE_TREND_WINDOW_HOURS = 1.0
CHARGE_RISE_MV_PER_HOUR = 18.0


class BatteryEstimate:
    def __init__(
        self,
        status: Literal["ok", "modeled", "insufficient_data", "not_draining"],
        wake_interval_s: int,
        drain_mv_per_hour: float | None = None,
        estimated_days_remaining: float | None = None,
        sample_count: int = 0,
        span_hours: float | None = None,
        checkin_mv_per_hour: float | None = None,
        checkin_sample_count: int = 0,
        push_mv_per_hour: float | None = None,
        push_sample_count: int = 0,
        charging_excluded_count: int = 0,
    ):
        self.status = status
        self.wake_interval_s = wake_interval_s
        self.drain_mv_per_hour = drain_mv_per_hour
        self.estimated_days_remaining = estimated_days_remaining
        self.sample_count = sample_count
        self.span_hours = span_hours
        self.checkin_mv_per_hour = checkin_mv_per_hour
        self.checkin_sample_count = checkin_sample_count
        self.push_mv_per_hour = push_mv_per_hour
        self.push_sample_count = push_sample_count
        self.charging_excluded_count = charging_excluded_count


def _trend_charging_flags(
    readings: list[BatteryReading],
    window_hours: float = CHARGE_TREND_WINDOW_HOURS,
    rise_mv_per_hour: float = CHARGE_RISE_MV_PER_HOUR,
) -> list[bool]:
    """Per-reading "was this recorded while charging" flag inferred purely from the mv
    trend, for an already-sorted reading list, one entry per input reading — the
    fallback charging_flags() uses when BatteryReading.reported_charging is None. For
    each reading, compares the mean battery_mv of everything within `window_hours`
    *after* it against the mean of everything within `window_hours` *before* it; a rise
    steeper than `rise_mv_per_hour` over that comparison is charging, not discharge
    noise (see the CHARGE_TREND_WINDOW_HOURS/CHARGE_RISE_MV_PER_HOUR comment above for
    how those defaults were picked). O(n^2) — fine at this project's per-display reading
    counts (hundreds, not millions); revisit with a proper sliding-window sum if that
    changes."""
    flags = []
    for r in readings:
        back = [o.battery_mv for o in readings if 0 <= (r.recorded_at - o.recorded_at).total_seconds() / 3600 <= window_hours]
        fwd = [o.battery_mv for o in readings if 0 <= (o.recorded_at - r.recorded_at).total_seconds() / 3600 <= window_hours]
        if len(back) < 2 or len(fwd) < 2:
            flags.append(False)
            continue
        trend = (sum(fwd) / len(fwd) - sum(back) / len(back)) / window_hours
        flags.append(trend > rise_mv_per_hour)
    return flags


def charging_flags(readings: list[BatteryReading]) -> list[bool]:
    """Per-reading "was this recorded while charging" flag for an already-sorted
    reading list, one entry per input reading. Trusts BatteryReading.reported_charging
    (the charger IC's own status pin, as of fw_version 2 — see that column's docstring)
    whenever it isn't None; falls back to _trend_charging_flags for any reading where
    it is, which covers both older firmware and history logged before this column
    existed. Real signal is authoritative on its own (it doesn't need the trend to
    agree) — but if only the trend catches a given reading, that's kept too, rather than
    letting a confirmed-not-charging signal at reading N override what reading N+1's own
    trend independently suggests about the gap between them."""
    trend_flags = _trend_charging_flags(readings)
    return [
        (r.reported_charging is True) or trend
        for r, trend in zip(readings, trend_flags)
    ]


def _exclude_charging(readings: list[BatteryReading]) -> list[BatteryReading]:
    """The subset of an already-sorted reading list that wasn't recorded while
    charging — used only for the informational raw count/span in the "insufficient_data"
    message. The actual rate math below uses _discharge_segment_pairs instead, which is
    gap-aware in a way that simply filtering this list and comparing its oldest-vs-newest
    isn't (see that function's docstring)."""
    flags = charging_flags(readings)
    return [r for r, charging in zip(readings, flags) if not charging]


def _discharge_segment_pairs(readings: list[BatteryReading]) -> list[tuple[BatteryReading, BatteryReading]]:
    """Consecutive (prev, next) pairs from an already-sorted, *full* (charging readings
    included) reading list where neither end was recorded while charging. This is the
    unit every rate calculation below sums over, instead of just filtering out charging
    readings and comparing the remainder's oldest-vs-newest — that naive approach
    conflates two unrelated discharge stretches whenever a charging session sits between
    them: the far side can end up sitting *higher* than the near side started, even
    though every real discharge run in between was declining, simply because charging
    boosted the level in the gap. Summing per-gap mV drop / elapsed hours over these
    pairs instead means each gap only ever compares itself to itself, and a pair
    straddling a charging reading is dropped rather than blended in."""
    flags = charging_flags(readings)
    pairs = []
    for i in range(len(readings) - 1):
        prev, nxt = readings[i], readings[i + 1]
        if flags[i] or flags[i + 1]:
            continue
        pairs.append((prev, nxt))
    return pairs


def _rates_by_interval(readings: list[BatteryReading]) -> dict[int, tuple[float, int, float]]:
    """Buckets discharge segments (_discharge_segment_pairs) by wake_interval_s and sums
    each bucket's mV drop and elapsed hours across every one — one number per wake
    interval this display has actually discharged at, {interval_s: (drain_mv_per_hour,
    segment_count, span_hours)}. Summing per-segment rather than diffing a whole
    bucket's oldest-vs-newest reading is what makes this immune to a charging session (or
    several) sitting in the middle of that bucket's history. Also drops any segment whose
    wake_interval_s changed mid-gap — unlike _power_breakdown, a rate here is meant to
    describe *one* interval, so a gap that straddles a setting change isn't a clean
    sample of either."""
    totals: dict[int, list[float]] = {}
    for prev, nxt in _discharge_segment_pairs(readings):
        if prev.wake_interval_s != nxt.wake_interval_s:
            continue
        span_hours = (nxt.recorded_at - prev.recorded_at).total_seconds() / 3600
        if span_hours <= 0:
            continue
        bucket = totals.setdefault(prev.wake_interval_s, [0.0, 0.0, 0])
        bucket[0] += prev.battery_mv - nxt.battery_mv
        bucket[1] += span_hours
        bucket[2] += 1

    rates: dict[int, tuple[float, int, float]] = {}
    for interval_s, (mv_drop, hours, segment_count) in totals.items():
        if segment_count < MIN_READINGS_FOR_ESTIMATE - 1 or hours < MIN_SPAN_HOURS_FOR_ESTIMATE:
            continue
        rates[interval_s] = (mv_drop / hours, segment_count, hours)
    return rates


def _raw_stats(readings: list[BatteryReading]) -> tuple[int, float | None]:
    """count/span for an already-sorted reading list regardless of whether it clears
    _rates_by_interval's trust threshold — used so "insufficient_data" can report what's
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


def _power_breakdown(readings: list[BatteryReading]) -> dict[str, float | int | None]:
    """Mean mV/hour drain over each non-charging wake-to-wake gap (_discharge_segment_
    pairs) in an already-sorted, full (all-intervals) reading list, split by whether the
    *earlier* reading's wake pushed a content payload before disconnecting
    (BatteryReading.pushed_payload) or was a bare status check-in. Unlike
    _rates_by_interval, this deliberately pools across wake_interval_s buckets (and keeps
    gaps that straddle an interval change) since check-in/push cost is a roughly fixed
    per-event cost rather than something that scales with wake frequency the way idle
    drain does — it only needs to agree on whether this wake was a push, not which
    interval it happened at."""
    checkin_rates: list[float] = []
    push_rates: list[float] = []
    for prev, nxt in _discharge_segment_pairs(readings):
        span_hours = (nxt.recorded_at - prev.recorded_at).total_seconds() / 3600
        if span_hours <= 0:
            continue
        rate = (prev.battery_mv - nxt.battery_mv) / span_hours
        (push_rates if prev.pushed_payload else checkin_rates).append(rate)

    return {
        "checkin_mv_per_hour": (
            sum(checkin_rates) / len(checkin_rates) if len(checkin_rates) >= MIN_SAMPLES_FOR_POWER_BREAKDOWN else None
        ),
        "checkin_sample_count": len(checkin_rates),
        "push_mv_per_hour": (
            sum(push_rates) / len(push_rates) if len(push_rates) >= MIN_SAMPLES_FOR_POWER_BREAKDOWN else None
        ),
        "push_sample_count": len(push_rates),
    }


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

    # latest_mv reflects the actual current voltage (even mid-charge, if it's on a
    # charger right now) — but every rate below is fit only over genuine discharge, or
    # "days remaining if unplugged now" would be skewed by whatever charging session it
    # last saw. See charging_flags' docstring for how that split is decided.
    latest_mv = readings[-1].battery_mv
    discharge_readings = _exclude_charging(readings)
    charging_excluded_count = len(readings) - len(discharge_readings)

    # Independent of the days-remaining estimate below (which stays scoped to a single
    # target interval) — pooled across every interval this display has ever run at, so
    # it can answer "how much does a push cost vs a bare check-in" even when there isn't
    # enough of any *one* interval's data to model days-remaining at all.
    breakdown = _power_breakdown(readings)
    breakdown["charging_excluded_count"] = charging_excluded_count

    rates = _rates_by_interval(readings)
    target_readings = sorted(
        (r for r in discharge_readings if r.wake_interval_s == target_interval), key=lambda r: r.recorded_at
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
                **breakdown,
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
            **breakdown,
        )

    hours_remaining = max(latest_mv - BATTERY_EMPTY_MV, 0) / drain_mv_per_hour
    return BatteryEstimate(
        status=status,
        wake_interval_s=target_interval,
        drain_mv_per_hour=drain_mv_per_hour,
        estimated_days_remaining=hours_remaining / 24,
        sample_count=sample_count,
        span_hours=span_hours,
        **breakdown,
    )
