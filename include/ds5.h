/*
 * ds5 -- the Sony DualSense's Bluetooth reports: the input parser and the
 * output builder. Plain C, no platform: ESP-IDF's HID host feeds them in
 * gamecontroller_bt.c, and any other transport can call them directly.
 *
 * Every offset is cited in src/ds5/ds5_proto.h to Linux v6.12's
 * drivers/hid/hid-playstation.c ("S1") and SDL 2.30.9's SDL_hidapi_ps5.c
 * ("S2").
 *
 * The output report follows S1's layout: seq_tag at [1], tag 0x10 at [2],
 * the common block from [3]. SDL 2.30.9 puts the block one byte earlier,
 * with 0x02 at [1]. Unverified on hardware.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gamecontroller.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DS5_OUTPUT_LEN	78	/* the Bluetooth output report, id included */

/*
 * One input report: report_id, then data, the bytes after the id -- as
 * ESP-IDF's HID host hands them over. Understands the full report 0x31
 * (its CRC checked) and the simple 0x01 a pad sends until it is switched to
 * full reports. On success updates *state -- the simple report has no
 * battery, so that stays as it was -- bumps state->seq and returns true;
 * otherwise leaves *state alone and returns false.
 */
bool ds5_parse(uint8_t report_id, const uint8_t *data, size_t len,
	       struct gamecontroller_state *state);

/* What an output report sets: only the parts whose flag is true. */
struct ds5_output {
	bool rumble;
	uint8_t strong, weak;		/* motors, 0..255 */
	bool lightbar;
	uint8_t red, green, blue;
	bool player_leds;
	uint8_t player_mask;		/* bits 0..4, one LED each; Linux
					 * lights bit 2 alone, the middle
					 * one, for player 1 */
	bool lightbar_setup;		/* release the start-up animation */
};

/* The whole Bluetooth output report into out, id and CRC included. seq is
 * the report's sequence number, 0..15, one more for each report sent.
 * Returns DS5_OUTPUT_LEN. */
size_t ds5_build_output(const struct ds5_output *o, uint8_t seq,
			uint8_t *out);

#ifdef __cplusplus
}
#endif
