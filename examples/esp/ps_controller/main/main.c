#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ps_controller.h"

static const char *TAG = "example";

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());   // Bluedroid stores pairing keys in NVS

    ESP_LOGI(TAG, "put the pad in pairing mode (SHARE/CREATE + PS)");
    if (ps_controller_start(20) != ESP_OK) {
        ESP_LOGE(TAG, "no controller found");
        return;
    }

    ps_state_t s;
    while (1) {
        ps_controller_get(&s);
        ESP_LOGI(TAG, "L(%4d,%4d) R(%4d,%4d) L2=%3u R2=%3u %s%s%s%s",
                 s.lx, s.ly, s.rx, s.ry, s.l2, s.r2,
                 s.cross ? "X " : "", s.circle ? "O " : "",
                 s.square ? "[] " : "", s.triangle ? "/\\ " : "");
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
