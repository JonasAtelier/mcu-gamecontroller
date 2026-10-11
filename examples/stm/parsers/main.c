/*
 * The STM32 HAL has no Bluetooth transport in this library: what it has is
 * the pads' report parsers and output builders, plain C. If the app gets a
 * pad's HID reports some other way -- its own transport -- they turn them
 * into the same struct gamecontroller_state as on ESP-IDF.
 *
 * A snippet, not a project. Needs GAMECONTROLLER_DS4 = y.
 */
#include <stdio.h>

#include CMSIS_device_header

#include "ds4.h"
#include "gamecontroller.h"

static struct gamecontroller_state pad = {
    .battery = GAMECONTROLLER_BATTERY_UNKNOWN,
};

/* One DualShock 4 Bluetooth input report: id, then the rest. */
void on_ds4_report(uint8_t report_id, const uint8_t *data, size_t len)
{
    if (!ds4_parse(report_id, data, len, &pad))
        return;                 /* another report, or a bad CRC */
    if (gamecontroller_pressed(&pad, GAMECONTROLLER_BTN_SOUTH))
        printf("Cross, left stick %d %d\n", pad.lx, pad.ly);
}

/* Rumble and a blue light bar, as the 78-byte report the pad takes. */
size_t ds4_rumble_report(uint8_t *out)
{
    const struct ds4_output o = { .strong = 128, .weak = 64, .blue = 255 };

    return ds4_build_output(&o, out);
}
