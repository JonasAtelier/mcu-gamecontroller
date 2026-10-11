#ifndef ESP_EVENT_H
#define ESP_EVENT_H

#include <stdint.h>

/* Host stub: only the types the HID host's callback uses. */
typedef const char *esp_event_base_t;
typedef void (*esp_event_handler_t)(void *arg, esp_event_base_t base,
				    int32_t id, void *data);
#define ESP_EVENT_ANY_ID	-1

#endif
