/*
 * Wire protocol for the `data_transfer` characteristic (spec 4.3). Must match
 * gateway/gateway/protocol.py byte-for-byte — see docs/protocol.md.
 *
 * One deliberate asymmetry from the Python side: gateway/gateway/protocol.py's
 * ChunkReassembler tolerates chunks arriving out of index order (defensive, since
 * nothing in the abstract protocol spec guarantees ordering). This firmware side does
 * NOT — it requires chunks in strict ascending index order and discards the whole
 * message otherwise. That's safe because chunks within one BLE connection are delivered
 * in order by the link layer (BLE ATT/L2CAP is ordered, unlike UDP), and requiring order
 * lets reassembly append straight into a flat buffer instead of needing a per-chunk
 * offset table — meaningfully simpler on a memory-constrained target.
 */

#ifndef CHUNK_PROTOCOL_H_
#define CHUNK_PROTOCOL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CHUNK_MSG_TYPE_FULL 0x01
#define CHUNK_MSG_TYPE_DIFF 0x02

#define CHUNK_HEADER_LEN 7 /* type(1) + chunk_index(2) + total_chunks(2) + crc16(2) */
#define CHUNK_TARGET_HASH_LEN 4

/* Max size of a single reassembled message (after the chunk header, before unwrapping
 * the target hash) firmware will buffer. Generous for a small status-display layout;
 * revisit alongside CHUNK_PROTOCOL_MAX_DIFF_ENTRIES if designs grow much larger. */
#define CHUNK_MAX_PAYLOAD_LEN 2048

#define CHUNK_MAX_DIFF_ENTRIES 32

typedef struct {
	uint8_t buffer[CHUNK_MAX_PAYLOAD_LEN];
	size_t buffer_len;
	uint16_t total_chunks;
	uint16_t next_expected_index;
	uint16_t expected_crc16;
	uint8_t msg_type;
	bool in_progress;
} chunk_reassembler_t;

void chunk_reassembler_reset(chunk_reassembler_t *r);

/*
 * Feed one `data_transfer` write into the reassembler. Returns true once a complete
 * message has been received AND its CRC16 matches, filling out_msg_type/out_payload/
 * out_payload_len — out_payload points into the reassembler's own buffer and is only
 * valid until the next feed() call, so the caller must consume it immediately.
 *
 * Returns false for every other case (still buffering, CRC mismatch, out-of-order
 * chunk, oversized message) without distinguishing which — per spec 4.3 these are all
 * "discard silently, the gateway's next check-in will see the stale content_hash and
 * retry," so the caller doesn't need to know why.
 */
bool chunk_reassembler_feed(chunk_reassembler_t *r, const uint8_t *chunk, size_t chunk_len,
			    uint8_t *out_msg_type, const uint8_t **out_payload,
			    size_t *out_payload_len);

/*
 * Splits a verified message payload into (target_content_hash, data). Firmware adopts
 * target_content_hash as-is rather than computing it — see docs/protocol.md,
 * "content_hash for diff updates," for why. Returns false if payload_len is too short
 * to contain the hash prefix.
 */
bool chunk_protocol_unwrap_target_hash(const uint8_t *payload, size_t payload_len,
					uint32_t *out_target_hash, const uint8_t **out_data,
					size_t *out_data_len);

typedef struct {
	uint8_t element_id;
	const uint8_t *value;  /* not null-terminated, points into the reassembler buffer */
	uint8_t value_len;
} chunk_diff_entry_t;

/*
 * Decodes a 0x02 diff message's data (spec: docs/protocol.md diff TLV format) into up
 * to max_entries entries. Returns the number of entries decoded, or -1 if the data is
 * malformed or has more than max_entries updates.
 */
int chunk_protocol_decode_diff(const uint8_t *data, size_t len, chunk_diff_entry_t *out_entries,
				size_t max_entries);

#endif /* CHUNK_PROTOCOL_H_ */
