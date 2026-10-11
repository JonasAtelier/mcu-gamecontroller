// Pure report parsing — no ESP dependencies, so it builds and tests on the host.
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int8_t  lx, ly, rx, ry;   // -128..127, +y = down, +x = right
    uint8_t l2, r2;           // 0..255 analog triggers
    bool up, down, left, right;
    bool cross, circle, square, triangle;
    bool l1, r1, l3, r3;
    bool share, options, ps, touchpad;
    bool mute;                // PS5 only
} ps_state_t;

// Fills *out from one HID input report. Returns false (and leaves *out alone)
// if the report id / length is not one we understand.
bool ps_parse(uint8_t report_id, const uint8_t *data, size_t len, ps_state_t *out);
