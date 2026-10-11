/*
 * Host-side check of the DualShock 4 parser and builder against the
 * layout-derived vectors in vectors.h (not captured from a pad).
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ds4.h"
#include "gamecontroller_math.h"
#include "vectors.h"

int main(void)
{
	struct gamecontroller_state s = { .battery = 77 };
	uint8_t bad[sizeof(ds4_full)];
	uint8_t out[DS4_OUTPUT_LEN];
	struct ds4_output o = { .strong = 0xC8, .weak = 0x32,
				.red = 1, .green = 2, .blue = 3 };

	/* The full report 0x11. */
	assert(ds4_parse(0x11, ds4_full, sizeof(ds4_full), &s));
	assert(s.lx == -32768 && s.ly == 128 && s.rx == 32767 && s.ry == -16320);
	assert(s.lt == 0x1010 && s.rt == 65535);
	assert(s.buttons == (GAMECONTROLLER_BTN_DPAD_RIGHT |
			     GAMECONTROLLER_BTN_SOUTH | GAMECONTROLLER_BTN_EAST |
			     GAMECONTROLLER_BTN_TL | GAMECONTROLLER_BTN_START |
			     GAMECONTROLLER_BTN_THUMBR | GAMECONTROLLER_BTN_MODE |
			     GAMECONTROLLER_BTN_TOUCHPAD));
	/* Cable in, level 5: 55 %, charging (hid-playstation.c:2367-2405). */
	assert(s.battery == 55 && s.charging && s.seq == 1);

	/* One flipped bit: the CRC refuses it, the state stays. */
	memcpy(bad, ds4_full, sizeof(bad));
	bad[2] ^= 0x01;
	assert(!ds4_parse(0x11, bad, sizeof(bad), &s));
	assert(s.lx == -32768 && s.seq == 1);
	/* Any other length, or id, is refused. */
	assert(!ds4_parse(0x11, ds4_full, sizeof(ds4_full) - 1, &s));
	assert(!ds4_parse(0x12, ds4_full, sizeof(ds4_full), &s));
	assert(!ds4_parse(0x11, NULL, 77, &s));

	/* The short report 0x01: no battery, which stays as it was. */
	assert(ds4_parse(0x01, ds4_minimal, sizeof(ds4_minimal), &s));
	assert(s.lx == 128 && s.buttons == (GAMECONTROLLER_BTN_NORTH |
						GAMECONTROLLER_BTN_SELECT));
	assert(s.battery == 55 && s.seq == 2);
	assert(!ds4_parse(0x01, ds4_minimal, 10, &s));

	/* Battery cases, without the cable and with: 10 is full, 11 full
	 * with the cable, 14 an error. */
	{
		uint8_t v[sizeof(ds4_full)];
		struct gamecontroller_state t = { 0 };
		static const struct { uint8_t status; uint8_t pct; bool chg; }
		cases[] = {
			{ 0x00, 5, false }, { 0x0A, 100, false },
			{ 0x0C, 100, false },	/* undefined, no cable: S1 says 100 */
			{ 0x19, 95, true }, { 0x1A, 100, true },
			{ 0x1B, 100, false }, { 0x1E, 0xFF, false },
		};

		for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
			memcpy(v, ds4_full, sizeof(v));
			v[2 + 29] = cases[i].status;
			/* re-signed; the CRC itself is checked in test_math */
			gc_put_le32(v + 73, gc_ps_input_crc32(0xA1, 0x11, v, 73));
			assert(ds4_parse(0x11, v, sizeof(v), &t));
			assert(t.battery == cases[i].pct && t.charging == cases[i].chg);
		}
	}

	/* The output report: every byte as S1 lays it out, CRC by zlib. */
	assert(ds4_build_output(&o, out) == DS4_OUTPUT_LEN);
	assert(!memcmp(out, ds4_out, sizeof(ds4_out)));

	printf("ds4: all checks passed\n");
	return 0;
}
