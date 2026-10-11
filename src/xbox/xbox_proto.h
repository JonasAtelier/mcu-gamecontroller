/*
 * Xbox Wireless Controller, Bluetooth LE HID. Sources, pinned:
 *   S3  SDL release-2.30.9 src/joystick/hidapi/SDL_hidapi_xboxone.c
 *   M   Linux v6.12 drivers/hid/hid-microsoft.c
 *   I   Linux v6.12 drivers/hid/hid-ids.h
 *   XS  xpadneo v0.9.6 docs/descriptors/xbxs.md (045E:0B13, Series)
 *   X1  xpadneo v0.9.6 docs/descriptors/xb1s_linux.md (One S)
 *   XN  xpadneo v0.9.6 hid-xpadneo.c
 * Offsets into data are after the report id. S3 counts from the id, so
 * its data[n] is our n - 1.
 */
#pragma once

/* ---- Pads (I:947-951; M:452-459 gives them rumble) ---- */
#define XBOX_PID_1708_BLE	0x0b20
#define XBOX_PID_1797_BLE	0x0b22
#define XBOX_PID_1914		0x0b13	/* Series: has Share (S3:169-172) */

/* ---- Report 0x01, the state: 16 bytes after the id on BLE firmware
 * (S3:964-976: 16 with the id is the older One S packet, more is this
 * one; XS: 128 bits). ---- */
#define XBOX_REPORT_STATE	0x01
#define XBOX_STATE_MIN_LEN	16
#define XBOX_LX		0	/* 16-bit LE, 0..65534 centred on 0x8000 (XS; S3:1036-1043) */
#define XBOX_LY		2
#define XBOX_RX		4	/* XS: Z */
#define XBOX_RY		6	/* XS: Rz */
#define XBOX_LT		8	/* 10 bits, 0..1023 (XS: Brake; S3:1024-1034) */
#define XBOX_RT		10	/* XS: Accelerator */
#define XBOX_HAT	12	/* 1..8 = N clockwise, 0 released (XS; S3:986-1016) */
#define XBOX_BTN0	13
#define XBOX_BTN1	14
#define XBOX_BTN2	15

/* S3:875-899. */
#define XBOX_BTN0_A		0x01
#define XBOX_BTN0_B		0x02
#define XBOX_BTN0_X		0x08
#define XBOX_BTN0_Y		0x10
#define XBOX_BTN0_LB		0x40
#define XBOX_BTN0_RB		0x80
#define XBOX_BTN1_VIEW		0x04	/* Back (S3:893-897) */
#define XBOX_BTN1_MENU		0x08	/* Start */
#define XBOX_BTN1_XBOX		0x10	/* Guide, unless report 0x02 brings it (S3:885-887) */
#define XBOX_BTN1_LS		0x20
#define XBOX_BTN1_RS		0x40
/* Share on a Series pad (XS: Consumer Record), View on others (X1:
 * Consumer AC Back); S3:893-897. */
#define XBOX_BTN2_EXTRA		0x01

/* ---- Report 0x02, the Xbox button alone (S3:1048-1052; X1: AC Home) ---- */
#define XBOX_REPORT_GUIDE	0x02
#define XBOX_GUIDE_BIT		0x01

/* ---- Report 0x04, the battery: one byte of flags (S3:1054-1075; XN:548-550) ---- */
#define XBOX_REPORT_BATTERY	0x04
#define XBOX_BATTERY_LEVEL	0x03	/* 0..3, low to full (XN:559-564) */
#define XBOX_BATTERY_TYPE	0x0C	/* 0: no battery reading (S3:1057; XN:548) */
#define XBOX_BATTERY_CHARGING	0x10	/* XN:549 */

/* ---- Report 0x03, rumble (M:40-57, 281-303; S3:490-499; XS and X1: PID
 * page output report 3) ---- */
#define XBOX_REPORT_RUMBLE	0x03
#define XBOX_RUMBLE_ENABLE	1
#define XBOX_RUMBLE_ENABLE_VAL	0x03	/* weak | strong motor (M:41-42, 291) */
#define XBOX_RUMBLE_STRONG	4	/* magnitude[2], left (M:44-48, 299) */
#define XBOX_RUMBLE_WEAK	5	/* magnitude[3], right (M:300) */
#define XBOX_RUMBLE_MAX		100	/* magnitudes 0..100 (M:319-320; XS: Logical Max 100) */
#define XBOX_RUMBLE_DURATION	6	/* 10 ms units; 255, as M:297 */
#define XBOX_RUMBLE_LOOP	8	/* 255, as M:298 (S3 sends 0xEB) */
