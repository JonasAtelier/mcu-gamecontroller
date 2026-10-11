#ifndef ESP_HIDH_H
#define ESP_HIDH_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_gap_bt_api.h"
#include "esp_hid_common.h"

/* Host stub for the HID host, the subset used; names and shapes as IDF
 * v5.5.5 esp_hidh.h and esp_hidh_bluedroid.h. */
typedef struct esp_hidh_dev_s esp_hidh_dev_t;

typedef enum {
	ESP_HIDH_ANY_EVENT = ESP_EVENT_ANY_ID,
	ESP_HIDH_OPEN_EVENT = 0,
	ESP_HIDH_BATTERY_EVENT,
	ESP_HIDH_INPUT_EVENT,
	ESP_HIDH_FEATURE_EVENT,
	ESP_HIDH_CLOSE_EVENT,
} esp_hidh_event_t;

typedef union {
	struct {
		esp_hidh_dev_t *dev;
		esp_err_t status;
	} open;
	struct {
		esp_hidh_dev_t *dev;
		int reason;
		esp_err_t status;
	} close;
	struct {
		esp_hidh_dev_t *dev;
		esp_hid_usage_t usage;
		uint16_t report_id;
		uint16_t length;
		uint8_t *data;
		uint8_t map_index;
	} input;
} esp_hidh_event_data_t;

typedef struct {
	esp_event_handler_t callback;
	uint16_t event_stack_size;
	void *callback_arg;
} esp_hidh_config_t;

esp_err_t esp_hidh_init(const esp_hidh_config_t *config);
esp_hidh_dev_t *esp_hidh_dev_open(esp_bd_addr_t bda,
				  esp_hid_transport_t transport,
				  uint8_t remote_addr_type);
esp_err_t esp_hidh_dev_close(esp_hidh_dev_t *dev);
esp_err_t esp_hidh_dev_output_set(esp_hidh_dev_t *dev, size_t map_index,
				  size_t report_id, uint8_t *data,
				  size_t length);
esp_err_t esp_hidh_dev_feature_get(esp_hidh_dev_t *dev, size_t map_index,
				   size_t report_id, size_t max_len,
				   uint8_t *data, size_t *length);
const char *esp_hidh_dev_name_get(esp_hidh_dev_t *dev);
uint16_t esp_hidh_dev_vendor_id_get(esp_hidh_dev_t *dev);
uint16_t esp_hidh_dev_product_id_get(esp_hidh_dev_t *dev);
esp_hid_transport_t esp_hidh_dev_transport_get(esp_hidh_dev_t *dev);

#endif
