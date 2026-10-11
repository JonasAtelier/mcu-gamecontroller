/*
 * Host-side check of the DualSense parser and builder against the
 * layout-derived vectors in vectors.h (not captured from a pad). The
 * output tests pin hid-playstation.c's layout -- seq_tag at [1], tag 0x10
 * at [2], the common block from [3] -- which SDL 2.30.9 puts one byte
 * earlier.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ds5.h"
#include "gamecontroller_math.h"
#include "vectors.h"

int main(void)
{
	struct gamecontroller_state s = { 0 };
	uint8_t v[sizeof(ds5_full)];
	uint8_t out[DS5_OUTPUT_LEN];
	struct ds5_output o = {
		.rumble = true, .strong = 0xC8, .weak = 0x32,
		.lightbar = true, .red = 10, .green = 20, .blue = 30,
		.player_leds = true, .player_mask = 0x04,
	};
	static const uint8_t simple[9] = { 0x00, 0xFF, 0x80, 0x80,
					   0x08 | 0x10, 0x00, 0x04, 0x22, 0x33 };

	/* The full report 0x31. */
	assert(ds5_parse(0x31, ds5_full, sizeof(ds5_full), &s));
	assert(s.lx == 128 && s.ly == -32768 && s.rx == 128 && s.ry == 32767);
	assert(s.lt == 0x7F7F && s.rt == 0);
	assert(s.buttons == (GAMECONTROLLER_BTN_DPAD_DOWN |
			     GAMECONTROLLER_BTN_DPAD_LEFT |
			     GAMECONTROLLER_BTN_NORTH | GAMECONTROLLER_BTN_TR |
			     GAMECONTROLLER_BTN_THUMBL | GAMECONTROLLER_BTN_MUTE));
	/* Charging state 1, level 7: 75 %, charging (S1:1453-1482). */
	assert(s.battery == 75 && s.charging && s.seq == 1);

	/* Bad CRC, length or id: refused. */
	memcpy(v, ds5_full, sizeof(v));
	v[1] ^= 0x80;
	assert(!ds5_parse(0x31, v, sizeof(v), &s) && s.seq == 1);
	assert(!ds5_parse(0x31, ds5_full, 76, &s));
	assert(!ds5_parse(0x11, ds5_full, sizeof(ds5_full), &s));

	/* Battery: full, the error states unknown, 10 capped at 100. */
	{
		static const struct { uint8_t status, pct; bool chg; } cases[] = {
			{ 0x0A, 100, false }, { 0x2A, 100, false },
			{ 0xA3, 0xFF, false }, { 0xF3, 0xFF, false },
			{ 0x03, 35, false },
		};

		for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
			struct gamecontroller_state t = { 0 };

			memcpy(v, ds5_full, sizeof(v));
			v[1 + 52] = cases[i].status;
			gc_put_le32(v + 73, gc_ps_input_crc32(0xA1, 0x31, v, 73));
			assert(ds5_parse(0x31, v, sizeof(v), &t));
			assert(t.battery == cases[i].pct && t.charging == cases[i].chg);
		}
	}

	/* The simple report 0x01, the DS4's short layout (SDL_hidapi_ps5.c:77-86):
	 * the battery stays. */
	assert(ds5_parse(0x01, simple, sizeof(simple), &s));
	assert(s.lx == -32768 && s.ly == 32767 && s.lt == 0x2222 && s.rt == 0x3333);
	assert(s.buttons == (GAMECONTROLLER_BTN_WEST | GAMECONTROLLER_BTN_MUTE));
	assert(s.battery == 75 && s.seq == 2);

	/* The output report, every byte, CRC by zlib. */
	assert(ds5_build_output(&o, 3, out) == DS5_OUTPUT_LEN);
	assert(!memcmp(out, ds5_out, sizeof(ds5_out)));
	/* The S1 offsets, pinned one by one. */
	assert(out[0] == 0x31 && out[1] == 0x30 && out[2] == 0x10);
	assert(out[3] == 0x03);			/* COMPATIBLE_VIBRATION | HAPTICS_SELECT */
	assert(out[4] == 0x14);			/* LIGHTBAR | PLAYER_INDICATOR */
	assert(out[5] == 0x32 && out[6] == 0xC8);	/* motor_right, motor_left */
	assert(out[46] == 0x04);		/* player_leds */
	assert(out[47] == 10 && out[48] == 20 && out[49] == 30);
	/* The sequence number takes the high nibble only. */
	ds5_build_output(&o, 0x13, out);
	assert(out[1] == 0x30);

	/* Only what is asked for: the light-bar release on its own. */
	{
		struct ds5_output setup = { .lightbar_setup = true };

		ds5_build_output(&setup, 0, out);
		assert(!memcmp(out, ds5_out_setup, sizeof(ds5_out_setup)));
		assert(out[3 + 38] == 0x02 && out[3 + 41] == 0x02);
		assert(out[3] == 0 && out[4] == 0);
	}

	printf("ds5: all checks passed\n");
	return 0;
}
