# Pad reference

[Back to README](../README.md)

Paths mentioned in this guide are relative to the repository root.

Every offset and bit behind the parsers and builders is cited, file:line,
in `src/<pad>/<pad>_proto.h`, to these sources at these pins:

| Tag | Source | Licence |
|---|---|---|
| S1 | Linux v6.12 `drivers/hid/hid-playstation.c` -- written by Sony Interactive Entertainment | GPL-2.0-or-later |
| S2 | SDL release-2.30.9 `src/joystick/hidapi/SDL_hidapi_ps4.c`, `SDL_hidapi_ps5.c` | zlib |
| S3 | SDL release-2.30.9 `SDL_hidapi_xboxone.c` | zlib |
| M, I | Linux v6.12 `drivers/hid/hid-microsoft.c`, `hid-ids.h` | GPL-2.0 |
| XS, X1, XN | xpadneo v0.9.6: the Series and One S report descriptors, `hid-xpadneo.c` | GPL-2.0 |

Only facts are taken from them -- offsets, bit positions, values -- no code
or comments. Offsets here count after the report id, the way ESP-IDF's HID
host hands input reports over (esp_hid `bt_hidh.c:700-703`, IDF v5.5.5);
output reports are built whole, id at [0], and the HID host is given them
without it (`bt_hidh.c:783-790` puts it back).

The test vectors are **derived from these layouts, not captured from
pads**; see [development](development.md#tests).

## DualShock 4

Input:

| Report | Length after the id | Layout |
|---|---|---|
| 0x11, full | 77 | two bytes, then the common block at [2]; CRC-32 at [73..76] |
| 0x01, short | 9 | the first nine bytes of the common block |

The common block (S1:425-440; S2:78-102 agrees): LX LY RX RY at 0..3, three
button bytes at 4..6, L2 R2 at 7..8, status at 29. The buttons are the
DualSense's (S1:96-111): hat 0..7 from north clockwise, 8 released, in the
low nibble; Square, Cross, Circle, Triangle; L1, R1, L2, R2, Share,
Options, L3, R3; PS, touchpad.

Battery, status[0] (S1:321-324, 2367-2405): the low nibble is tens of
percent, read at the middle (0 is 5 %); bit 4 is the cable. Without it, 10
is 100 %. With it, the pad is charging, 10 is 100 % charging, 11 full, and
14 and 15 are errors -- battery unknown.

Full reports come once the host reads feature report 0x02 (which "switches
them into advanced report mode", S2:462-463) and 0x05, the Bluetooth
calibration (S1:1822, S2:472).

Output, report 0x11, 78 bytes (S1:487-494; S2:852-864): `hw_control` 0xC4
-- HID data, CRC, a 4 ms report interval -- at [1]; flags 0x03, motor and
LED, at [3], always both together, as S1:2147-2163 does for third-party
pads; weak motor [6], strong [7]; red, green, blue [8..10]; CRC-32 at
[74..77]. So `set_rumble` resends the last light bar colour and
`set_lightbar` the last rumble.

## DualSense

Input:

| Report | Length after the id | Layout |
|---|---|---|
| 0x31, full | 77 | one byte, then the common block at [1]; CRC-32 at [73..76] |
| 0x01, simple | 9 | the DualShock 4's short layout (S2:77-86) |

The common block (S1:207-227; S2:88-138 agrees): LX LY RX RY at 0..3, L2 R2
at 4..5, a counter at 6, buttons at 7..10 -- the DualShock 4's, plus mic
mute -- and status at 52.

Battery, status (S1:1453-1482): the low nibble is tens of percent at the
middle, capped at 100; the high nibble is 0 discharging, 1 charging, 2
full, anything else an error -- battery unknown.

esp-ps-controller parsed the simple report 0x01 with the DualShock 4's
layout; S2:77-86 confirms it is the same.

### The output report: S1, not SDL

Report 0x31, 78 bytes. **This library follows S1's layout**: `seq_tag` at
[1], a sequence number in the high nibble, +1 per report (S1:1197); tag
0x10 at [2] (S1:129, 1191, "Tag must be set. Exact meaning is unclear.");
the common block from [3] (S1:232-266). **SDL 2.30.9 puts the block one
byte earlier, with 0x02 at [1]** (S2:991-994). The two disagree on every
output byte; S1 is Sony's own driver, so S1 it is. **Unverified on a pad.**
`test/test_ds5.c` pins every S1 offset.

Into the common block: flags at 0, 1 and 38; weak motor 2, strong 3;
light-bar setup 41; player LEDs 43; red, green, blue 44..46. CRC-32 at
[74..77].

Before the first light-bar or player-LED report, the library sends one
releasing the light bar from its start-up animation (flag2 bit 1, setup
value 2: S1:1532-1542), as S1 does.

### Rumble

Always the original "compatible" rumble: flag0 = COMPATIBLE_VIBRATION |
HAPTICS_SELECT (S1:131-132, 1267-1271). Newer firmware offers a "v2"
rumble -- S1 uses it from feature version 2.21 (S1:1654), S2 from firmware
2.24 (S2:686-693), each after reading the firmware version -- but S1:1644-1650
says "both devices also support the old mode", so the old mode needs no
firmware read and behaves the same on every pad. It may feel different
from a console's rumble on new firmware.

## Xbox Wireless Controller (unverified)

**Never run with a pad.** BLE firmware only: model 1708 (0x0b20), 1797
(0x0b22), 1914 (0x0b13) (I:947-951; M:452-459 gives them rumble).

Input:

| Report | Length after the id | What |
|---|---|---|
| 0x01 | 16 or more | the state (S3:964-976; XS) |
| 0x02 | 1 | the Xbox button alone, bit 0 (S3:1048-1052; X1) |
| 0x04 | 1 | battery flags (S3:1054-1075; XN:548-550) |

The state: LX LY RX RY as 16-bit little-endian at 0, 2, 4, 6, centred on
0x8000; LT, RT as 10 bits at 8 and 10; the hat at 12, 1..8 from north
clockwise, 0 released; buttons at 13..15 (S3:875-899): A, B, X, Y, LB, RB;
View, Menu, Xbox, left and right stick; and one extra bit at 15, which is
Share on a Series pad and View on the others (S3:893-897; XS: Consumer
Record; X1: AC Back). A 15-byte state is the older One S firmware's and is
refused.

Once the pad has sent report 0x02, the Xbox button is taken from there
only (S3:885-887).

Battery flags: bits 0..1 a level, low to full (XN:559-564), shown as 0, 33,
67 or 100 % -- a level, not a measurement; bits 2..3 the type, 0 for no
reading (S3:1057; XN:548); bit 4 charging (XN:549).

Rumble, report 0x03 (M:40-57, 281-303; S3:490-499; the descriptors' PID
output report 3): enable 0x03, weak and strong motors (M:41-42, 291);
magnitudes 0..100 for the left and right trigger motors, the strong and the
weak motor at [2..5] (M:44-48, 299-300); duration 255 ×10 ms, delay 0,
loop count 255 (M:297-298; S3 sends 0xEB). The trigger motors are left at
0.

## Provenance

Ported from esp-ps-controller main (dfadc0a): its pure parser
`ps_reports.c`, header and test unchanged, and its Bluetooth Classic glue,
reworked into `gamecontroller_bt.c` with the fixes listed in
[API usage](usage.md#changes-from-esp-ps-controller). Its offsets agree
with S1 and S2. The DualShock 4 and DualSense parsers and builders are new,
from S1 and S2; the Xbox pad is new, from S3, M, I and xpadneo.
