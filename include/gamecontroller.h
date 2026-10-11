/*
 * gamecontroller -- a small Linux-input-style game controller subsystem for
 * ESP-IDF and the STM32 HAL.
 *
 * One struct gamecontroller_dev per pad. It holds what every pad shares --
 * two sticks, two analog triggers, the buttons, the battery -- in one
 * normalised struct gamecontroller_state, and the outputs a pad may have:
 * rumble, a light bar, player LEDs. Buttons are named by position, as in
 * Linux's Documentation/input/gamepad.rst: SOUTH is Cross on a PlayStation
 * pad and A on an Xbox pad.
 *
 * The pads themselves arrive over Bluetooth, through ESP-IDF's HID host;
 * see gamecontroller_bt.h. The report parsers and output builders behind
 * them (ds4.h, ds5.h, xbox.h) are plain C on both platforms.
 *
 * Every int-returning function returns 0 or a negative errno.
 */
#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Buttons, by position (gamepad.rst). */
#define GAMECONTROLLER_BTN_SOUTH	(1u << 0)	/* Cross, A */
#define GAMECONTROLLER_BTN_EAST		(1u << 1)	/* Circle, B */
#define GAMECONTROLLER_BTN_WEST		(1u << 2)	/* Square, X */
#define GAMECONTROLLER_BTN_NORTH	(1u << 3)	/* Triangle, Y */
#define GAMECONTROLLER_BTN_TL		(1u << 4)	/* L1, LB */
#define GAMECONTROLLER_BTN_TR		(1u << 5)	/* R1, RB */
#define GAMECONTROLLER_BTN_TL2		(1u << 6)	/* L2 pressed (PlayStation) */
#define GAMECONTROLLER_BTN_TR2		(1u << 7)	/* R2 pressed (PlayStation) */
#define GAMECONTROLLER_BTN_SELECT	(1u << 8)	/* Share/Create, View */
#define GAMECONTROLLER_BTN_START	(1u << 9)	/* Options, Menu */
#define GAMECONTROLLER_BTN_MODE		(1u << 10)	/* PS, Xbox */
#define GAMECONTROLLER_BTN_THUMBL	(1u << 11)	/* L3, left stick */
#define GAMECONTROLLER_BTN_THUMBR	(1u << 12)	/* R3, right stick */
#define GAMECONTROLLER_BTN_DPAD_UP	(1u << 13)
#define GAMECONTROLLER_BTN_DPAD_DOWN	(1u << 14)
#define GAMECONTROLLER_BTN_DPAD_LEFT	(1u << 15)
#define GAMECONTROLLER_BTN_DPAD_RIGHT	(1u << 16)
#define GAMECONTROLLER_BTN_TOUCHPAD	(1u << 17)	/* touchpad click (PlayStation) */
#define GAMECONTROLLER_BTN_MUTE		(1u << 18)	/* mic mute (DualSense) */
#define GAMECONTROLLER_BTN_CAPTURE	(1u << 19)	/* Share (Xbox Series) */

#define GAMECONTROLLER_BATTERY_UNKNOWN	0xFF

/*
 * One pad, normalised. Sticks: -32768..32767, +x right, +y down -- the
 * pads' own convention, and Linux's. Triggers: 0 released .. 65535 fully
 * pressed, whatever the pad's own resolution.
 */
struct gamecontroller_state {
	int16_t lx, ly, rx, ry;
	uint16_t lt, rt;
	uint32_t buttons;		/* GAMECONTROLLER_BTN_* */
	uint8_t battery;		/* 0..100 %, or GAMECONTROLLER_BATTERY_UNKNOWN */
	bool charging;
	bool connected;
	uint32_t seq;			/* +1 for every report that changed this */
};

enum gamecontroller_kind {
	GAMECONTROLLER_DS4,		/* DualShock 4 */
	GAMECONTROLLER_DS5,		/* DualSense */
	GAMECONTROLLER_XBOX,		/* Xbox Wireless Controller, BLE */
};

struct gamecontroller_dev;

struct gamecontroller_ops {
	/* the latest state, copied out whole */
	int (*get_state)(struct gamecontroller_dev *gc,
			 struct gamecontroller_state *state);
	/* optional, each: 0..255 strong (low frequency) and weak motors */
	int (*set_rumble)(struct gamecontroller_dev *gc, uint8_t strong,
			  uint8_t weak);
	int (*set_lightbar)(struct gamecontroller_dev *gc, uint8_t red,
			    uint8_t green, uint8_t blue);
	/* bits 0..4, one LED each; the DualSense's middle LED is bit 2 */
	int (*set_player_leds)(struct gamecontroller_dev *gc, uint8_t mask);
};

struct gamecontroller_dev {
	const char *name;		/* "ds4", "ds5", "xbox"; log prefix */
	const struct gamecontroller_ops *ops;
	void *priv;
	enum gamecontroller_kind kind;
};

/* The latest state; a pad that is gone reads connected = false. */
int gamecontroller_get_state(struct gamecontroller_dev *gc,
			     struct gamecontroller_state *state);

/* -EOPNOTSUPP on a pad without the output. */
int gamecontroller_set_rumble(struct gamecontroller_dev *gc, uint8_t strong,
			      uint8_t weak);
int gamecontroller_set_lightbar(struct gamecontroller_dev *gc, uint8_t red,
				uint8_t green, uint8_t blue);
int gamecontroller_set_player_leds(struct gamecontroller_dev *gc,
				   uint8_t mask);

/* Whether a button is down in a state. */
static inline bool gamecontroller_pressed(const struct gamecontroller_state *s,
					  uint32_t button)
{
	return (s->buttons & button) != 0;
}

#ifdef __cplusplus
}
#endif

#endif /* GAMECONTROLLER_H */
