/*
 * DualSense Bluetooth report layout. "S1" is Linux v6.12
 * drivers/hid/hid-playstation.c, "S2" SDL release-2.30.9
 * src/joystick/hidapi/SDL_hidapi_ps5.c; file:line at those pins. Offsets
 * into data are after the report id, as ESP-IDF's HID host passes input
 * reports (esp_hid bt_hidh.c:700-703); output offsets are into the whole
 * report, id at [0].
 */
#pragma once

/* ---- Input ---- */
#define DS5_REPORT_FULL		0x31	/* S1:81-82, 78 bytes with the id */
#define DS5_REPORT_FULL_LEN	77	/* after the id */
#define DS5_REPORT_SIMPLE	0x01	/* S2:77-86, nine bytes after the id */
#define DS5_REPORT_SIMPLE_LEN	9

/* The full report: one byte, then the common block (S1:1346, which
 * counts from the id). */
#define DS5_FULL_COMMON		1
/* CRC-32 over 0xA1 and the report up to here (S1:1341): the last four. */
#define DS5_FULL_CRC		73

/* The common block (S1:207-227; S2:88-138 agrees): */
#define DS5_LX		0
#define DS5_LY		1
#define DS5_RX		2
#define DS5_RY		3
#define DS5_L2		4
#define DS5_R2		5
#define DS5_BUTTONS	7	/* four bytes; the first three as the DS4's */
#define DS5_STATUS	52	/* S2:134 ucBatteryLevel, the same byte */

/* The simple report is the DS4's short one: sticks, three button bytes,
 * then the triggers (S2:77-86). */
#define DS5_SIMPLE_BUTTONS	4
#define DS5_SIMPLE_L2		7
#define DS5_SIMPLE_R2		8

/* Button bits (S1:96-111; used at S1:1365-1377). */
#define DS5_BTN0_HAT		0x0F
#define DS5_BTN0_SQUARE		0x10
#define DS5_BTN0_CROSS		0x20
#define DS5_BTN0_CIRCLE		0x40
#define DS5_BTN0_TRIANGLE	0x80
#define DS5_BTN1_L1		0x01
#define DS5_BTN1_R1		0x02
#define DS5_BTN1_L2		0x04
#define DS5_BTN1_R2		0x08
#define DS5_BTN1_CREATE		0x10
#define DS5_BTN1_OPTIONS	0x20
#define DS5_BTN1_L3		0x40
#define DS5_BTN1_R3		0x80
#define DS5_BTN2_PS		0x01
#define DS5_BTN2_TOUCHPAD	0x02
#define DS5_BTN2_MUTE		0x04

/* status: level in the low nibble, charging state in the high (S1:114-116,
 * 1453-1482). */
#define DS5_STATUS_BATTERY	0x0F
#define DS5_STATUS_CHARGING_SHIFT 4

/* ---- Feature report a host reads first: calibration, 0x05 (S1:88, 972;
 * S2:72, 555). ---- */
#define DS5_FEATURE_CALIBRATION	0x05

/* ---- Output: report 0x31, 78 bytes (S1:85-86, 259-266). S1's layout:
 * seq_tag at [1] (S1:1197), tag at [2] (S1:129, 1191), the common block
 * from [3] (S1:232-256). S2:991-994 differs: 0x02 at [1] and the block
 * from [2]. ---- */
#define DS5_OUT_ID		0
#define DS5_OUT_SEQ_TAG		1	/* sequence number in the high nibble */
#define DS5_OUT_TAG		2
#define DS5_OUT_TAG_VAL		0x10
#define DS5_OUT_COMMON		3
/* Into the common block: */
#define DS5_OUT_FLAG0		0
#define DS5_OUT_FLAG1		1
#define DS5_OUT_MOTOR_WEAK	2	/* motor_right (S1:1504) */
#define DS5_OUT_MOTOR_STRONG	3	/* motor_left (S1:1503) */
#define DS5_OUT_FLAG2		38
#define DS5_OUT_LIGHTBAR_SETUP	41
#define DS5_OUT_PLAYER_LEDS	43
#define DS5_OUT_RED		44
#define DS5_OUT_GREEN		45
#define DS5_OUT_BLUE		46
#define DS5_OUT_CRC		74	/* CRC-32 over 0xA2 and [0..73] (S1:76, 1241-1247) */

/*
 * Rumble is always the original "compatible" kind, v1: COMPATIBLE_VIBRATION
 * and HAPTICS_SELECT in flag0 (S1:131-132, 1267-1271). S1 switches to v2
 * from firmware 2.21 (S1:1654) and S2 from 2.24 (S2:686-693), both after a
 * firmware read; S1:1644-1650 says "both devices also support the old
 * mode", so v1 needs no read.
 */
#define DS5_FLAG0_RUMBLE	0x03
#define DS5_FLAG1_LIGHTBAR	0x04	/* S1:135, 1278 */
#define DS5_FLAG1_PLAYER_LEDS	0x10	/* S1:137, 1287 */
#define DS5_FLAG2_LIGHTBAR_SETUP 0x02	/* S1:138, 1540 */
#define DS5_LIGHTBAR_SETUP_LIGHT_OUT 0x02 /* S1:141, 1541: fade the start-up light out */
