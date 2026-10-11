#include "ps_reports.h"

// dpad nibble 0..7 = N, NE, E, SE, S, SW, W, NW; 8 = released.
static void parse_dpad(uint8_t v, ps_state_t *s)
{
    static const uint8_t dirs[8] = {0x1, 0x3, 0x2, 0x6, 0x4, 0xC, 0x8, 0x9}; // U|R|D|L
    uint8_t b = (v < 8) ? dirs[v] : 0;
    s->up = b & 0x1; s->right = b & 0x2; s->down = b & 0x4; s->left = b & 0x8;
}

// DS4 and DualSense use the same 3 button bytes.
static void parse_buttons(const uint8_t *b, ps_state_t *s)
{
    parse_dpad(b[0] & 0x0F, s);
    s->square   = b[0] & 0x10; s->cross   = b[0] & 0x20;
    s->circle   = b[0] & 0x40; s->triangle= b[0] & 0x80;
    s->l1       = b[1] & 0x01; s->r1      = b[1] & 0x02;
    /* b[1] & 0x04 / 0x08 are the digital L2/R2 — the analog values say the same thing */
    s->share    = b[1] & 0x10; s->options = b[1] & 0x20;
    s->l3       = b[1] & 0x40; s->r3      = b[1] & 0x80;
    s->ps       = b[2] & 0x01; s->touchpad= b[2] & 0x02;
    s->mute     = b[2] & 0x04;
}

static int8_t axis(uint8_t v) { return (int8_t)(v - 128); }

static void parse_axes(const uint8_t *c, ps_state_t *s)
{
    s->lx = axis(c[0]); s->ly = axis(c[1]);
    s->rx = axis(c[2]); s->ry = axis(c[3]);
}

bool ps_parse(uint8_t report_id, const uint8_t *data, size_t len, ps_state_t *out)
{
    switch (report_id) {
    case 0x11:                       // DualShock 4, full report over BT
        if (len < 11) return false;
        parse_axes(data + 2, out);           // 2 pad bytes before the payload
        parse_buttons(data + 6, out);
        out->l2 = data[9]; out->r2 = data[10];
        return true;

    case 0x31:                       // DualSense, full report over BT
        if (len < 11) return false;
        parse_axes(data + 1, out);           // 1 seq/tag byte before the payload
        out->l2 = data[5]; out->r2 = data[6];
        parse_buttons(data + 8, out);
        return true;

    case 0x01:                       // compatibility report — both pads send this before
        if (len < 9) return false;   // the host asks for the full one
        parse_axes(data, out);
        parse_buttons(data + 4, out);
        out->l2 = data[7]; out->r2 = data[8];
        return true;

    default:
        return false;
    }
}
