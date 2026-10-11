# Writes test/vectors.h: layout-derived test vectors for mcu-gamecontroller,
# written from the layouts cited in src/<pad>/<pad>_proto.h independently of
# the C code; the CRCs by zlib.crc32 (CRC-32/ISO-HDLC).
#
#   python3 test/gen_vectors.py > test/vectors.h
import zlib

print("""/*
 * Layout-derived test vectors -- NOT captured from a pad. Written by
 * gen_vectors.py from the layouts cited in src/<pad>/<pad>_proto.h, apart
 * from the C code; the CRCs are zlib.crc32. Regenerate:
 *
 *   python3 test/gen_vectors.py > test/vectors.h
 */
#pragma once

#include <stdint.h>
""")

def c(name, b):
    print(f"static const uint8_t {name}[{len(b)}] = {{")
    for i in range(0, len(b), 12):
        print("\t" + " ".join(f"0x{x:02X}," for x in b[i:i+12]))
    print("};")
def crc(seed, data): return zlib.crc32(bytes([seed]) + bytes(data)) & 0xFFFFFFFF
le = lambda v: [v & 0xFF, v >> 8 & 0xFF, v >> 16 & 0xFF, v >> 24 & 0xFF]

print(f"/* CRC-32 of '123456789': 0x{zlib.crc32(b'123456789'):08X} */")
# DS4 full input 0x11: data after id, 77 bytes: [0..1] pad, common at 2
d = [0] * 77
d[0], d[1] = 0xC0, 0x00
com = 2
d[com+0], d[com+1], d[com+2], d[com+3] = 0x00, 0x80, 0xFF, 0x40
d[com+4] = 0x02 | 0x20 | 0x40       # hat E, cross, circle
d[com+5] = 0x01 | 0x20 | 0x80       # L1, options, R3
d[com+6] = 0x01 | 0x02              # PS, touchpad
d[com+7], d[com+8] = 0x10, 0xFF
d[com+29] = 0x10 | 5                # cable, level 5
d[73:77] = le(crc(0xA1, [0x11] + d[:73]))
c("ds4_full", d)
# DS4 minimal 0x01: 9 bytes
c("ds4_minimal", [0x80, 0x80, 0x80, 0x80, 0x08 | 0x80, 0x10, 0x00, 0x00, 0x00])
# DS5 full input 0x31: data after id, 77 bytes: [0] seq/tag, common at 1
d = [0] * 77
com = 1
d[com+0], d[com+1], d[com+2], d[com+3] = 0x80, 0x00, 0x80, 0xFF
d[com+4], d[com+5] = 0x7F, 0x00     # L2, R2
d[com+7] = 0x05 | 0x80              # hat 5 = SW, triangle
d[com+8] = 0x02 | 0x40              # R1, L3
d[com+9] = 0x04                     # mute
d[com+52] = 0x10 | 7                # charging, level 7
d[73:77] = le(crc(0xA1, [0x31] + d[:73]))
c("ds5_full", d)
# DS5 output: S1 layout, rumble strong 0xC8 weak 0x32, lightbar 10,20,30,
# player leds 0x04, seq 3
o = [0] * 78
o[0], o[1], o[2] = 0x31, 3 << 4, 0x10
cm = 3
o[cm+0] = 0x03
o[cm+1] = 0x04 | 0x10
o[cm+2], o[cm+3] = 0x32, 0xC8       # motor_right weak, motor_left strong
o[cm+43] = 0x04
o[cm+44], o[cm+45], o[cm+46] = 10, 20, 30
o[74:78] = le(crc(0xA2, o[:74]))
c("ds5_out", o)
# DS5 lightbar setup only, seq 0
o = [0] * 78
o[0], o[1], o[2] = 0x31, 0, 0x10
o[3+38] = 0x02
o[3+41] = 0x02
o[74:78] = le(crc(0xA2, o[:74]))
c("ds5_out_setup", o)
# DS4 output: strong 0xC8 weak 0x32, rgb 1,2,3
o = [0] * 78
o[0], o[1], o[3] = 0x11, 0xC4, 0x03
o[6], o[7], o[8], o[9], o[10] = 0x32, 0xC8, 1, 2, 3
o[74:78] = le(crc(0xA2, o[:74]))
c("ds4_out", o)
