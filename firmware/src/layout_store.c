#include "layout_store.h"

#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(layout_store, CONFIG_LOG_DEFAULT_LEVEL);

#define FULL_LAYOUT_FORMAT_VERSION 3
/* id(1) + x(2 LE) + y(2 LE) + w(2 LE) + flags(1) + font_id(1) + text_len(1) = 10 bytes
 * before the text itself — must match gateway/gateway/protocol.py::encode_full_layout's
 * per-element record. */
#define ELEMENT_HEADER_LEN 10

/* Bit layout of the per-element flags byte; mirrors protocol.py's FLAG_* constants.
 * Plain shifts rather than Zephyr's BIT(): this file is also compiled host-natively by
 * tests/run.sh, where sys/util.h is only a shim and BIT() isn't defined. */
#define FLAG_CHECKABLE (1 << 0)
#define FLAG_CHECKED (1 << 1)
#define FLAG_UNDERLINE (1 << 2)
#define FLAG_STRIKETHROUGH (1 << 3)
#define FLAG_ALIGN_SHIFT 4
#define FLAG_ALIGN_MASK (0x3 << FLAG_ALIGN_SHIFT)

static layout_t layout;

void layout_store_reset(void)
{
	memset(&layout, 0, sizeof(layout));
}

bool layout_store_apply_full(const uint8_t *data, size_t len)
{
	if (len < 2) {
		LOG_WRN("layout_store: payload too short for header");
		return false;
	}
	if (data[0] != FULL_LAYOUT_FORMAT_VERSION) {
		LOG_WRN("layout_store: unsupported format version %u", data[0]);
		return false;
	}

	uint8_t count = data[1];
	if (count > LAYOUT_MAX_ELEMENTS) {
		LOG_WRN("layout_store: %u elements exceeds max %d — design too large for this "
			"device, truncating",
			count, LAYOUT_MAX_ELEMENTS);
		count = LAYOUT_MAX_ELEMENTS;
	}

	/* Parse into a scratch copy first — a malformed element partway through must
	 * leave the previously-retained (still-displayed) layout untouched rather than
	 * being left half-overwritten. */
	layout_t parsed;
	memset(&parsed, 0, sizeof(parsed));

	size_t offset = 2;
	for (uint8_t i = 0; i < count; i++) {
		if (offset + ELEMENT_HEADER_LEN > len) {
			LOG_WRN("layout_store: truncated element header at index %u", i);
			return false;
		}

		uint8_t text_len = data[offset + 9];
		if (text_len > LAYOUT_MAX_TEXT_LEN || offset + ELEMENT_HEADER_LEN + text_len > len) {
			LOG_WRN("layout_store: malformed/oversized text at element index %u", i);
			return false;
		}

		layout_element_t *el = &parsed.elements[i];
		el->element_id = data[offset];
		el->x = (uint16_t)(data[offset + 1] | (data[offset + 2] << 8));
		el->y = (uint16_t)(data[offset + 3] | (data[offset + 4] << 8));
		el->w = (uint16_t)(data[offset + 5] | (data[offset + 6] << 8));
		uint8_t flags = data[offset + 7];
		el->checkable = (flags & FLAG_CHECKABLE) != 0;
		el->checked = (flags & FLAG_CHECKED) != 0;
		el->underline = (flags & FLAG_UNDERLINE) != 0;
		el->strikethrough = (flags & FLAG_STRIKETHROUGH) != 0;
		el->align = (layout_align_t)((flags & FLAG_ALIGN_MASK) >> FLAG_ALIGN_SHIFT);
		el->font_id = data[offset + 8];
		memcpy(el->text, &data[offset + ELEMENT_HEADER_LEN], text_len);
		el->text_len = text_len;

		offset += ELEMENT_HEADER_LEN + text_len;
	}

	parsed.count = count;
	layout = parsed;
	return true;
}

bool layout_store_apply_diff_entry(const chunk_diff_entry_t *entry)
{
	for (uint8_t i = 0; i < layout.count; i++) {
		layout_element_t *el = &layout.elements[i];

		if (el->element_id != entry->element_id) {
			continue;
		}

		if (el->checkable) {
			el->checked =
				entry->value_len == 7 && memcmp(entry->value, "checked", 7) == 0;
		} else {
			uint8_t copy_len = entry->value_len;
			if (copy_len > LAYOUT_MAX_TEXT_LEN) {
				copy_len = LAYOUT_MAX_TEXT_LEN;
			}
			memcpy(el->text, entry->value, copy_len);
			el->text_len = copy_len;
		}
		return true;
	}
	return false;
}

const layout_t *layout_store_get(void)
{
	return &layout;
}
