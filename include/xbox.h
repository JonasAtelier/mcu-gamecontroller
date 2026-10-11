/*
 * xbox -- the Xbox Wireless Controller's Bluetooth LE HID reports: the
 * input parser and the rumble builder. Plain C, no platform: ESP-IDF's
 * BLE HID host feeds them in gamecontroller_bt.c, and any other transport
 * can call them directly.
 *
 * UNVERIFIED ON HARDWARE. Written from SDL 2.30.9's SDL_hidapi_xboxone.c,
 * Linux v6.12's hid-microsoft.c and xpadneo v0.9.6's report descriptors,
 * cited in src/xbox/xbox_proto.h. Only pads on Bluetooth LE firmware: the
 * models 1708, 1797 and 1914 of hid-ids.h.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gamecontroller.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XBOX_VENDOR_MICROSOFT	0x045e
#define XBOX_RUMBLE_LEN		9	/* the rumble report, id included */

/* What the parser has to remember between reports. */
struct xbox_parser {
	bool has_share;		/* Series pads: the extra bit is Share */
	bool guide_report;	/* the Xbox button arrives in its own report */
};

/* Set up a parser for a pad of this USB product id (from the pad's
 * Device Information service). */
void xbox_parser_init(struct xbox_parser *p, uint16_t product_id);

/* Whether a vendor and product id is one of the BLE pads above. */
bool xbox_is_ble_pad(uint16_t vendor_id, uint16_t product_id);

/*
 * One input report: report_id, then data, the bytes after the id.
 * Understands the state report 0x01, the Xbox-button report 0x02 and the
 * battery report 0x04. On success updates *state, bumps state->seq and
 * returns true; otherwise leaves *state alone and returns false.
 */
bool xbox_parse(struct xbox_parser *p, uint8_t report_id, const uint8_t *data,
		size_t len, struct gamecontroller_state *state);

/* The rumble report into out, id included; strong and weak 0..255, scaled
 * to the pad's 0..100. Returns XBOX_RUMBLE_LEN. */
size_t xbox_build_rumble(uint8_t strong, uint8_t weak, uint8_t *out);

#ifdef __cplusplus
}
#endif
