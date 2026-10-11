# Hardware

[Back to README](../README.md)

Paths mentioned in this guide are relative to the repository root.

There is nothing to wire: the pads are on Bluetooth, and the radio is the
ESP's own.

## Which chip for which pad

| Pad | Bluetooth | ESP32 | ESP32-S3, C3, C6, ... | STM32 |
|---|---|---|---|---|
| DualShock 4 | Classic (BR/EDR) | yes | no | parsers only |
| DualSense | Classic (BR/EDR) | yes | no | parsers only |
| Xbox Wireless Controller, BLE firmware | LE | yes | yes | parsers only |

Bluetooth Classic is on the ESP32 alone: of the chips in ESP-IDF v5.5.5,
only `soc/esp32/include/soc/soc_caps.h` sets `SOC_BT_CLASSIC_SUPPORTED`;
`SOC_BLE_SUPPORTED` is set for the ESP32, S3, C2, C3, C5, C6, C61 and H2.
The workspace's esp-forge targets (dingo, flux, nova) are all ESP32-S3, so
on them only the Xbox pad has a transport.

The Xbox pads are the ones Linux knows on Bluetooth LE firmware
(`hid-ids.h:947-951`, v6.12): model 1708 (product 0x0b20), 1797 (0x0b22)
and 1914, the Series pad (0x0b13). A 1708 on its older Bluetooth Classic
firmware is not supported.

## Pairing

- **DualShock 4, DualSense:** hold SHARE (CREATE on the DualSense) and PS
  until the light bar flashes (esp-ps-controller's instructions), then call
  `gamecontroller_bt_start()` with `.playstation = true`. The pad pairs
  with "Just Works", no PIN.
- **Xbox:** put the pad in pairing mode, as its manual says, then call
  `gamecontroller_bt_start()` with `.xbox = true`. The library bonds with
  "Just Works" too, and accepts the pad's security request. Unverified with
  a pad.

Bluedroid stores the keys in NVS: call `nvs_flash_init()` first.

## The bus layer

Every category library carries the same I2C, SPI and UART bus layer
(`src/gamecontroller_priv.h`, `src/gamecontroller_port_*.c`) and its port
tests. No pad here uses it: they are on Bluetooth, through ESP-IDF's HID
host. It is there so the library has the shape of the others, and for a
future pad or transport that needs it.
