# Installation

[Back to README](../README.md)

Paths mentioned in this guide are relative to the repository root.

The repository is `mcu-gamecontroller`; the forge package key, C API and
configuration options are `gamecontroller`. It replaces
`esp-ps-controller` (forge key `ps_controller`): a project that
`REQUIRES ps_controller` now `REQUIRES gamecontroller`; `ps_controller.h`
and `ps_reports.h` are unchanged.

## With forge

stm-forge must be at `ee921c3` or later; older ones only compile
`src/*.c`, so the parsers in `src/<pad>/` are never built.

The catalog line in `profiles/pkgversion` is the same for both forges:

```make
gamecontroller_GIT=https://github.com/JonasAtelier/mcu-gamecontroller@stable
```

and a target turns the library and its pads on in
`configs/<target>_defconfig`:

```make
PKG_gamecontroller     = y
GAMECONTROLLER_DS4     = y
GAMECONTROLLER_DS5     = y
# GAMECONTROLLER_XBOX is not set
# GAMECONTROLLER_DEBUG is not set
```

esp-forge turns each line into `CONFIG_GAMECONTROLLER_<PAD>=y`; stm-forge
passes `-DCONFIG_GAMECONTROLLER_<PAD>=1`. On esp-forge the Bluetooth
options below go into the target's sdkconfig as well.

## ESP-IDF

```sh
git clone https://github.com/JonasAtelier/mcu-gamecontroller components/gamecontroller
```

```cmake
idf_component_register(SRCS "main.c" REQUIRES gamecontroller nvs_flash)
```

The component always requires `bt` and `esp_hid`, as esp-ps-controller
did; without Bluetooth on they build to little.

### Bluetooth options

A component cannot switch Bluetooth on; the app's sdkconfig must. Which
options depend on the pads and the chip:

| Pads | Chip | sdkconfig |
|---|---|---|
| DS4 / DS5 | ESP32 | `BT_ENABLED`, `BTDM_CTRL_MODE_BR_EDR_ONLY`, `BT_BLUEDROID_ENABLED`, `BT_CLASSIC_ENABLED`, `BT_HID_ENABLED`, `BT_HID_HOST_ENABLED` |
| Xbox | any ESP with BLE | `BT_ENABLED`, `BT_BLUEDROID_ENABLED`, `BT_BLE_ENABLED`, `BT_BLE_42_FEATURES_SUPPORTED` |
| both | ESP32 | all of the above, with `BTDM_CTRL_MODE_BTDM` instead |

`GAMECONTROLLER_DS4` and `GAMECONTROLLER_DS5` depend on
`SOC_BT_CLASSIC_SUPPORTED`, so menuconfig offers them on the ESP32 only;
`GAMECONTROLLER_XBOX` depends on `SOC_BLE_SUPPORTED`. A pad enabled without
its Bluetooth options builds its report parser alone, and the CMakeLists
warns which options are missing. `examples/esp/*/sdkconfig.defaults` are
working sets.

Call `nvs_flash_init()` before `gamecontroller_bt_start()`: Bluedroid keeps
pairing and bonding keys in NVS.

## STM32 HAL

Copy `include/` and `src/` into an STM32 project, compile every `.c` under
`src/`, add `include/` and `src/` to the include path, and define
`CMSIS_device_header`, `USE_HAL_DRIVER` and `CONFIG_GAMECONTROLLER_<PAD>=1`
for each pad whose parser you want.

**There is no Bluetooth transport on the HAL.** What builds is the core,
the bus layer, and the pads' report parsers and output builders (`ds4.h`,
`ds5.h`, `xbox.h`): plain C for an app that gets the pad's HID reports some
other way. `src/gamecontroller_bt.c` and the `ps_controller_*` calls are
ESP-IDF only and compile to nothing here. With the library on and no pad
picked, `src/gamecontroller_port_stm.c` emits a `#warning`.

**printf() is the log**, as in every category library: retarget newlib's
`_write()` to a UART.
