/*
 * Kernel dev_err()-style logging for game controller drivers and their
 * users.
 *
 * ESP-IDF: everything goes out under the one ESP-IDF tag "gamecontroller",
 * prefixed with the pad's name, so one esp_log_level_set("gamecontroller",
 * ...) controls every pad and a line still says which pad it came from.
 * There is deliberately no level machinery of our own; gamecontroller_dbg()
 * is also subject to CONFIG_LOG_MAXIMUM_LEVEL, and CONFIG_GAMECONTROLLER_DEBUG
 * is not used.
 *
 * STM32 HAL: every line goes out through printf() as
 * "gamecontroller: <name>: ...". Where printf() ends up is the app's
 * business: with newlib, retarget _write() to a UART. gamecontroller_dbg()
 * alone is compiled out unless CONFIG_GAMECONTROLLER_DEBUG is 1 (stm-forge:
 * GAMECONTROLLER_DEBUG = y).
 */
#ifndef GAMECONTROLLER_LOG_H
#define GAMECONTROLLER_LOG_H

#ifdef ESP_PLATFORM
#include "esp_log.h"
#else
#include <stdio.h>
#endif

#include "gamecontroller.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ESP_PLATFORM
#define GAMECONTROLLER_LOG_TAG	"gamecontroller"

#define gamecontroller_err(gamecontroller, fmt, ...) \
	ESP_LOGE(GAMECONTROLLER_LOG_TAG, "%s: " fmt, (gamecontroller)->name, ##__VA_ARGS__)
#define gamecontroller_warn(gamecontroller, fmt, ...) \
	ESP_LOGW(GAMECONTROLLER_LOG_TAG, "%s: " fmt, (gamecontroller)->name, ##__VA_ARGS__)
#define gamecontroller_info(gamecontroller, fmt, ...) \
	ESP_LOGI(GAMECONTROLLER_LOG_TAG, "%s: " fmt, (gamecontroller)->name, ##__VA_ARGS__)
#define gamecontroller_dbg(gamecontroller, fmt, ...) \
	ESP_LOGD(GAMECONTROLLER_LOG_TAG, "%s: " fmt, (gamecontroller)->name, ##__VA_ARGS__)

#else /* STM32 HAL */
#define gamecontroller_err(gamecontroller, fmt, ...) \
	printf("gamecontroller: %s: " fmt "\n", (gamecontroller)->name, ##__VA_ARGS__)
#define gamecontroller_warn(gamecontroller, fmt, ...) \
	printf("gamecontroller: %s: " fmt "\n", (gamecontroller)->name, ##__VA_ARGS__)
#define gamecontroller_info(gamecontroller, fmt, ...) \
	printf("gamecontroller: %s: " fmt "\n", (gamecontroller)->name, ##__VA_ARGS__)

#if CONFIG_GAMECONTROLLER_DEBUG
#define gamecontroller_dbg(gamecontroller, fmt, ...) \
	printf("gamecontroller: %s: " fmt "\n", (gamecontroller)->name, ##__VA_ARGS__)
#else
/* Still type-checked, so a debug line cannot rot while it is off. */
#define gamecontroller_dbg(gamecontroller, fmt, ...) do { \
	if (0) \
		printf("gamecontroller: %s: " fmt "\n", (gamecontroller)->name, \
		       ##__VA_ARGS__); \
} while (0)
#endif
#endif /* ESP_PLATFORM */

/* One info line: sticks, triggers, buttons, battery. */
void gamecontroller_log_state(const struct gamecontroller_dev *gc,
			      const struct gamecontroller_state *s);

#ifdef __cplusplus
}
#endif

#endif /* GAMECONTROLLER_LOG_H */
