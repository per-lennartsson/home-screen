import { parseUtc } from "./format.js";

// A gateway is considered online if it's checked in within this window. There's no heartbeat
// interval defined elsewhere in the system to derive this from, so this is a reasonable fixed
// threshold rather than a backend-provided value.
export const GATEWAY_ONLINE_WINDOW_MS = 5 * 60 * 1000;

export function isGatewayOnline(gateway) {
  if (!gateway.last_checkin_at) return false;
  return Date.now() - parseUtc(gateway.last_checkin_at).getTime() < GATEWAY_ONLINE_WINDOW_MS;
}

export function displayStatus(display) {
  if (!display.last_seen_at) return "never";
  return display.in_sync ? "in_sync" : "pending";
}
