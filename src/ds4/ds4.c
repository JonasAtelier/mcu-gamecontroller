/* The DualShock 4's Bluetooth reports. Plain C; built on every platform. */
#include "gamecontroller_priv.h"

#if CONFIG_GAMECONTROLLER_DS4

#include <string.h>

#include "ds4.h"
#include "ds4_proto.h"
#include "gamecontroller_math.h"

/* The input CRC seed: the HIDP header of a DATA | INPUT packet. */
#define SEED_INPUT	0xA1	/* S1:75 */
#define SEED_OUTPUT	0xA2	/* S1:76 */

/* Sticks, triggers and buttons: the first nine bytes of the common block,
 * which the short report carries too (S1:2258-2269). */
static void parse_common(const uint8_t *c, struct gamecontroller_state *s)
{
    const uint8_t *b = c + DS4_BUTTONS;
    uint32_t btn = gc_hat(b[0] & DS4_BTN0_HAT);

    s->lx = gc_axis8(c[DS4_LX]);
    s->ly = gc_axis8(c[DS4_LY]);
    s->rx = gc_axis8(c[DS4_RX]);
    s->ry = gc_axis8(c[DS4_RY]);
    s->lt = gc_trigger8(c[DS4_L2]);
    s->rt = gc_trigger8(c[DS4_R2]);

    if (b[0] & DS4_BTN0_SQUARE)   btn |= GAMECONTROLLER_BTN_WEST;
    if (b[0] & DS4_BTN0_CROSS)    btn |= GAMECONTROLLER_BTN_SOUTH;
    if (b[0] & DS4_BTN0_CIRCLE)   btn |= GAMECONTROLLER_BTN_EAST;
    if (b[0] & DS4_BTN0_TRIANGLE) btn |= GAMECONTROLLER_BTN_NORTH;
    if (b[1] & DS4_BTN1_L1)       btn |= GAMECONTROLLER_BTN_TL;
    if (b[1] & DS4_BTN1_R1)       btn |= GAMECONTROLLER_BTN_TR;
    if (b[1] & DS4_BTN1_L2)       btn |= GAMECONTROLLER_BTN_TL2;
    if (b[1] & DS4_BTN1_R2)       btn |= GAMECONTROLLER_BTN_TR2;
    if (b[1] & DS4_BTN1_SHARE)    btn |= GAMECONTROLLER_BTN_SELECT;
    if (b[1] & DS4_BTN1_OPTIONS)  btn |= GAMECONTROLLER_BTN_START;
    if (b[1] & DS4_BTN1_L3)       btn |= GAMECONTROLLER_BTN_THUMBL;
    if (b[1] & DS4_BTN1_R3)       btn |= GAMECONTROLLER_BTN_THUMBR;
    if (b[2] & DS4_BTN2_PS)       btn |= GAMECONTROLLER_BTN_MODE;
    if (b[2] & DS4_BTN2_TOUCHPAD) btn |= GAMECONTROLLER_BTN_TOUCHPAD;
    s->buttons = btn;
}

/*
 * The battery, as S1:2367-2405 reads it: 0-9 is that many tens of percent,
 * taken at the middle; 10 is full. With the cable in, 11 is full and 14 and
 * 15 are errors, for which the level is unknown.
 */
static void parse_battery(uint8_t status0, struct gamecontroller_state *s)
{
    uint8_t level = status0 & DS4_STATUS0_BATTERY;

    s->charging = false;
    if (status0 & DS4_STATUS0_CABLE) {
        if (level < 10) {
            s->battery = (uint8_t)(level * 10 + 5);
            s->charging = true;
        } else if (level == 10) {
            s->battery = 100;
            s->charging = true;
        } else if (level == DS4_BATTERY_FULL) {
            s->battery = 100;
        } else {
            s->battery = GAMECONTROLLER_BATTERY_UNKNOWN;
        }
    } else {
        s->battery = level < 10 ? (uint8_t)(level * 10 + 5) : 100;
    }
}

bool ds4_parse(uint8_t report_id, const uint8_t *data, size_t len,
               struct gamecontroller_state *state)
{
    struct gamecontroller_state s;

    if (!data || !state)
        return false;
    s = *state;

    switch (report_id) {
    case DS4_REPORT_FULL:
        if (len != DS4_REPORT_FULL_LEN ||
            gc_ps_input_crc32(SEED_INPUT, report_id, data, DS4_FULL_CRC) !=
            gc_le32(data + DS4_FULL_CRC))
            return false;
        parse_common(data + DS4_FULL_COMMON, &s);
        parse_battery(data[DS4_FULL_COMMON + DS4_STATUS0], &s);
        break;
    case DS4_REPORT_MINIMAL:
        if (len != DS4_REPORT_MINIMAL_LEN)
            return false;
        parse_common(data, &s);
        break;
    default:
        return false;
    }

    s.seq++;
    *state = s;
    return true;
}

size_t ds4_build_output(const struct ds4_output *o, uint8_t *out)
{
    memset(out, 0, DS4_OUTPUT_LEN);
    out[DS4_OUT_ID] = DS4_REPORT_FULL;
    out[DS4_OUT_HW_CONTROL] = DS4_OUT_HW_CONTROL_VAL;
    out[DS4_OUT_FLAGS] = DS4_OUT_FLAGS_VAL;
    out[DS4_OUT_MOTOR_WEAK] = o->weak;
    out[DS4_OUT_MOTOR_STRONG] = o->strong;
    out[DS4_OUT_RED] = o->red;
    out[DS4_OUT_GREEN] = o->green;
    out[DS4_OUT_BLUE] = o->blue;
    gc_put_le32(out + DS4_OUT_CRC, gc_ps_crc32(SEED_OUTPUT, out, DS4_OUT_CRC));
    return DS4_OUTPUT_LEN;
}

#endif /* CONFIG_GAMECONTROLLER_DS4 */
