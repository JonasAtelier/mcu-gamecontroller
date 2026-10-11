/*
 * Host-side check of the core on the STM32 HAL: the cases are
 * ../core_cases.h; this file supplies the log prefix.
 */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "stm32_hal_stub.h"

#include "gamecontroller.h"
#include "gamecontroller_log.h"

/* The HAL's lines are bare printf(), no level letter. */
#define LOG_PREFIX(level)	""

#include "../core_cases.h"

int main(void)
{
	run_core_cases();
	printf("all core checks passed\n");
	return 0;
}
