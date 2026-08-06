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

/* Generated from gateway/gateway/protocol.py::encode_full_layout for a layout with one
 * plain text element ("Hello", not checkable) and one button-sourced checklist row
 * ("Take out trash", checkable, initially unchecked) — see the command in
 * firmware/tests/run.sh's header comment for how to regenerate this if the wire format
 * ever changes. */
static const char *FULL_LAYOUT_HEX =
	"01020100000000000548656c6c6f0200001400010e54616b65206f7574207472617368";

static void test_apply_full_matches_python_reference(void)
{
	uint8_t data[128];
	size_t len = hex_to_bytes(FULL_LAYOUT_HEX, data);

	layout_store_reset();
	bool ok = layout_store_apply_full(data, len);
	CHECK(ok, "well-formed full layout should be accepted");

	const layout_t *layout = layout_store_get();
	CHECK(layout->count == 2, "should retain 2 elements");

	const layout_element_t *el0 = &layout->elements[0];
	CHECK(el0->element_id == 1, "element 0 id should be 1");
	CHECK(el0->x == 0 && el0->y == 0, "element 0 position should be (0, 0)");
	CHECK(!el0->checkable, "plain text element should not be checkable");
	CHECK(!el0->checked, "plain text element should not be checked");
	CHECK(el0->text_len == 5 && memcmp(el0->text, "Hello", 5) == 0,
	      "element 0 text should be 'Hello'");

	const layout_element_t *el1 = &layout->elements[1];
	CHECK(el1->element_id == 2, "element 1 id should be 2");
	CHECK(el1->x == 0 && el1->y == 20, "element 1 position should be (0, 20)");
	CHECK(el1->checkable, "button-sourced element should be checkable");
	CHECK(!el1->checked, "button-sourced element should start unchecked");
	CHECK(el1->text_len == 14 && memcmp(el1->text, "Take out trash", 14) == 0,
	      "element 1 text should be 'Take out trash'");
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

	uint8_t truncated[] = {0x01, 0x02, 0x01}; /* claims 2 elements, header cut off */
	bool ok = layout_store_apply_full(truncated, sizeof(truncated));
	CHECK(!ok, "truncated full payload should be rejected");
	CHECK(layout_store_get()->count == 2,
	      "a rejected full payload must leave the previously-retained layout untouched");

	uint8_t bad_version[] = {0x02, 0x00};
	CHECK(!layout_store_apply_full(bad_version, sizeof(bad_version)),
	      "unsupported format version should be rejected");
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
