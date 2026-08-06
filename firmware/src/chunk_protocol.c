#include "chunk_protocol.h"

#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(chunk_protocol, CONFIG_LOG_DEFAULT_LEVEL);

/* CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) — pinned in docs/protocol.md, must match
 * gateway/gateway/protocol.py::crc16_ccitt_false exactly. */
static uint16_t crc16_ccitt_false(const uint8_t *data, size_t len)
{
	uint16_t crc = 0xFFFF;

	for (size_t i = 0; i < len; i++) {
		crc ^= (uint16_t)data[i] << 8;
		for (int bit = 0; bit < 8; bit++) {
			if (crc & 0x8000) {
				crc = (uint16_t)((crc << 1) ^ 0x1021);
			} else {
				crc = (uint16_t)(crc << 1);
			}
		}
	}
	return crc;
}

void chunk_reassembler_reset(chunk_reassembler_t *r)
{
	r->buffer_len = 0;
	r->total_chunks = 0;
	r->next_expected_index = 0;
	r->expected_crc16 = 0;
	r->msg_type = 0;
	r->in_progress = false;
}

bool chunk_reassembler_feed(chunk_reassembler_t *r, const uint8_t *chunk, size_t chunk_len,
			    uint8_t *out_msg_type, const uint8_t **out_payload,
			    size_t *out_payload_len)
{
	if (chunk_len < CHUNK_HEADER_LEN) {
		chunk_reassembler_reset(r);
		return false;
	}

	uint8_t msg_type = chunk[0];
	uint16_t index = sys_get_le16(&chunk[1]);
	uint16_t total = sys_get_le16(&chunk[3]);
	uint16_t crc = sys_get_le16(&chunk[5]);
	const uint8_t *body = &chunk[CHUNK_HEADER_LEN];
	size_t body_len = chunk_len - CHUNK_HEADER_LEN;

	if (index == 0) {
		/* Start of a new message. A fresh index-0 chunk always wins, even if a
		 * previous message was mid-reassembly — mirrors the gateway-side
		 * ChunkReassembler's "start fresh" behavior. */
		chunk_reassembler_reset(r);
		r->total_chunks = total;
		r->expected_crc16 = crc;
		r->msg_type = msg_type;
		r->in_progress = true;
	}

	if (!r->in_progress || index != r->next_expected_index || total != r->total_chunks ||
	    crc != r->expected_crc16 || msg_type != r->msg_type) {
		/* Out of order or belongs to a different message. BLE guarantees
		 * in-order delivery within a connection, so this should only happen on
		 * a genuine protocol error — discard and wait for the next index-0
		 * chunk rather than trying to recover mid-stream. */
		LOG_WRN("chunk protocol: unexpected chunk index=%u expected=%u, discarding",
			index, r->next_expected_index);
		chunk_reassembler_reset(r);
		return false;
	}

	if (r->buffer_len + body_len > sizeof(r->buffer)) {
		LOG_WRN("chunk protocol: message exceeds %zu bytes, discarding",
			sizeof(r->buffer));
		chunk_reassembler_reset(r);
		return false;
	}

	memcpy(&r->buffer[r->buffer_len], body, body_len);
	r->buffer_len += body_len;
	r->next_expected_index++;

	if (r->next_expected_index < r->total_chunks) {
		return false; /* more chunks still expected */
	}

	if (crc16_ccitt_false(r->buffer, r->buffer_len) != r->expected_crc16) {
		LOG_WRN("chunk protocol: CRC16 mismatch on reassembled message, discarding");
		chunk_reassembler_reset(r);
		return false;
	}

	*out_msg_type = r->msg_type;
	*out_payload = r->buffer;
	*out_payload_len = r->buffer_len;
	return true;
}

bool chunk_protocol_unwrap_target_hash(const uint8_t *payload, size_t payload_len,
					uint32_t *out_target_hash, const uint8_t **out_data,
					size_t *out_data_len)
{
	if (payload_len < CHUNK_TARGET_HASH_LEN) {
		return false;
	}

	*out_target_hash = sys_get_le32(payload);
	*out_data = payload + CHUNK_TARGET_HASH_LEN;
	*out_data_len = payload_len - CHUNK_TARGET_HASH_LEN;
	return true;
}

int chunk_protocol_decode_diff(const uint8_t *data, size_t len, chunk_diff_entry_t *out_entries,
				size_t max_entries)
{
	if (len < 1) {
		return -1;
	}

	uint8_t count = data[0];
	size_t offset = 1;
	size_t decoded = 0;

	if (count > max_entries) {
		LOG_WRN("chunk protocol: diff has %u entries, max supported is %zu", count,
			max_entries);
		return -1;
	}

	for (uint8_t i = 0; i < count; i++) {
		if (offset + 2 > len) {
			return -1;
		}
		uint8_t element_id = data[offset];
		uint8_t value_len = data[offset + 1];
		offset += 2;

		if (offset + value_len > len) {
			return -1;
		}

		out_entries[decoded].element_id = element_id;
		out_entries[decoded].value = &data[offset];
		out_entries[decoded].value_len = value_len;
		decoded++;
		offset += value_len;
	}

	return (int)decoded;
}
