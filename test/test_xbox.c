/*
 * Host-side check of the Xbox BLE parser and rumble builder, against
 * layout-derived reports (not captured from a pad): the state report as
 * xpadneo's 045E:0B13 descriptor and SDL_hidapi_xboxone.c lay it out.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "xbox.h"

/* After the id: LX LY RX RY (16-bit LE), LT RT (10-bit), hat, buttons. */
static const uint8_t state[16] = {
	0x00, 0x00,	/* LX 0 */
	0xFF, 0xFF,	/* LY 65535 */
	0x00, 0x80,	/* RX centre */
	0x34, 0x12,	/* RY 0x1234 */
	0xFF, 0x03,	/* LT 1023 */
	0x00, 0x02,	/* RT 512 */
	0x03,		/* hat 3: E */
	0x01 | 0x08 | 0x80,	/* A, X, RB */
	0x04 | 0x08 | 0x20,	/* View, Menu, LS */
	0x01,		/* the extra bit */
};

int main(void)
{
	struct gamecontroller_state s = { 0 };
	struct xbox_parser p;
	uint8_t v[sizeof(state)];
	uint8_t out[XBOX_RUMBLE_LEN];
	static const uint8_t rumble[XBOX_RUMBLE_LEN] = {
		0x03, 0x03, 0x00, 0x00, 100, 50, 0xFF, 0x00, 0xFF,
	};

	assert(xbox_is_ble_pad(0x045e, 0x0b13) && xbox_is_ble_pad(0x045e, 0x0b20));
	assert(xbox_is_ble_pad(0x045e, 0x0b22));
	assert(!xbox_is_ble_pad(0x045e, 0x02fd) && !xbox_is_ble_pad(0x054c, 0x0b13));

	/* A One S on BLE firmware: the extra bit is View too. */
	xbox_parser_init(&p, 0x0b20);
	assert(xbox_parse(&p, 0x01, state, sizeof(state), &s));
	assert(s.lx == -32768 && s.ly == 32767 && s.rx == 0 && s.ry == 0x1234 - 0x8000);
	assert(s.lt == 65535 && s.rt == 32800);
	assert(s.buttons == (GAMECONTROLLER_BTN_DPAD_RIGHT |
			     GAMECONTROLLER_BTN_SOUTH | GAMECONTROLLER_BTN_WEST |
			     GAMECONTROLLER_BTN_TR | GAMECONTROLLER_BTN_SELECT |
			     GAMECONTROLLER_BTN_START | GAMECONTROLLER_BTN_THUMBL));
	assert(s.seq == 1);

	/* A Series pad: the extra bit is Share. */
	xbox_parser_init(&p, 0x0b13);
	memcpy(v, state, sizeof(v));
	v[14] = 0;		/* View up */
	assert(xbox_parse(&p, 0x01, v, sizeof(v), &s));
	assert(s.buttons & GAMECONTROLLER_BTN_CAPTURE);
	assert(!(s.buttons & GAMECONTROLLER_BTN_SELECT));

	/* Hat 0 is released; 8 is up-left. */
	v[12] = 0;
	assert(xbox_parse(&p, 0x01, v, sizeof(v), &s));
	assert(!(s.buttons & (GAMECONTROLLER_BTN_DPAD_UP | GAMECONTROLLER_BTN_DPAD_DOWN |
			      GAMECONTROLLER_BTN_DPAD_LEFT | GAMECONTROLLER_BTN_DPAD_RIGHT)));
	v[12] = 8;
	assert(xbox_parse(&p, 0x01, v, sizeof(v), &s));
	assert((s.buttons & (GAMECONTROLLER_BTN_DPAD_UP | GAMECONTROLLER_BTN_DPAD_LEFT)) ==
	       (GAMECONTROLLER_BTN_DPAD_UP | GAMECONTROLLER_BTN_DPAD_LEFT));

	/* The Xbox button: in the state report until report 0x02 appears,
	 * from then on only there. */
	v[14] = 0x10;
	assert(xbox_parse(&p, 0x01, v, sizeof(v), &s));
	assert(s.buttons & GAMECONTROLLER_BTN_MODE);
	assert(xbox_parse(&p, 0x02, (const uint8_t[]){ 0x00 }, 1, &s));
	assert(!(s.buttons & GAMECONTROLLER_BTN_MODE) && p.guide_report);
	assert(xbox_parse(&p, 0x01, v, sizeof(v), &s));
	assert(!(s.buttons & GAMECONTROLLER_BTN_MODE));
	assert(xbox_parse(&p, 0x02, (const uint8_t[]){ 0x01 }, 1, &s));
	v[14] = 0;
	assert(xbox_parse(&p, 0x01, v, sizeof(v), &s));
	assert(s.buttons & GAMECONTROLLER_BTN_MODE);

	/* Battery: four levels; type 0 unknown; charging bit. */
	assert(xbox_parse(&p, 0x04, (const uint8_t[]){ 0x04 | 0x02 }, 1, &s));
	assert(s.battery == 67 && !s.charging);
	assert(xbox_parse(&p, 0x04, (const uint8_t[]){ 0x04 | 0x10 | 0x03 }, 1, &s));
	assert(s.battery == 100 && s.charging);
	assert(xbox_parse(&p, 0x04, (const uint8_t[]){ 0x03 }, 1, &s));
	assert(s.battery == GAMECONTROLLER_BATTERY_UNKNOWN);

	/* The older 15-byte state (16 with the id) and unknown ids: refused. */
	assert(!xbox_parse(&p, 0x01, state, 15, &s));
	assert(!xbox_parse(&p, 0x05, state, sizeof(state), &s));
	assert(!xbox_parse(&p, 0x04, state, 0, &s));

	/* Rumble: report 3, both motors on, 0..100, as hid-microsoft.c builds it. */
	assert(xbox_build_rumble(255, 128, out) == XBOX_RUMBLE_LEN);
	assert(!memcmp(out, rumble, sizeof(rumble)));
	xbox_build_rumble(0, 1, out);
	assert(out[4] == 0 && out[5] == 0);

	printf("xbox: all checks passed\n");
	return 0;
}
