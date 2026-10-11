/*
 * Any pad this build knows -- DualShock 4, DualSense, Xbox -- on an ESP32:
 * scan for one, print its state five times a second, rumble while SOUTH
 * (Cross, A) is held and turn the light bar red while EAST (Circle, B) is.
 */
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "gamecontroller.h"
#include "gamecontroller_bt.h"
#include "gamecontroller_log.h"

static const char *TAG = "example";

void app_main(void)
{
    gamecontroller_bt_config_t cfg = {
        .scan_seconds = 30,
        .playstation = true,
        .xbox = true,
    };
    bool rumbling = false, red = false;

    ESP_ERROR_CHECK(nvs_flash_init());   /* Bluedroid stores pairing keys in NVS */

    ESP_LOGI(TAG, "put a pad in pairing mode");
    if (gamecontroller_bt_start(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "no pad found");
        return;
    }

    for (;;) {
        struct gamecontroller_dev *pad = gamecontroller_bt_pad(0);
        struct gamecontroller_state s;
        bool south, east;

        if (!pad || gamecontroller_get_state(pad, &s)) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        gamecontroller_log_state(pad, &s);

        south = gamecontroller_pressed(&s, GAMECONTROLLER_BTN_SOUTH);
        if (south != rumbling) {
            gamecontroller_set_rumble(pad, south ? 200 : 0, south ? 100 : 0);
            rumbling = south;
        }
        east = gamecontroller_pressed(&s, GAMECONTROLLER_BTN_EAST);
        if (east != red) {
            /* -EOPNOTSUPP on an Xbox pad: it has no light bar */
            gamecontroller_set_lightbar(pad, east ? 255 : 0, 0, east ? 0 : 64);
            red = east;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
