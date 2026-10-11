#pragma once
#include "esp_err.h"
#include "ps_reports.h"

// Brings up Bluetooth Classic, scans for a PS4/PS5 pad in pairing mode
// (hold SHARE/CREATE + PS until the light bar flashes) and connects to it.
// Blocks until connected or the timeout expires.
esp_err_t ps_controller_start(uint32_t scan_seconds);

// Same, but skips the scan and connects straight to "aa:bb:cc:dd:ee:ff".
esp_err_t ps_controller_start_mac(const char *mac);

bool ps_controller_connected(void);

// Latest button/stick state. Safe to call from any task.
void ps_controller_get(ps_state_t *out);

// mcu-gamecontroller: the first PlayStation pad as a struct gamecontroller_dev
// -- rumble, light bar, player LEDs -- or NULL while none is connected.
#include "gamecontroller.h"
struct gamecontroller_dev *ps_controller_dev(void);
