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

#endif /* EPAPER_H_ */
