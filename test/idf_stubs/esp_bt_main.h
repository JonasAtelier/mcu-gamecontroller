#ifndef ESP_BT_MAIN_H
#define ESP_BT_MAIN_H

#include "esp_err.h"

/* Host stub for Bluedroid's start-up, as IDF v5.5.5 esp_bt_main.h. */
typedef enum {
	ESP_BLUEDROID_STATUS_UNINITIALIZED = 0,
	ESP_BLUEDROID_STATUS_INITIALIZED,
	ESP_BLUEDROID_STATUS_ENABLED,
} esp_bluedroid_status_t;

esp_bluedroid_status_t esp_bluedroid_get_status(void);
esp_err_t esp_bluedroid_init(void);
esp_err_t esp_bluedroid_enable(void);

#endif
