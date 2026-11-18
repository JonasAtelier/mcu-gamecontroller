# mcu-gamecontroller

Read buttons, sticks and triggers from a PlayStation or Xbox controller. An original ESP32 can pair with a DualShock 4 or DualSense over Bluetooth Classic. BLE-capable ESP32 chips supported by the driver can use Xbox models 1708, 1797 and 1914 running BLE firmware.

**Platforms** · ESP-IDF 5.2+ and STM32 HAL. Hardware testing is pending. The host tests use simulated controller reports.

## Quick start

This example uses an original ESP32. The STM32 port provides report parsers and output builders and requires your own transport. The Xbox driver is based on documented reports and awaits testing with a controller.

In a terminal with ESP-IDF activated, get the example and select the target for an original ESP32.

```sh
git clone https://github.com/JonasAtelier/mcu-gamecontroller.git gamecontroller
cd gamecontroller/examples/esp/gamecontroller
idf.py set-target esp32
```

See the [pairing guide](docs/hardware.md) for the button combination for your controller. Connect the board by USB, then build, upload and open the serial monitor.

```sh
idf.py build flash monitor
```

When prompted, put the controller into pairing mode. Once connected, move a stick to see its state change in the monitor. Hold Cross or A to test rumble.

## Using the library in your project

- Add the component and enable the driver for your hardware. [Installation and build options](docs/installation.md)
- Start from an example for your chip. The STM32 examples fit into a project with board setup already in place. [ESP-IDF examples](examples/esp/) · [STM32 examples](examples/stm/)
- Look up calls and chip-specific settings. [API usage](docs/usage.md) · [Hardware reference](docs/chips.md)

To run the simulated tests on your computer, run `make -C test test` from the library root. [Test requirements and coverage](docs/development.md)

## License

[MIT](LICENSE)

<p align="center">
  <img src="assets/module.jpg"
       alt="A PlayStation DualShock 4 controller"
       width="520">
  <br><sub>DualShock 4 &mdash; photo by Evan-Amos, public domain, via <a href="https://commons.wikimedia.org/wiki/File:DualShock_4.jpg">Wikimedia Commons</a></sub>
</p>
