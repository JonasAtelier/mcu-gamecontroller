#ifndef ESP_BT_H
#define ESP_BT_H

#include "esp_err.h"

/* Host stub for the Bluetooth controller API, the subset used; names as
 * IDF v5.5.5 components/bt/include/esp32/include/esp_bt.h. */
typedef enum {
	ESP_BT_MODE_IDLE = 0x00,
	ESP_BT_MODE_BLE = 0x01,
	ESP_BT_MODE_CLASSIC_BT = 0x02,
	ESP_BT_MODE_BTDM = 0x03,
} esp_bt_mode_t;

typedef enum {
	ESP_BT_CONTROLLER_STATUS_IDLE = 0,
	ESP_BT_CONTROLLER_STATUS_INITED,
	ESP_BT_CONTROLLER_STATUS_ENABLED,
	ESP_BT_CONTROLLER_STATUS_NUM,
} esp_bt_controller_status_t;

typedef struct {
	uint8_t mode;
} esp_bt_controller_config_t;

#define BT_CONTROLLER_INIT_CONFIG_DEFAULT() { .mode = ESP_BT_MODE_BTDM }

esp_err_t esp_bt_controller_mem_release(esp_bt_mode_t mode);
esp_err_t esp_bt_controller_init(esp_bt_controller_config_t *cfg);
esp_err_t esp_bt_controller_enable(esp_bt_mode_t mode);
esp_bt_controller_status_t esp_bt_controller_get_status(void);

#endif
