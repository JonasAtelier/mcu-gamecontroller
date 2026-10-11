CC ?= cc
# gnu11: the gamecontroller_* log macros use `, ##__VA_ARGS__`.
CFLAGS += -std=gnu11 -Wall -Wextra -Werror -pedantic
# Signed overflow usually wraps anyway, so a test of wrapping arithmetic
# passes on the broken code too. UBSan makes it abort instead.
CFLAGS += -fsanitize=undefined -fno-sanitize-recover=all
# src/ for the common private header, and every chip folder, because the
# tests include a chip's private headers by plain name.
CPPFLAGS += -I../include -I../src $(addprefix -I,$(wildcard ../src/*/))

# Two suites, one per platform: the core through src/gamecontroller_port_esp.c
# against idf_stubs/ (esp/), and through src/gamecontroller_port_stm.c
# against hal_stubs/ (stm/); on ESP-IDF also the Bluetooth glue, against
# fakes of the stack (esp/test_bt). The pads' parsers and builders touch no
# bus or platform API: test_ds4, test_ds5 and test_xbox run them once, built
# as stm-forge builds them. test_reports.c is esp-ps-controller's own test,
# unchanged.

PADS := -DCONFIG_GAMECONTROLLER_DS4=1 -DCONFIG_GAMECONTROLLER_DS5=1 \
	-DCONFIG_GAMECONTROLLER_XBOX=1
# What ESP-IDF passes: ESP_PLATFORM, and every pad on. Bluetooth is off, as
# in a build without it, except for test_bt below.
ESP_CPPFLAGS := -Iidf_stubs -DESP_PLATFORM $(PADS)
# What stm-forge passes: the family header the code includes, the HAL modules
# an app's hal_conf would enable, and every pad option on.
STM_CPPFLAGS := -Ihal_stubs \
		-DCMSIS_device_header='"stm32_hal_stub.h"' \
		-DHAL_I2C_MODULE_ENABLED -DHAL_SPI_MODULE_ENABLED \
		-DHAL_UART_MODULE_ENABLED \
		$(PADS) -DCONFIG_GAMECONTROLLER_DEBUG=1
# The glue's Bluetooth, as an ESP32 app with both transports sets it.
BT_CPPFLAGS := -DCONFIG_BT_BLUEDROID_ENABLED=1 -DCONFIG_BT_CLASSIC_ENABLED=1 \
	       -DCONFIG_BT_HID_HOST_ENABLED=1 -DCONFIG_BT_BLE_ENABLED=1 \
	       -DCONFIG_GAMECONTROLLER_MAX_PADS=2

# The core and the stubs every binary of a suite links.
CORE     := ../src/gamecontroller.c ../src/gamecontroller_log.c
ESP_CORE := $(CORE) ../src/gamecontroller_port_esp.c idf_stubs/idf_stubs.c
STM_CORE := $(CORE) ../src/gamecontroller_port_stm.c hal_stubs/hal_stubs.c

PARSERS := ../src/ds4/ds4.c ../src/ds5/ds5.c ../src/xbox/xbox.c

PAD_TESTS  := test_math test_ds4 test_ds5 test_xbox
ESP_TESTS  := $(addprefix esp/,test_core test_bt test_port_esp)
STM_TESTS  := $(addprefix stm/,test_core test_port_stm)
TESTS      := test_reports $(PAD_TESTS) $(ESP_TESTS) $(STM_TESTS)

.PHONY: all clean test

all: $(TESTS)

test_reports: test_reports.c ../src/ps_reports.c
test_math: test_math.c
test_ds4: test_ds4.c $(PARSERS) $(STM_CORE)
test_ds5: test_ds5.c $(PARSERS) $(STM_CORE)
test_xbox: test_xbox.c $(PARSERS) $(STM_CORE)

esp/test_core: esp/test_core.c $(ESP_CORE)
esp/test_bt: esp/test_bt.c ../src/gamecontroller_bt.c ../src/ps_reports.c \
	$(PARSERS) $(ESP_CORE)
esp/test_port_esp: esp/test_port_esp.c $(ESP_CORE)

stm/test_core: stm/test_core.c $(STM_CORE)
stm/test_port_stm: stm/test_port_stm.c $(STM_CORE)

$(PAD_TESTS): CPPFLAGS += $(STM_CPPFLAGS)
$(ESP_TESTS): CPPFLAGS += $(ESP_CPPFLAGS)
$(STM_TESTS): CPPFLAGS += $(STM_CPPFLAGS)

esp/test_bt: CPPFLAGS += $(BT_CPPFLAGS)
# Every printf() in a binary that links the HAL stubs also lands in
# stub_log_last, so a test can check what was logged.
$(STM_TESTS) $(PAD_TESTS): LDFLAGS += -Wl,--wrap=printf

$(TESTS):
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c,$^) -o $@ -lm

test: $(TESTS)
	@set -e; for t in $(TESTS); do echo "./$$t"; ./$$t; done

clean:
	rm -f $(TESTS)

# Headers are prerequisites so that editing one rebuilds. Without this, `make
# test` reruns a stale binary and reports green for code it never compiled.
# Recipes use $(filter %.c,$^) so the headers never reach the compiler.
HDRS  := $(wildcard ../include/*.h ../src/*.h ../src/*/*.h)
STUBS := $(wildcard idf_stubs/*.h idf_stubs/*/*.h hal_stubs/*.h)
VECTORS := vectors.h
$(TESTS): $(HDRS) $(STUBS) $(VECTORS) $(wildcard *_cases.h)
