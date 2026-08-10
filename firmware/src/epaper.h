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

/* Command 0x01: force a full refresh next update regardless of content changes, to
 * clear ghosting accumulated from repeated partial refreshes (spec 4.4). */
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

#endif /* EPAPER_H_ */
