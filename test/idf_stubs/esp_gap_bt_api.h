#ifndef ESP_GAP_BT_API_H
#define ESP_GAP_BT_API_H

#include <stdint.h>

#include "esp_err.h"

/* Host stub for Classic GAP, the subset used; names as IDF v5.5.5
 * esp_gap_bt_api.h and esp_bt_defs.h. EIR types and lengths are the real
 * values; the event and enum numbers are the stub's own. */
typedef uint8_t esp_bd_addr_t[6];

#define ESP_BT_GAP_EIR_DATA_LEN	240

typedef enum {
	ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME = 0x08,
	ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME = 0x09,
} esp_bt_eir_type_t;

typedef enum {
	ESP_BT_GAP_DISC_RES_EVT = 0,
	ESP_BT_GAP_DISC_STATE_CHANGED_EVT,
} esp_bt_gap_cb_event_t;

typedef enum {
	ESP_BT_GAP_DEV_PROP_BDNAME = 1,
	ESP_BT_GAP_DEV_PROP_COD,
	ESP_BT_GAP_DEV_PROP_RSSI,
	ESP_BT_GAP_DEV_PROP_EIR,
} esp_bt_gap_dev_prop_type_t;

typedef struct {
	esp_bt_gap_dev_prop_type_t type;
	int len;
	void *val;
} esp_bt_gap_dev_prop_t;

typedef union {
	struct disc_res_param {
		esp_bd_addr_t bda;
		int num_prop;
		esp_bt_gap_dev_prop_t *prop;
	} disc_res;
} esp_bt_gap_cb_param_t;

typedef void (*esp_bt_gap_cb_t)(esp_bt_gap_cb_event_t event,
				esp_bt_gap_cb_param_t *param);

typedef uint8_t esp_bt_io_cap_t;
#define ESP_BT_IO_CAP_NONE	3

typedef enum {
	ESP_BT_SP_IOCAP_MODE = 0,
} esp_bt_sp_param_t;

typedef enum {
	ESP_BT_NON_CONNECTABLE,
	ESP_BT_CONNECTABLE,
} esp_bt_connection_mode_t;

typedef enum {
	ESP_BT_NON_DISCOVERABLE,
	ESP_BT_LIMITED_DISCOVERABLE,
	ESP_BT_GENERAL_DISCOVERABLE,
} esp_bt_discovery_mode_t;

typedef enum {
	ESP_BT_INQ_MODE_GENERAL_INQUIRY,
	ESP_BT_INQ_MODE_LIMITED_INQUIRY,
} esp_bt_inq_mode_t;

uint8_t *esp_bt_gap_resolve_eir_data(uint8_t *eir, esp_bt_eir_type_t type,
				     uint8_t *length);
esp_err_t esp_bt_gap_start_discovery(esp_bt_inq_mode_t mode,
				     uint8_t inq_len, uint8_t num_rsps);
esp_err_t esp_bt_gap_cancel_discovery(void);
esp_err_t esp_bt_gap_set_security_param(esp_bt_sp_param_t param_type,
					void *value, uint8_t len);
esp_err_t esp_bt_gap_set_scan_mode(esp_bt_connection_mode_t c_mode,
				   esp_bt_discovery_mode_t d_mode);
esp_err_t esp_bt_gap_register_callback(esp_bt_gap_cb_t callback);

#endif
