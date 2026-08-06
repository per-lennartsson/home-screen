/* Host-native test shim only — see log.h in this directory for why this exists.
 * Reimplements the two little-endian readers chunk_protocol.c uses, byte-by-byte so
 * behavior doesn't depend on host endianness. */
#ifndef SHIM_ZEPHYR_SYS_BYTEORDER_H_
#define SHIM_ZEPHYR_SYS_BYTEORDER_H_

#include <stdint.h>

static inline uint16_t sys_get_le16(const uint8_t *src)
{
	return (uint16_t)(src[0] | ((uint16_t)src[1] << 8));
}

static inline uint32_t sys_get_le32(const uint8_t *src)
{
	return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) |
	       ((uint32_t)src[3] << 24);
}

#endif
