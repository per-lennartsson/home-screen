/*
 * Retains the last-applied full layout in RAM so a later diff (which only carries
 * element_id + new value, spec 4.3) has something to patch and re-rasterize against —
 * epaper_apply_full/epaper_apply_diff had no such state before this feature (see
 * firmware/README.md). Pure/hardware-independent like chunk_protocol.c, so it's
 * natively testable (firmware/tests/) without the nRF Connect SDK toolchain.
 */

#ifndef LAYOUT_STORE_H_
#define LAYOUT_STORE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chunk_protocol.h"

#define LAYOUT_MAX_ELEMENTS 16
#define LAYOUT_MAX_TEXT_LEN 32

/* Horizontal alignment of the text within the element's w-wide box. Values match the
 * 2-bit field the wire format packs into flags (gateway/gateway/protocol.py's
 * ALIGN_LEFT/CENTER/RIGHT) and the `align` strings in frontend/src/lib/layout.js. */
typedef enum {
	LAYOUT_ALIGN_LEFT = 0,
	LAYOUT_ALIGN_CENTER = 1,
	LAYOUT_ALIGN_RIGHT = 2,
} layout_align_t;

typedef struct {
	uint8_t element_id;
	uint16_t x;
	uint16_t y;
	uint16_t w; /* box width, needed to resolve `align`; added in format 3 */
	bool checkable; /* true for button-sourced checklist rows (props.source=="button") */
	bool checked;
	bool underline;
	bool strikethrough; /* the explicit text style, distinct from `checked` above — a
			     * checked checklist row is struck through too, but for a
			     * different reason (see rasterizer.c) */
	layout_align_t align;
	uint8_t font_id; /* index into the shared font ladder, encoding size AND weight —
			  * see firmware/src/fonts/hs_fonts.h. Not range-checked here, same
			  * as x/y: hs_font_get() falls back to a sane font at render time
			  * rather than rejecting the whole layout over one bad byte. */
	char text[LAYOUT_MAX_TEXT_LEN];
	uint8_t text_len;
} layout_element_t;

typedef struct {
	layout_element_t elements[LAYOUT_MAX_ELEMENTS];
	uint8_t count;
} layout_t;

void layout_store_reset(void);

/* Parses a 0x01 (full) data_transfer payload — the flat binary format encoded by
 * gateway/gateway/protocol.py::encode_full_layout (docs/protocol.md), not JSON —
 * replacing whatever layout was previously retained. Bounds-checks every field against
 * `len` and the LAYOUT_MAX_* caps; returns false and leaves the previous layout
 * untouched on any malformed or oversized input, matching chunk_protocol.c's "discard
 * silently, the next check-in will retry" convention. */
bool layout_store_apply_full(const uint8_t *data, size_t len);

/* Patches one decoded diff entry (chunk_protocol_decode_diff) into the retained layout.
 * The matching element's own retained `checkable` flag (set once at the last full
 * apply) decides how entry->value is interpreted: for a checkable element it's the
 * literal string "checked"/"unchecked"; otherwise it replaces the element's displayed
 * text verbatim. Returns false (no-op) if entry->element_id isn't in the retained
 * layout — an update for an element this device doesn't know about is silently ignored,
 * same "stale content_hash, next check-in retries" philosophy as the rest of spec 4.3. */
bool layout_store_apply_diff_entry(const chunk_diff_entry_t *entry);

const layout_t *layout_store_get(void);

#endif /* LAYOUT_STORE_H_ */
