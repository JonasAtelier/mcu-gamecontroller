#ifndef ESP_HID_COMMON_H
#define ESP_HID_COMMON_H

/* Host stub, the subset used; as IDF v5.5.5 esp_hid_common.h. */
typedef enum {
	ESP_HID_TRANSPORT_BT,
	ESP_HID_TRANSPORT_BLE,
	ESP_HID_TRANSPORT_USB,
	ESP_HID_TRANSPORT_MAX,
} esp_hid_transport_t;

typedef enum {
	ESP_HID_USAGE_GENERIC = 0,
	ESP_HID_USAGE_GAMEPAD = 8,
} esp_hid_usage_t;

#endif
