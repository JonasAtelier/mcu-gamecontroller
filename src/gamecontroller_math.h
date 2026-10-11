/*
 * The conversions every pad's parser shares: raw stick and trigger values
 * to the normalised ranges of struct gamecontroller_state, an 8-way hat to
 * d-pad bits, and the CRC-32 that PlayStation Bluetooth reports carry.
 * Kept free of platform headers so the host tests check them.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "gamecontroller.h"

/* An 8-bit stick, 0..255 with 128 at rest: x 257 - 32768 puts 0 at -32768
 * and 255 at 32767, the scaling SDL uses (SDL_hidapi_ps4.c:994). */
static inline int16_t gc_axis8(uint8_t v)
{
	return (int16_t)((int32_t)v * 257 - 32768);
}

/* An 8-bit trigger, 0 released: 255 becomes 65535. */
static inline uint16_t gc_trigger8(uint8_t v)
{
	return (uint16_t)(v * 257u);
}

/* A 16-bit stick centred on 0x8000 (SDL_hidapi_xboxone.c:1036-1043). */
static inline int16_t gc_axis16(uint16_t v)
{
	return (int16_t)((int32_t)v - 0x8000);
}

/* A 10-bit trigger, 0..1023: 1023 becomes 65535, rounded. */
static inline uint16_t gc_trigger10(uint16_t v)
{
	if (v > 1023)
		v = 1023;
	return (uint16_t)(((uint32_t)v * 65535u + 511u) / 1023u);
}

/* A hat as 0..7 = N, NE, E, SE, S, SW, W, NW; anything else is released.
 * The order is hid-playstation.c:535-537's; Xbox counts 1..8 and passes
 * its value less one. */
static inline uint32_t gc_hat(unsigned int v)
{
	static const uint32_t dirs[8] = {
		GAMECONTROLLER_BTN_DPAD_UP,
		GAMECONTROLLER_BTN_DPAD_UP | GAMECONTROLLER_BTN_DPAD_RIGHT,
		GAMECONTROLLER_BTN_DPAD_RIGHT,
		GAMECONTROLLER_BTN_DPAD_DOWN | GAMECONTROLLER_BTN_DPAD_RIGHT,
		GAMECONTROLLER_BTN_DPAD_DOWN,
		GAMECONTROLLER_BTN_DPAD_DOWN | GAMECONTROLLER_BTN_DPAD_LEFT,
		GAMECONTROLLER_BTN_DPAD_LEFT,
		GAMECONTROLLER_BTN_DPAD_UP | GAMECONTROLLER_BTN_DPAD_LEFT,
	};

	return v < 8 ? dirs[v] : 0;
}

/*
 * The CRC-32 of a PlayStation Bluetooth report: the reflected CRC-32
 * (polynomial 0xEDB88320, start 0xFFFFFFFF, inverted at the end) over one
 * seed byte -- the HIDP header, 0xA1 for input and 0xA2 for output -- and
 * then the report from its id up to the CRC (hid-playstation.c:75-76,
 * 1241-1247; SDL_hidapi_ps4.c:870-874). Stored little-endian in the last
 * four bytes.
 */
static inline uint32_t gc_crc32_update(uint32_t crc, const uint8_t *p,
				       size_t n)
{
	while (n--) {
		crc ^= *p++;
		for (int k = 0; k < 8; k++)
			crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
	}
	return crc;
}

static inline uint32_t gc_ps_crc32(uint8_t seed, const uint8_t *report,
				   size_t n)
{
	uint32_t crc = gc_crc32_update(0xFFFFFFFFu, &seed, 1);

	return ~gc_crc32_update(crc, report, n);
}

/* The same for an input report whose id arrived apart from its data. */
static inline uint32_t gc_ps_input_crc32(uint8_t seed, uint8_t report_id,
					 const uint8_t *data, size_t n)
{
	uint32_t crc = gc_crc32_update(0xFFFFFFFFu, &seed, 1);

	crc = gc_crc32_update(crc, &report_id, 1);
	return ~gc_crc32_update(crc, data, n);
}

static inline uint32_t gc_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
	       (uint32_t)p[3] << 24;
}

static inline uint16_t gc_le16(const uint8_t *p)
{
	return (uint16_t)(p[0] | p[1] << 8);
}

static inline void gc_put_le32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
}
