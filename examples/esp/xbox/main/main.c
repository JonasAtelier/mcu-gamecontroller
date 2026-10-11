/*
 * An Xbox Wireless Controller on an ESP32-S3, over BLE: scan, then print
 * its state five times a second and rumble while A is held. Unverified on
 * a pad.
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
    gamecontroller_bt_config_t cfg = { .scan_seconds = 30, .xbox = true };
    bool rumbling = false;

    ESP_ERROR_CHECK(nvs_flash_init());   /* Bluedroid stores bonding keys in NVS */

    ESP_LOGI(TAG, "put the pad in pairing mode");
    if (gamecontroller_bt_start(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "no pad found");
        return;
    }

    for (;;) {
        struct gamecontroller_dev *pad = gamecontroller_bt_pad(0);
        struct gamecontroller_state s;
        bool a;

        if (pad && gamecontroller_get_state(pad, &s) == 0) {
            gamecontroller_log_state(pad, &s);
            a = gamecontroller_pressed(&s, GAMECONTROLLER_BTN_SOUTH);
            if (a != rumbling) {
                gamecontroller_set_rumble(pad, a ? 255 : 0, a ? 128 : 0);
                rumbling = a;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
