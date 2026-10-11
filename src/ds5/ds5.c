/* The DualSense's Bluetooth reports. Plain C; built on every platform. */
#include "gamecontroller_priv.h"

#if CONFIG_GAMECONTROLLER_DS5

#include <string.h>

#include "ds5.h"
#include "ds5_proto.h"
#include "gamecontroller_math.h"

#define SEED_INPUT	0xA1	/* S1:75 */
#define SEED_OUTPUT	0xA2	/* S1:76 */

static uint32_t parse_buttons(const uint8_t *b)
{
    uint32_t btn = gc_hat(b[0] & DS5_BTN0_HAT);

    if (b[0] & DS5_BTN0_SQUARE)   btn |= GAMECONTROLLER_BTN_WEST;
    if (b[0] & DS5_BTN0_CROSS)    btn |= GAMECONTROLLER_BTN_SOUTH;
    if (b[0] & DS5_BTN0_CIRCLE)   btn |= GAMECONTROLLER_BTN_EAST;
    if (b[0] & DS5_BTN0_TRIANGLE) btn |= GAMECONTROLLER_BTN_NORTH;
    if (b[1] & DS5_BTN1_L1)       btn |= GAMECONTROLLER_BTN_TL;
    if (b[1] & DS5_BTN1_R1)       btn |= GAMECONTROLLER_BTN_TR;
    if (b[1] & DS5_BTN1_L2)       btn |= GAMECONTROLLER_BTN_TL2;
    if (b[1] & DS5_BTN1_R2)       btn |= GAMECONTROLLER_BTN_TR2;
    if (b[1] & DS5_BTN1_CREATE)   btn |= GAMECONTROLLER_BTN_SELECT;
    if (b[1] & DS5_BTN1_OPTIONS)  btn |= GAMECONTROLLER_BTN_START;
    if (b[1] & DS5_BTN1_L3)       btn |= GAMECONTROLLER_BTN_THUMBL;
    if (b[1] & DS5_BTN1_R3)       btn |= GAMECONTROLLER_BTN_THUMBR;
    if (b[2] & DS5_BTN2_PS)       btn |= GAMECONTROLLER_BTN_MODE;
    if (b[2] & DS5_BTN2_TOUCHPAD) btn |= GAMECONTROLLER_BTN_TOUCHPAD;
    if (b[2] & DS5_BTN2_MUTE)     btn |= GAMECONTROLLER_BTN_MUTE;
    return btn;
}

/*
 * The battery, as S1:1453-1482 reads it. Charging state 0: discharging,
 * 1: charging, both with tens of percent taken at the middle and capped
 * at 100; 2: full; anything else (0xa, 0xb errors, 0xf) unknown.
 */
static void parse_battery(uint8_t status, struct gamecontroller_state *s)
{
    uint8_t level = status & DS5_STATUS_BATTERY;
    uint8_t pct = level >= 10 ? 100 : (uint8_t)(level * 10 + 5);

    switch (status >> DS5_STATUS_CHARGING_SHIFT) {
    case 0x0:
        s->battery = pct;
        s->charging = false;
        break;
    case 0x1:
        s->battery = pct;
        s->charging = true;
        break;
    case 0x2:
        s->battery = 100;
        s->charging = false;
        break;
    default:
        s->battery = GAMECONTROLLER_BATTERY_UNKNOWN;
        s->charging = false;
        break;
    }
}

bool ds5_parse(uint8_t report_id, const uint8_t *data, size_t len,
               struct gamecontroller_state *state)
{
    struct gamecontroller_state s;
    const uint8_t *c;

    if (!data || !state)
        return false;
    s = *state;

    switch (report_id) {
    case DS5_REPORT_FULL:
        if (len != DS5_REPORT_FULL_LEN ||
            gc_ps_input_crc32(SEED_INPUT, report_id, data, DS5_FULL_CRC) !=
            gc_le32(data + DS5_FULL_CRC))
            return false;
        c = data + DS5_FULL_COMMON;
        s.lx = gc_axis8(c[DS5_LX]);
        s.ly = gc_axis8(c[DS5_LY]);
        s.rx = gc_axis8(c[DS5_RX]);
        s.ry = gc_axis8(c[DS5_RY]);
        s.lt = gc_trigger8(c[DS5_L2]);
        s.rt = gc_trigger8(c[DS5_R2]);
        s.buttons = parse_buttons(c + DS5_BUTTONS);
        parse_battery(c[DS5_STATUS], &s);
        break;
    case DS5_REPORT_SIMPLE:
        if (len != DS5_REPORT_SIMPLE_LEN)
            return false;
        s.lx = gc_axis8(data[DS5_LX]);
        s.ly = gc_axis8(data[DS5_LY]);
        s.rx = gc_axis8(data[DS5_RX]);
        s.ry = gc_axis8(data[DS5_RY]);
        s.lt = gc_trigger8(data[DS5_SIMPLE_L2]);
        s.rt = gc_trigger8(data[DS5_SIMPLE_R2]);
        s.buttons = parse_buttons(data + DS5_SIMPLE_BUTTONS);
        break;
    default:
        return false;
    }

    s.seq++;
    *state = s;
    return true;
}

size_t ds5_build_output(const struct ds5_output *o, uint8_t seq,
                        uint8_t *out)
{
    uint8_t *c = out + DS5_OUT_COMMON;

    memset(out, 0, DS5_OUTPUT_LEN);
    out[DS5_OUT_ID] = DS5_REPORT_FULL;
    out[DS5_OUT_SEQ_TAG] = (uint8_t)((seq & 0x0F) << 4);
    out[DS5_OUT_TAG] = DS5_OUT_TAG_VAL;

    if (o->rumble) {
        c[DS5_OUT_FLAG0] |= DS5_FLAG0_RUMBLE;
        c[DS5_OUT_MOTOR_WEAK] = o->weak;
        c[DS5_OUT_MOTOR_STRONG] = o->strong;
    }
    if (o->lightbar) {
        c[DS5_OUT_FLAG1] |= DS5_FLAG1_LIGHTBAR;
        c[DS5_OUT_RED] = o->red;
        c[DS5_OUT_GREEN] = o->green;
        c[DS5_OUT_BLUE] = o->blue;
    }
    if (o->player_leds) {
        c[DS5_OUT_FLAG1] |= DS5_FLAG1_PLAYER_LEDS;
        c[DS5_OUT_PLAYER_LEDS] = o->player_mask & 0x1F;
    }
    if (o->lightbar_setup) {
        c[DS5_OUT_FLAG2] |= DS5_FLAG2_LIGHTBAR_SETUP;
        c[DS5_OUT_LIGHTBAR_SETUP] = DS5_LIGHTBAR_SETUP_LIGHT_OUT;
    }
    gc_put_le32(out + DS5_OUT_CRC, gc_ps_crc32(SEED_OUTPUT, out, DS5_OUT_CRC));
    return DS5_OUTPUT_LEN;
}

#endif /* CONFIG_GAMECONTROLLER_DS5 */
