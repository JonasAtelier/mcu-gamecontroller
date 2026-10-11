#ifndef ESP_GAP_BLE_API_H
#define ESP_GAP_BLE_API_H

#include <stdint.h>

#include "esp_err.h"
#include "esp_gap_bt_api.h"

/* Host stub for BLE GAP, the subset used; names as IDF v5.5.5
 * esp_gap_ble_api.h and esp_bt_defs.h. AD types, appearance, auth, IO and
 * key-mask values are the real ones; event and enum numbers the stub's. */
#define ESP_BLE_APPEARANCE_HID_GAMEPAD	0x03C4
#define ESP_BLE_ADV_DATA_LEN_MAX	31
#define ESP_BLE_SCAN_RSP_DATA_LEN_MAX	31

typedef enum {
	ESP_BLE_AD_TYPE_APPEARANCE = 0x19,
} esp_ble_adv_data_type;

typedef enum {
	BLE_SCAN_TYPE_PASSIVE = 0x0,
	BLE_SCAN_TYPE_ACTIVE = 0x1,
} esp_ble_scan_type_t;

typedef enum {
	BLE_ADDR_TYPE_PUBLIC = 0x00,
	BLE_ADDR_TYPE_RANDOM = 0x01,
} esp_ble_addr_type_t;

typedef enum {
	BLE_SCAN_FILTER_ALLOW_ALL = 0x0,
} esp_ble_scan_filter_t;

typedef enum {
	BLE_SCAN_DUPLICATE_DISABLE = 0x0,
	BLE_SCAN_DUPLICATE_ENABLE = 0x1,
} esp_ble_scan_duplicate_t;

typedef struct {
	esp_ble_scan_type_t scan_type;
	esp_ble_addr_type_t own_addr_type;
	esp_ble_scan_filter_t scan_filter_policy;
	uint16_t scan_interval;
	uint16_t scan_window;
	esp_ble_scan_duplicate_t scan_duplicate;
} esp_ble_scan_params_t;

typedef enum {
	ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT = 2,
	ESP_GAP_BLE_SCAN_RESULT_EVT = 3,
	ESP_GAP_BLE_SEC_REQ_EVT = 10,
} esp_gap_ble_cb_event_t;

typedef enum {
	ESP_GAP_SEARCH_INQ_RES_EVT = 0,
	ESP_GAP_SEARCH_INQ_CMPL_EVT = 1,
} esp_gap_search_evt_t;

typedef union {
	struct ble_scan_result_evt_param {
		esp_gap_search_evt_t search_evt;
		esp_bd_addr_t bda;
		esp_ble_addr_type_t ble_addr_type;
		uint8_t ble_adv[ESP_BLE_ADV_DATA_LEN_MAX +
				ESP_BLE_SCAN_RSP_DATA_LEN_MAX];
		uint8_t adv_data_len;
		uint8_t scan_rsp_len;
	} scan_rst;
	struct ble_sec_req_param {
		struct {
			esp_bd_addr_t bd_addr;
		} ble_req;
	} ble_security;
} esp_ble_gap_cb_param_t;

typedef void (*esp_gap_ble_cb_t)(esp_gap_ble_cb_event_t event,
				 esp_ble_gap_cb_param_t *param);

typedef uint8_t esp_ble_auth_req_t;
typedef uint8_t esp_ble_io_cap_t;
#define ESP_LE_AUTH_BOND	(1 << 0)
#define ESP_IO_CAP_NONE		3
#define ESP_BLE_ENC_KEY_MASK	(1 << 0)
#define ESP_BLE_ID_KEY_MASK	(1 << 1)

typedef enum {
	ESP_BLE_SM_AUTHEN_REQ_MODE = 2,
	ESP_BLE_SM_IOCAP_MODE = 4,
	ESP_BLE_SM_SET_INIT_KEY = 5,
	ESP_BLE_SM_SET_RSP_KEY = 6,
} esp_ble_sm_param_t;

uint8_t *esp_ble_resolve_adv_data_by_type(uint8_t *adv_data,
					  uint16_t adv_data_len,
					  esp_ble_adv_data_type type,
					  uint8_t *length);
esp_err_t esp_ble_gap_set_scan_params(esp_ble_scan_params_t *scan_params);
esp_err_t esp_ble_gap_start_scanning(uint32_t duration);
esp_err_t esp_ble_gap_stop_scanning(void);
esp_err_t esp_ble_gap_security_rsp(esp_bd_addr_t bd_addr, bool accept);
esp_err_t esp_ble_gap_set_security_param(esp_ble_sm_param_t param_type,
					 void *value, uint8_t len);
esp_err_t esp_ble_gap_register_callback(esp_gap_ble_cb_t callback);

#endif
