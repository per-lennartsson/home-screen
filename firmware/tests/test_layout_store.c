/*
 * Host-native check that layout_store.c parses the exact flat binary format
 * gateway/gateway/protocol.py::encode_full_layout produces, and that diff entries patch
 * the retained layout the way epaper_apply_diff (epaper.c) relies on. Compiled with the
 * system C compiler against the shims in tests/shims/ — see firmware/tests/run.sh.
 *
 * Run: firmware/tests/run.sh
 */

#include <stdio.h>
#include <string.h>

#include "layout_store.h"

static int failures = 0;

#define CHECK(cond, msg)                                                                         \
	do {                                                                                       \
		if (!(cond)) {                                                                    \
			printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);                   \
			failures++;                                                               \
		}                                                                                 \
	} while (0)

static size_t hex_to_bytes(const char *hex, uint8_t *out)
{
	size_t len = strlen(hex) / 2;
	for (size_t i = 0; i < len; i++) {
		sscanf(hex + 2 * i, "%2hhx", &out[i]);
	}
	return len;
}

/* Generated from gateway/gateway/protocol.py::encode_full_layout (format 3) for a layout
 * with three elements — see the command in firmware/tests/run.sh's header comment for how
 * to regenerate this if the wire format ever changes:
 *   1. plain text "Hello", w=120, fontSize 16 regular       -> font_id 1, flags 0x00
 *   2. checklist row "Take out trash", w=200, fontSize 32   -> font_id 4, flags 0x01
 *   3. "Vardagsrum", w=180, fontSize 24 bold+center+underline
 *                                                           -> font_id 9, flags 0x14
 * Element 3 exists to pin the parts of the byte layout the other two leave at zero: a
 * non-default alignment, an explicit text style, and a bold weight (which lives in
 * font_id, not in a flag bit). */
static const char *FULL_LAYOUT_HEX =
	"03030100000000780000010548656c6c6f0200001400c80001040e54616b65206f757420747261"
	"7368030a004600b40014090a5661726461677372756d";

static void test_apply_full_matches_python_reference(void)
{
	uint8_t data[128];
	size_t len = hex_to_bytes(FULL_LAYOUT_HEX, data);

	layout_store_reset();
	bool ok = layout_store_apply_full(data, len);
	CHECK(ok, "well-formed full layout should be accepted");

	const layout_t *layout = layout_store_get();
	CHECK(layout->count == 3, "should retain 3 elements");

	const layout_element_t *el0 = &layout->elements[0];
	CHECK(el0->element_id == 1, "element 0 id should be 1");
	CHECK(el0->x == 0 && el0->y == 0, "element 0 position should be (0, 0)");
	CHECK(el0->w == 120, "element 0 width should be 120");
	CHECK(!el0->checkable, "plain text element should not be checkable");
	CHECK(!el0->checked, "plain text element should not be checked");
	CHECK(!el0->underline && !el0->strikethrough, "element 0 should have no text styling");
	CHECK(el0->align == LAYOUT_ALIGN_LEFT, "element 0 should be left-aligned");
	CHECK(el0->font_id == 1, "element 0 font_id should be 1 (regular 16px)");
	CHECK(el0->text_len == 5 && memcmp(el0->text, "Hello", 5) == 0,
	      "element 0 text should be 'Hello'");

	const layout_element_t *el1 = &layout->elements[1];
	CHECK(el1->element_id == 2, "element 1 id should be 2");
	CHECK(el1->x == 0 && el1->y == 20, "element 1 position should be (0, 20)");
	CHECK(el1->w == 200, "element 1 width should be 200");
	CHECK(el1->checkable, "button-sourced element should be checkable");
	CHECK(!el1->checked, "button-sourced element should start unchecked");
	CHECK(el1->font_id == 4, "element 1 font_id should be 4 (regular 32px)");
	CHECK(el1->text_len == 14 && memcmp(el1->text, "Take out trash", 14) == 0,
	      "element 1 text should be 'Take out trash'");

	/* The styling element: alignment, underline, and a bold weight carried by font_id
	 * rather than a flag bit — the three things format 3 added that format 2 dropped. */
	const layout_element_t *el2 = &layout->elements[2];
	CHECK(el2->element_id == 3, "element 2 id should be 3");
	CHECK(el2->x == 10 && el2->y == 70, "element 2 position should be (10, 70)");
	CHECK(el2->w == 180, "element 2 width should be 180");
	CHECK(el2->align == LAYOUT_ALIGN_CENTER, "element 2 should be center-aligned");
	CHECK(el2->underline, "element 2 should be underlined");
	CHECK(!el2->strikethrough, "element 2 should not be struck through");
	CHECK(el2->font_id == 9, "element 2 font_id should be 9 (semibold 24px: 6 + index 3)");
	CHECK(el2->text_len == 10 && memcmp(el2->text, "Vardagsrum", 10) == 0,
	      "element 2 text should be 'Vardagsrum'");
}

