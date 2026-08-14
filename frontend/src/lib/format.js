const UNITS = [
  ["year", 31536000],
  ["month", 2592000],
  ["day", 86400],
  ["hour", 3600],
  ["minute", 60],
  ["second", 1],
];

// The backend (FastAPI/SQLAlchemy over SQLite) serializes timestamps as naive ISO strings with
// no timezone designator, even though they're UTC under the hood — `new Date(...)` would
// otherwise parse them as local time. Append "Z" when there's no offset already.
export function parseUtc(isoString) {
  const hasOffset = /Z$|[+-]\d\d:\d\d$/.test(isoString);
  return new Date(hasOffset ? isoString : `${isoString}Z`);
}

// Applies a value element's display precision (rounds numeric states to N decimals,
// same rounding HA's own "Display precision" entity setting does), an optional
// roundTo step snap (e.g. 0.5 turns 21.34/21.26/20.98 into 21.5/21.0/21.0 instead of
// chasing every 0.1 wobble — for entities that should read as precise but shouldn't
// repaint on every minor sensor jitter), and appends its unit of measurement suffix.
// Non-numeric states pass through both untouched — a unit can still be appended (e.g.
// a text sensor with a custom unit).
export function formatEntityValue(rawValue, { unit, precision, roundTo } = {}) {
  if (rawValue == null) return rawValue;
  let text = rawValue;
  const hasPrecision = precision !== undefined && precision !== null && precision !== "";
  const hasRoundTo = roundTo !== undefined && roundTo !== null && roundTo !== "";
  if (hasPrecision || hasRoundTo) {
    let num = Number(rawValue);
    if (!Number.isNaN(num)) {
      if (hasRoundTo) {
        const step = Number(roundTo);
        if (step > 0) num = Math.round(num / step) * step;
      }
      text = hasPrecision ? num.toFixed(Number(precision)) : String(num);
    }
  }
  return unit ? `${text} ${unit}` : text;
}

export function relativeTime(isoString) {
  if (!isoString) return "never";
  const seconds = (Date.now() - parseUtc(isoString).getTime()) / 1000;
  if (seconds < 0) return "just now";
  for (const [unit, secondsPerUnit] of UNITS) {
    const value = Math.floor(seconds / secondsPerUnit);
    if (value >= 1) {
      return `${value} ${unit}${value === 1 ? "" : "s"} ago`;
    }
  }
  return "just now";
}
