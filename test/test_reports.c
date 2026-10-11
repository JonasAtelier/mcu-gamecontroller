// Host-side check of the report parsers:  cc -I../include test_reports.c ../src/ps_reports.c -o t && ./t
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ps_reports.h"

int main(void)
{
    ps_state_t s;

    // DualShock 4, report 0x11: pad, pad, LX, LY, RX, RY, btn0..2, L2, R2
    const uint8_t ds4[] = {0xC0, 0x00, 0x00, 0x80, 0xFF, 0x80, 0x28, 0x02, 0x01, 0x00, 0xFF};
    memset(&s, 0, sizeof(s));
    assert(ps_parse(0x11, ds4, sizeof(ds4), &s));
    assert(s.lx == -128 && s.ly == 0 && s.rx == 127 && s.ry == 0);
    assert(s.cross && !s.square && s.r1 && s.ps);
    assert(!s.up && !s.down && !s.left && !s.right);   // dpad nibble 8 = released
    assert(s.l2 == 0 && s.r2 == 255);

    // DualSense, report 0x31: seq, LX, LY, RX, RY, L2, R2, seq, btn0..2
    const uint8_t ds5[] = {0x00, 0x80, 0x00, 0x80, 0xFF, 0x7F, 0x00, 0x00, 0x82, 0x40, 0x04};
    memset(&s, 0, sizeof(s));
    assert(ps_parse(0x31, ds5, sizeof(ds5), &s));
    assert(s.lx == 0 && s.ly == -128 && s.rx == 0 && s.ry == 127);
    assert(s.l2 == 0x7F && s.r2 == 0);
    assert(s.triangle && s.l3 && s.mute);
    assert(s.right && !s.up && !s.down && !s.left);    // dpad nibble 2 = east

    // Compatibility report 0x01
    const uint8_t simple[] = {0x80, 0x80, 0x80, 0x80, 0x40, 0x10, 0x00, 0x11, 0x22};
    memset(&s, 0, sizeof(s));
    assert(ps_parse(0x01, simple, sizeof(simple), &s));
    assert(s.circle && s.share && s.up && !s.right);
    assert(s.l2 == 0x11 && s.r2 == 0x22);

    // Junk and truncated reports are rejected
    assert(!ps_parse(0x11, ds4, 5, &s));
    assert(!ps_parse(0x99, ds4, sizeof(ds4), &s));

    printf("all report parsing checks passed\n");
    return 0;
}