static void test_diff_entry_toggles_checked_state(void)
{
	uint8_t data[128];
	size_t len = hex_to_bytes(FULL_LAYOUT_HEX, data);
	layout_store_reset();
	layout_store_apply_full(data, len);

	chunk_diff_entry_t entry = {
		.element_id = 2,
		.value = (const uint8_t *)"checked",
		.value_len = 7,
	};
	bool applied = layout_store_apply_diff_entry(&entry);
	CHECK(applied, "diff entry for a known checkable element should apply");
	CHECK(layout_store_get()->elements[1].checked, "row should now read checked");

	entry.value = (const uint8_t *)"unchecked";
	entry.value_len = 9;
	layout_store_apply_diff_entry(&entry);
	CHECK(!layout_store_get()->elements[1].checked, "row should toggle back to unchecked");
}

static void test_diff_entry_replaces_text_for_non_checkable_element(void)
{
	uint8_t data[128];
	size_t len = hex_to_bytes(FULL_LAYOUT_HEX, data);
	layout_store_reset();
	layout_store_apply_full(data, len);

	chunk_diff_entry_t entry = {
		.element_id = 1,
		.value = (const uint8_t *)"Hi",
		.value_len = 2,
	};
	bool applied = layout_store_apply_diff_entry(&entry);
	CHECK(applied, "diff entry for a known non-checkable element should apply");

	const layout_element_t *el0 = &layout_store_get()->elements[0];
	CHECK(el0->text_len == 2 && memcmp(el0->text, "Hi", 2) == 0,
	      "non-checkable element's text should be replaced verbatim");
	CHECK(!el0->checked, "replacing text must not affect the checked flag");
}

static void test_diff_entry_for_unknown_element_is_a_noop(void)
{
	uint8_t data[128];
	size_t len = hex_to_bytes(FULL_LAYOUT_HEX, data);
	layout_store_reset();
	layout_store_apply_full(data, len);

	chunk_diff_entry_t entry = {
		.element_id = 99,
		.value = (const uint8_t *)"checked",
		.value_len = 7,
	};
	bool applied = layout_store_apply_diff_entry(&entry);
	CHECK(!applied, "diff entry for an element_id not in the retained layout should no-op");
}

static void test_malformed_full_payload_is_rejected_without_corrupting_state(void)
{
	uint8_t good[128];
	size_t good_len = hex_to_bytes(FULL_LAYOUT_HEX, good);
	layout_store_reset();
	layout_store_apply_full(good, good_len);

	uint8_t truncated[] = {0x03, 0x02, 0x01}; /* valid version, claims 2 elements, header cut off */
	bool ok = layout_store_apply_full(truncated, sizeof(truncated));
	CHECK(!ok, "truncated full payload should be rejected");
	CHECK(layout_store_get()->count == 3,
	      "a rejected full payload must leave the previously-retained layout untouched");

	/* Format 2 is the superseded font_scale-based layout. A device running this build
	 * must refuse it rather than misparse the shorter per-element record. */
	uint8_t old_version[] = {0x02, 0x00};
	CHECK(!layout_store_apply_full(old_version, sizeof(old_version)),
	      "superseded format version 2 should be rejected");
}

int main(void)
{
	test_apply_full_matches_python_reference();
	test_diff_entry_toggles_checked_state();
	test_diff_entry_replaces_text_for_non_checkable_element();
	test_diff_entry_for_unknown_element_is_a_noop();
	test_malformed_full_payload_is_rejected_without_corrupting_state();

	if (failures == 0) {
		printf("All layout_store native tests passed.\n");
		return 0;
	}
	printf("%d check(s) failed.\n", failures);
	return 1;
}
