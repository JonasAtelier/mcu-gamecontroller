/*
 * DualShock 4 Bluetooth report layout. "S1" is Linux v6.12
 * drivers/hid/hid-playstation.c, "S2" SDL release-2.30.9
 * src/joystick/hidapi/SDL_hidapi_ps4.c; file:line at those pins. Offsets
 * into data are after the report id, as ESP-IDF's HID host passes input
 * reports (esp_hid bt_hidh.c:700-703); output offsets are into the whole
 * report, id at [0].
 */
#pragma once

/* ---- Input ---- */
#define DS4_REPORT_FULL		0x11	/* S1:297-298, 78 bytes with the id */
#define DS4_REPORT_FULL_LEN	77	/* after the id */
#define DS4_REPORT_MINIMAL	0x01	/* S1:295-296, 10 bytes with the id */
#define DS4_REPORT_MINIMAL_LEN	9

/* The full report: two bytes, then the common block (S1:452-460). */
#define DS4_FULL_COMMON		2
/* CRC-32 over 0xA1 and the report up to here, little-endian
 * (S1:75, 2250): the last four bytes. */
#define DS4_FULL_CRC		73

/* The common block (S1:425-440; S2:78-102 agrees): */
#define DS4_LX		0
#define DS4_LY		1
#define DS4_RX		2
#define DS4_RY		3
#define DS4_BUTTONS	4	/* three bytes */
#define DS4_L2		7
#define DS4_R2		8
#define DS4_STATUS0	29	/* S2:96 ucBatteryLevel, the same byte */

/* Button bits, shared with the DualSense (S1:96-111, used for the DS4 at
 * S1:2288-2300). */
#define DS4_BTN0_HAT		0x0F	/* 0..7 N, NE, .. NW; 8 released */
#define DS4_BTN0_SQUARE		0x10
#define DS4_BTN0_CROSS		0x20
#define DS4_BTN0_CIRCLE		0x40
#define DS4_BTN0_TRIANGLE	0x80
#define DS4_BTN1_L1		0x01
#define DS4_BTN1_R1		0x02
#define DS4_BTN1_L2		0x04
#define DS4_BTN1_R2		0x08
#define DS4_BTN1_SHARE		0x10
#define DS4_BTN1_OPTIONS	0x20
#define DS4_BTN1_L3		0x40
#define DS4_BTN1_R3		0x80
#define DS4_BTN2_PS		0x01
#define DS4_BTN2_TOUCHPAD	0x02

/* status[0]: battery in the low nibble, cable in bit 4 (S1:321-324). */
#define DS4_STATUS0_BATTERY	0x0F
#define DS4_STATUS0_CABLE	0x10
#define DS4_BATTERY_FULL	11	/* with the cable in (S1:324, 2390) */

/* ---- Feature reports a host reads to get full reports: 0x02, which "for
 * Bluetooth controllers ... switches them into advanced report mode"
 * (S2:462-463), and 0x05, the Bluetooth calibration report (S1:306,
 * 1822; S2:472). ---- */
#define DS4_FEATURE_FULL_1	0x02
#define DS4_FEATURE_FULL_2	0x05

/* ---- Output: report 0x11, 78 bytes (S1:301-302, 487-494; S2:852-864) ---- */
#define DS4_OUT_ID		0
#define DS4_OUT_HW_CONTROL	1	/* HID | CRC32, and the poll interval */
#define DS4_OUT_FLAGS		3	/* valid_flag0 */
#define DS4_OUT_MOTOR_WEAK	6	/* motor_right (S1:2484) */
#define DS4_OUT_MOTOR_STRONG	7	/* motor_left (S1:2483) */
#define DS4_OUT_RED		8
#define DS4_OUT_GREEN		9
#define DS4_OUT_BLUE		10
#define DS4_OUT_CRC		74	/* CRC-32 over 0xA2 and [0..73] (S1:76, 2209-2213) */

/* HID data present, CRC present (S1:342-343, 2203), and a 4 ms poll
 * interval (S1:339-341): S2:853 sends the same 0xC0 | 0x04. */
#define DS4_OUT_HW_CONTROL_VAL	0xC4
/* Motor and LED always together, for third-party pads (S1:2147-2163,
 * 346-347); S2:854 sets the same 0x03. */
#define DS4_OUT_FLAGS_VAL	0x03
