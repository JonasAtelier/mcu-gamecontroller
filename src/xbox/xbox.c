/* The Xbox Wireless Controller's BLE reports. Plain C; built on every
 * platform. Unverified on hardware. */
#include "gamecontroller_priv.h"

#if CONFIG_GAMECONTROLLER_XBOX

#include <string.h>

#include "gamecontroller_math.h"
#include "xbox.h"
#include "xbox_proto.h"

void xbox_parser_init(struct xbox_parser *p, uint16_t product_id)
{
    p->has_share = product_id == XBOX_PID_1914;
    p->guide_report = false;
}

bool xbox_is_ble_pad(uint16_t vendor_id, uint16_t product_id)
{
    return vendor_id == XBOX_VENDOR_MICROSOFT &&
           (product_id == XBOX_PID_1708_BLE || product_id == XBOX_PID_1797_BLE ||
            product_id == XBOX_PID_1914);
}

static void parse_state(const struct xbox_parser *p, const uint8_t *d,
                        struct gamecontroller_state *s)
{
    uint8_t hat = d[XBOX_HAT] & 0x0F;
    uint32_t btn = hat >= 1 && hat <= 8 ? gc_hat(hat - 1u) : 0;

    s->lx = gc_axis16(gc_le16(d + XBOX_LX));
    s->ly = gc_axis16(gc_le16(d + XBOX_LY));
    s->rx = gc_axis16(gc_le16(d + XBOX_RX));
    s->ry = gc_axis16(gc_le16(d + XBOX_RY));
    s->lt = gc_trigger10(gc_le16(d + XBOX_LT) & 0x3FF);
    s->rt = gc_trigger10(gc_le16(d + XBOX_RT) & 0x3FF);

    if (d[XBOX_BTN0] & XBOX_BTN0_A)  btn |= GAMECONTROLLER_BTN_SOUTH;
    if (d[XBOX_BTN0] & XBOX_BTN0_B)  btn |= GAMECONTROLLER_BTN_EAST;
    if (d[XBOX_BTN0] & XBOX_BTN0_X)  btn |= GAMECONTROLLER_BTN_WEST;
    if (d[XBOX_BTN0] & XBOX_BTN0_Y)  btn |= GAMECONTROLLER_BTN_NORTH;
    if (d[XBOX_BTN0] & XBOX_BTN0_LB) btn |= GAMECONTROLLER_BTN_TL;
    if (d[XBOX_BTN0] & XBOX_BTN0_RB) btn |= GAMECONTROLLER_BTN_TR;
    if (d[XBOX_BTN1] & XBOX_BTN1_VIEW) btn |= GAMECONTROLLER_BTN_SELECT;
    if (d[XBOX_BTN1] & XBOX_BTN1_MENU) btn |= GAMECONTROLLER_BTN_START;
    if (d[XBOX_BTN1] & XBOX_BTN1_LS)   btn |= GAMECONTROLLER_BTN_THUMBL;
    if (d[XBOX_BTN1] & XBOX_BTN1_RS)   btn |= GAMECONTROLLER_BTN_THUMBR;
    if (d[XBOX_BTN2] & XBOX_BTN2_EXTRA)
        btn |= p->has_share ? GAMECONTROLLER_BTN_CAPTURE : GAMECONTROLLER_BTN_SELECT;

    /* Once the pad has sent the Xbox button on its own, that report owns
     * it (S3:885-887). */
    if (p->guide_report)
        btn |= s->buttons & GAMECONTROLLER_BTN_MODE;
    else if (d[XBOX_BTN1] & XBOX_BTN1_XBOX)
        btn |= GAMECONTROLLER_BTN_MODE;
    s->buttons = btn;
}

/* Four levels, low to full (XN:559-564), as 0/33/67/100 %: a level, not a
 * measurement. A type of 0 is no battery reading. */
static void parse_battery(uint8_t flags, struct gamecontroller_state *s)
{
    static const uint8_t pct[4] = { 0, 33, 67, 100 };

    s->charging = (flags & XBOX_BATTERY_CHARGING) != 0;
    s->battery = (flags & XBOX_BATTERY_TYPE) ?
                 pct[flags & XBOX_BATTERY_LEVEL] : GAMECONTROLLER_BATTERY_UNKNOWN;
}

bool xbox_parse(struct xbox_parser *p, uint8_t report_id, const uint8_t *data,
                size_t len, struct gamecontroller_state *state)
{
    struct gamecontroller_state s;

    if (!p || !data || !state)
        return false;
    s = *state;

    switch (report_id) {
    case XBOX_REPORT_STATE:
        if (len < XBOX_STATE_MIN_LEN)
            return false;
        parse_state(p, data, &s);
        break;
    case XBOX_REPORT_GUIDE:
        if (len < 1)
            return false;
        p->guide_report = true;
        s.buttons = (s.buttons & ~GAMECONTROLLER_BTN_MODE) |
                    ((data[0] & XBOX_GUIDE_BIT) ? GAMECONTROLLER_BTN_MODE : 0);
        break;
    case XBOX_REPORT_BATTERY:
        if (len < 1)
            return false;
        parse_battery(data[0], &s);
        break;
    default:
        return false;
    }

    s.seq++;
    *state = s;
    return true;
}

static uint8_t magnitude(uint8_t v)
{
    return (uint8_t)((v * XBOX_RUMBLE_MAX + 127u) / 255u);
}

size_t xbox_build_rumble(uint8_t strong, uint8_t weak, uint8_t *out)
{
    memset(out, 0, XBOX_RUMBLE_LEN);
    out[0] = XBOX_REPORT_RUMBLE;
    out[XBOX_RUMBLE_ENABLE] = XBOX_RUMBLE_ENABLE_VAL;
    out[XBOX_RUMBLE_STRONG] = magnitude(strong);
    out[XBOX_RUMBLE_WEAK] = magnitude(weak);
    out[XBOX_RUMBLE_DURATION] = 0xFF;
    out[XBOX_RUMBLE_LOOP] = 0xFF;
    return XBOX_RUMBLE_LEN;
}

#endif /* CONFIG_GAMECONTROLLER_XBOX */
