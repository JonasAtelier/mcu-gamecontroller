/*
 * ds4 -- the Sony DualShock 4's Bluetooth reports: the input parser and the
 * output builder. Plain C, no platform: ESP-IDF's HID host feeds them in
 * gamecontroller_bt.c, and any other transport can call them directly.
 *
 * Every offset is cited in src/ds4/ds4_proto.h to Linux v6.12's
 * drivers/hid/hid-playstation.c and SDL 2.30.9's SDL_hidapi_ps4.c.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gamecontroller.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DS4_OUTPUT_LEN	78	/* the Bluetooth output report, id included */

/*
 * One input report: report_id, then data, the bytes after the id -- as
 * ESP-IDF's HID host hands them over. Understands the full report 0x11
 * (its CRC checked) and the short 0x01 a pad sends until it is switched to
 * full reports. On success updates *state -- the short report has no
 * battery, so that stays as it was -- bumps state->seq and returns true;
 * otherwise leaves *state alone and returns false.
 */
bool ds4_parse(uint8_t report_id, const uint8_t *data, size_t len,
	       struct gamecontroller_state *state);

/* What an output report sets. Rumble and light bar always go together. */
struct ds4_output {
	uint8_t strong, weak;		/* motors, 0..255 */
	uint8_t red, green, blue;	/* light bar */
};

/* The whole Bluetooth output report into out, id and CRC included;
 * returns DS4_OUTPUT_LEN. */
size_t ds4_build_output(const struct ds4_output *o, uint8_t *out);

#ifdef __cplusplus
}
#endif
