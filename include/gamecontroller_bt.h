/*
 * gamecontroller_bt -- pads over Bluetooth, through ESP-IDF's HID host.
 * ESP-IDF only: the STM32 HAL has no Bluetooth transport here.
 *
 * DualShock 4 and DualSense are Bluetooth Classic pads, so only on the
 * ESP32 (SOC_BT_CLASSIC_SUPPORTED); the Xbox Wireless Controller, on its
 * BLE firmware, is a BLE HID pad, on any ESP with BLE. The Xbox path is
 * unverified on hardware.
 *
 * Needs, in the app's sdkconfig: CONFIG_BT_ENABLED and
 * CONFIG_BT_BLUEDROID_ENABLED; for DS4/DS5 also CONFIG_BT_CLASSIC_ENABLED,
 * CONFIG_BT_HID_ENABLED and CONFIG_BT_HID_HOST_ENABLED; for Xbox
 * CONFIG_BT_BLE_ENABLED; for both on one ESP32 the controller in dual mode,
 * CONFIG_BTDM_CTRL_MODE_BTDM. examples/esp/ carries working
 * sdkconfig.defaults. Bluedroid keeps pairing keys in NVS: nvs_flash_init()
 * first.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "gamecontroller.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t scan_seconds;  /* how long to look; 0 means 20 */
    bool     playstation;   /* DS4/DS5 over Bluetooth Classic; ESP32 only */
    bool     xbox;          /* Xbox over BLE */
} gamecontroller_bt_config_t;

/*
 * Bring Bluetooth up (once; later calls reuse it), look for pads in pairing
 * mode -- PlayStation: hold SHARE/CREATE + PS until the light bar flashes;
 * Xbox: its pairing mode, per its manual -- and connect to the first one
 * found. Blocks until a pad is connected or the time is up:
 * ESP_ERR_TIMEOUT. ESP_ERR_NOT_SUPPORTED for a kind this build or chip has
 * no transport for.
 */
esp_err_t gamecontroller_bt_start(const gamecontroller_bt_config_t *cfg);

/* The pad in slot index (0 .. CONFIG_GAMECONTROLLER_MAX_PADS - 1), or NULL
 * if that slot holds none. Valid while the pad stays connected. */
struct gamecontroller_dev *gamecontroller_bt_pad(unsigned int index);

/* How many pads are connected now. */
unsigned int gamecontroller_bt_count(void);

#ifdef __cplusplus
}
#endif
