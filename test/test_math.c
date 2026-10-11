/*
 * Host-side check of src/gamecontroller_math.h: the scaling to the
 * normalised ranges, the hat, and the CRC-32 against zlib's.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gamecontroller_math.h"

int main(void)
{
	static const uint8_t check[] = "123456789";
	uint8_t data[3] = { 0x11, 0x22, 0x33 };
	uint8_t seeded[4] = { 0xA2, 0x11, 0x22, 0x33 };

	/* 8-bit: the ends reach the ends, the middle is near zero. */
	assert(gc_axis8(0) == -32768 && gc_axis8(255) == 32767);
	assert(gc_axis8(128) == 128);
	assert(gc_trigger8(0) == 0 && gc_trigger8(255) == 65535);
	/* 16-bit centred on 0x8000. */
	assert(gc_axis16(0) == -32768 && gc_axis16(0x8000) == 0);
	assert(gc_axis16(0xFFFF) == 32767);
	/* 10-bit: 1023 is fully pressed; past it is clamped. */
	assert(gc_trigger10(0) == 0 && gc_trigger10(1023) == 65535);
	assert(gc_trigger10(512) == 32800 && gc_trigger10(4095) == 65535);

	/* The hat: N clockwise, 8 and up released. */
	assert(gc_hat(0) == GAMECONTROLLER_BTN_DPAD_UP);
	assert(gc_hat(3) == (GAMECONTROLLER_BTN_DPAD_DOWN |
			     GAMECONTROLLER_BTN_DPAD_RIGHT));
	assert(gc_hat(7) == (GAMECONTROLLER_BTN_DPAD_UP |
			     GAMECONTROLLER_BTN_DPAD_LEFT));
	assert(gc_hat(8) == 0 && gc_hat(15) == 0);

	/* CRC-32: the catalogue check value, as zlib.crc32 gives it. */
	assert(~gc_crc32_update(0xFFFFFFFFu, check, 9) == 0xCBF43926u);
	/* A seeded CRC is the CRC of the seed and the data together. */
	assert(gc_ps_crc32(0xA2, data, 3) ==
	       ~gc_crc32_update(0xFFFFFFFFu, seeded, 4));
	seeded[0] = 0xA1;
	assert(gc_ps_input_crc32(0xA1, 0x11, data + 1, 2) ==
	       ~gc_crc32_update(0xFFFFFFFFu, (const uint8_t[]){ 0xA1, 0x11,
								 0x22, 0x33 }, 4));
	assert(gc_le32((const uint8_t[]){ 1, 2, 3, 4 }) == 0x04030201u);
	assert(gc_le16((const uint8_t[]){ 0x34, 0x12 }) == 0x1234);
	memset(data, 0, sizeof(data));
	printf("gamecontroller maths: all checks passed\n");
	return 0;
}
