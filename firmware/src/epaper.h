/* ePaper rendering (spec 4.4). See epaper.c for why this is a stub. */

#ifndef EPAPER_H_
#define EPAPER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int epaper_init(void);

/* Applies a full render (msg type 0x01: canonical JSON layout bytes) — full refresh,
 * clears ghosting, slower/more power. Returns true on success. */
bool epaper_apply_full(const uint8_t *data, size_t len);

/* Applies a value diff (msg type 0x02: TLV-encoded, decoded here via
 * chunk_protocol_decode_diff) — partial refresh of just the changed region(s). Returns
 * true on success. */
bool epaper_apply_diff(const uint8_t *data, size_t len);

/* Command 0x01: immediately flashes the panel through several full black/white
 * inversions (see FULL_REFRESH_FLASH_CYCLES in epaper.c) to shake out accumulated
 * ghosting, then redraws whatever layout is currently retained. A single inversion
 * isn't enough — every ordinary content update already does one of those (v1 has no
 * partial-refresh path), so this has to do something visibly more thorough than a normal
 * update to actually clear ghosting. Triggered from the frontend, manually or on a
 * schedule, via backend Display.full_refresh_due and gateway/gateway/sync.py. */
void epaper_force_full_refresh(void);

/* Command 0x03: visibly identify this specific display, useful when physically locating
 * a display that matches a dashboard entry. */
void epaper_identify(void);

/* Commands 0x04/0x05: per-display mounting setting, RAM-only like layout_store's
 * retained layout (survives DEEP_SLEEP — see main.c's duty cycle — but not a reset).
 * The gateway resends this every sync regardless of whether it actually changed (spec:
 * gateway/gateway/sync.py), so it's self-healing after a watchdog reset the same way
 * layout content is; a no-op here when the value is unchanged avoids a redundant panel
 * refresh on every one of those resends. Re-renders and pushes the currently retained
 * layout immediately when the value does change, rather than waiting for the next
 * content update. */
void epaper_set_rotation(bool rotate_180);

/* System-status overlays: small icons composited on top of whatever content is
 * currently retained, on every subsequent panel push, independent of the gateway. Set
 * from main.c using state the firmware already knows locally (the charger IC's status
 * pin via battery.c, and the advertising-window retry counter) — the whole point is
 * these stay visible even when the gateway itself is unreachable. See epaper.c for the
 * icons and corner placement. Both are no-ops when the value hasn't changed, so calling
 * either every wake cycle (as main.c does) doesn't cost a panel refresh unless the
 * status actually flipped. */

/* Charging indicator, top-left corner. Composited into every render regardless of what
 * triggered it — including a freshly-applied full/diff layout — so it is never hidden
 * by new gateway content occupying the same corner. */
void epaper_set_charging(bool charging);

/* "Not connected" indicator, top-right corner. main.c sets this once the advertising
 * window has elapsed with no gateway connection some configurable number of times in a
 * row. epaper_apply_full/epaper_apply_diff clear it themselves the moment real content
 * actually arrives (proof the gateway is reachable again), rather than waiting for
 * main.c's end-of-cycle bookkeeping — so a new sync always wins over a stale badge in
 * the same corner. */
void epaper_set_connection_lost(bool lost);

#endif /* EPAPER_H_ */
