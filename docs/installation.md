# Installation

[Back to README](../README.md)

Paths mentioned in this guide are relative to the repository root.

The repository is `mcu-gamecontroller`; the forge package key, C API and
configuration options are `gamecontroller`. It replaces
`esp-ps-controller` (forge key `ps_controller`): a project that
`REQUIRES ps_controller` now `REQUIRES gamecontroller`; `ps_controller.h`
and `ps_reports.h` are unchanged.

Three step-by-step guides come first: [ESP-IDF](#esp-idf-step-by-step),
[STM32 with CMake](#with-cmake-a-cubemx-cmake-project) and
[STM32 without CMake](#without-cmake-stm32cubeide-a-makefile). None
needs a forge; if you build with esp-forge or stm-forge, see
[With forge](#with-forge). The two platforms offer different things:

| | ESP-IDF | STM32 HAL |
|---|---|---|
| Bluetooth: scan, pair, connect a pad | yes | no |
| A pad's input reports into `struct gamecontroller_state` | yes, from Bluetooth | yes, from reports your own transport hands over |
| Rumble and light reports | sent to the pad | built, for your transport to send |

The reference follows the guides.

## ESP-IDF, step by step

**You need** ESP-IDF 5.2 or newer, with `idf.py` working in your terminal, an
ESP-IDF project (a folder with a `CMakeLists.txt` and a `main/` folder;
`idf.py create-project my-project` makes one), and an original ESP32: the
DualShock 4 and DualSense talk Bluetooth Classic, which no other ESP chip
has. On an ESP32-S3, C3, C6 and the like only the Xbox pad works, over BLE:
start from `examples/esp/xbox` instead.

ESP-IDF calls a library a **component**: a folder with its own
`CMakeLists.txt` that the build finds and compiles. This library is the
component `gamecontroller`.

1. **Get the library.** Pick one way.

   - **Component manager**, the simplest: the first build downloads the
     library into `managed_components/gamecontroller`. Create
     `main/idf_component.yml`:

     ```yaml
     dependencies:
       gamecontroller:
         git: https://github.com/JonasAtelier/mcu-gamecontroller.git
         version: stable
     ```

   - **Clone**, if you want to edit the library or build offline. In the
     project folder:

     ```sh
     git clone https://github.com/JonasAtelier/mcu-gamecontroller components/gamecontroller
     ```

     Keep the folder name `gamecontroller`: ESP-IDF names a component after
     its folder.

   ```
   my-project/
   ├── CMakeLists.txt
   ├── sdkconfig.defaults       step 2
   ├── main/
   │   ├── CMakeLists.txt       step 3
   │   ├── idf_component.yml    step 1, component manager
   │   └── my-project.c         step 4
   └── components/
       └── gamecontroller/      step 1, clone
   ```

2. **Turn Bluetooth and your pads on.** A component cannot switch Bluetooth
   on; your project's settings must. Copy
   [examples/esp/gamecontroller/sdkconfig.defaults](../examples/esp/gamecontroller/sdkconfig.defaults)
   into the project folder: `sdkconfig.defaults` is a text file of settings
   you choose, and that one turns on Bluetooth Classic and BLE together and
   the three pads, `CONFIG_GAMECONTROLLER_DS4`, `_DS5` and `_XBOX`. ESP-IDF
   copies these lines into `sdkconfig`, the project's full settings file,
   only when it creates that file: **if `sdkconfig` already exists, delete
   it** so the lines are read. [Bluetooth options](#bluetooth-options) says
   which lines each pad needs.

3. **Let your code see the library.** A new project's `main/CMakeLists.txt`
   has no `REQUIRES` and already sees every component. If yours has a
   `REQUIRES` list, add `gamecontroller` and `nvs_flash` (where Bluetooth
   keeps its pairing keys) to it:

   ```cmake
   idf_component_register(SRCS "main.c" REQUIRES gamecontroller nvs_flash)
   ```

4. **Write the smallest test.** Copy
   [examples/esp/gamecontroller/main/main.c](../examples/esp/gamecontroller/main/main.c)
   over your main source file. It scans for 30 seconds, connects the first
   pad it knows and prints its state five times a second; holding Cross
   (or A) rumbles. [API usage](usage.md) explains the calls.

5. **Build.**

   ```sh
   idf.py set-target esp32
   idf.py build
   ```

   A good build ends with `Project build complete.` Flash it as any ESP-IDF
   app, open the serial monitor and put the pad in pairing mode (SHARE + PS
   on a DualShock 4, CREATE + PS on a DualSense). Once it connects, a line
   ends in `gamecontroller: ds4: connected: Wireless Controller`, and the
   state lines follow.

**If it fails**

| The build prints | Why | Fix |
|---|---|---|
| `Failed to resolve component 'gamecontroller' required by component 'main': unknown name.` | The clone is not in a folder named `gamecontroller` | Rename it to `components/gamecontroller` |
| `fatal error: gamecontroller.h: No such file or directory` | `main/CMakeLists.txt` has a `REQUIRES` list without `gamecontroller` | Step 3 |
| `gamecontroller: CONFIG_GAMECONTROLLER is on but no pad is selected; set CONFIG_GAMECONTROLLER_<PAD>=y (esp-forge: GAMECONTROLLER_<PAD> = y)` (a CMake warning) | No pad turned on, often because `sdkconfig` existed before you copied `sdkconfig.defaults` | Step 2: delete `sdkconfig`, build again |
| `gamecontroller: DS4/DS5 need CONFIG_BT_BLUEDROID_ENABLED, CONFIG_BT_CLASSIC_ENABLED and CONFIG_BT_HID_HOST_ENABLED; only their report parsers are built` (a CMake warning) | The pads are on, Bluetooth is not | Step 2 |

## STM32, step by step

On the STM32 this library has no Bluetooth. What it gives is each pad's
report parser and output builder (`ds4.h`, `ds5.h`, `xbox.h`), plain C:
your app gets the pad's HID input reports some other way, its own
transport, and hands them over; the parser turns them into the same
`struct gamecontroller_state` as on ESP-IDF. The steps use the DualShock 4.

**You need** a project for an STM32 that uses ST's **HAL**, the vendor
library behind `HAL_Init()` and friends; STM32CubeMX makes one, and with
**Toolchain / IDE: CMake** a CMake one, the main case below. The parsers
need no HAL module: no peripheral to enable for this library.

### With CMake (a CubeMX CMake project)

1. **Get the library.** In the project folder, next to the top-level
   `CMakeLists.txt`:

   ```sh
   git clone https://github.com/JonasAtelier/mcu-gamecontroller
   ```

   ```
   my-board/
   ├── CMakeLists.txt                  step 2
   ├── cmake/stm32cubemx/              made by CubeMX
   ├── Core/
   │   └── Src/main.c                  step 4
   ├── Drivers/                        the HAL, copied in by CubeMX
   └── mcu-gamecontroller/             step 1
   ```

2. **Add it to the build.** In the top `CMakeLists.txt`, after the line
   `add_subdirectory(cmake/stm32cubemx)`, add:

   ```cmake
   set(CONFIG_GAMECONTROLLER_DS4 ON)   # your pad's parser; one line per pad
   set(MCU_HAL_TARGET stm32cubemx)
   set(MCU_DEVICE_HEADER stm32f4xx.h)  # your family's header
   add_subdirectory(mcu-gamecontroller)
   ```

   and add `gamecontroller` to the link line CubeMX already wrote, so it
   reads:

   ```cmake
   target_link_libraries(${CMAKE_PROJECT_NAME} stm32cubemx gamecontroller)
   ```

   Do not add `PRIVATE`: CubeMX's line has no such keyword, and CMake refuses
   to mix the two forms on one target.

   `stm32cubemx` is a CMake *target*, a named bundle of settings, that CubeMX
   makes to carry the HAL's include folders and defines; `MCU_HAL_TARGET`
   lends those to the library, which works with any CubeMX version.
   `MCU_DEVICE_HEADER` is your chip family's main header.

3. **No HAL module to check.** The parsers are plain C; the library's
   clock calls only the core HAL.

4. **Write the smallest test.** It is
   [examples/stm/parsers/main.c](../examples/stm/parsers/main.c) fitted into
   CubeMX's `Core/Src/main.c`. Paste each part between the
   `USER CODE BEGIN` and `END` comments named, so a regenerate keeps it.

   After `/* USER CODE BEGIN Includes */`:

   ```c
   #include <stdio.h>

   #include "ds4.h"
   #include "gamecontroller.h"
   ```

   After `/* USER CODE BEGIN 0 */`, just above `int main(void)`:

   ```c
   static struct gamecontroller_state pad = {
     .battery = GAMECONTROLLER_BATTERY_UNKNOWN,
   };

   /*
    * Your transport calls this with each DualShock 4 Bluetooth input report:
    * its id, then the bytes after it.
    */
   void on_ds4_report(uint8_t report_id, const uint8_t *data, size_t len)
   {
     if (!ds4_parse(report_id, data, len, &pad))
       return; /* another report, or a bad CRC */
     if (gamecontroller_pressed(&pad, GAMECONTROLLER_BTN_SOUTH))
       printf("Cross, left stick %d %d\n", pad.lx, pad.ly);
   }
   ```

   After `/* USER CODE BEGIN 2 */`, which comes after the `MX_..._Init()`
   calls:

   ```c
     /* Rumble and a blue light bar, as the report the pad takes. */
     const struct ds4_output out = { .strong = 128, .weak = 64, .blue = 255 };
     uint8_t report[DS4_OUTPUT_LEN];

     printf("ds4 output report: %u bytes\n",
            (unsigned)ds4_build_output(&out, report));
   ```

   The lines go out through `printf()`: send that to a UART by writing
   newlib's `_write()`. [API usage](usage.md) explains the calls.

5. **Build** as before, for example `cmake --build` with your build folder.
   A good build shows a `Linking C executable` line and no error. On the
   UART: `ds4 output report: 78 bytes`, the report your transport would
   send to the pad.

**If it fails**

| You see | Why | Fix |
|---|---|---|
| `gamecontroller: no pad is selected; set(CONFIG_GAMECONTROLLER_<PAD> ON) before add_subdirectory()` (a CMake warning), then ``undefined reference to `ds4_build_output'`` | No pad turned on, or turned on after `add_subdirectory(mcu-gamecontroller)` | Step 2 |
| `#error "gamecontroller: CMSIS_device_header is not defined; ...` | `MCU_DEVICE_HEADER` is missing | Step 2 |
| `fatal error: stm32f4xx.h: No such file or directory` | `MCU_HAL_TARGET` is missing | Step 2 |
| `Target "stm32cubmx" not found.` (a CMake error) | `MCU_HAL_TARGET` is misspelt | Step 2 |
| `The plain signature for target_link_libraries has already been used with the target` | `PRIVATE` added to the link line | Step 2 |

### Without CMake (STM32CubeIDE, a Makefile)

1. Copy `include/` and `src/` of this repository into your project, for
   example to `Middlewares/mcu-gamecontroller/`, keeping the pad folders
   inside `src/`.
2. Add every `.c` file under `src/` to the build, pad folders included;
   `src/gamecontroller_bt.c` compiles to nothing here.
3. Add two include paths, `Middlewares/mcu-gamecontroller/include` and
   `Middlewares/mcu-gamecontroller/src`, and two defines:
   `CMSIS_device_header="stm32f4xx.h"`, quotes included, and
   `CONFIG_GAMECONTROLLER_DS4=1`, one per pad. CubeMX already defines
   `USE_HAL_DRIVER` and your device, such as `STM32F405xx`.
4. Steps 4 and 5 of the CMake guide apply as they are.

Where these settings usually live: in STM32CubeIDE, **Project → Properties →
C/C++ Build → Settings → Tool Settings → MCU GCC Compiler** (Include paths,
Preprocessor); in a CubeMX Makefile, `C_SOURCES`, `C_INCLUDES` and `C_DEFS`.
The build errors are the ones in the table above; the fix is the matching
define or include path instead of a CMake line. One more:
`#include expects "FILENAME" or <FILENAME>` means `CMSIS_device_header` lost
its quotes.

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

Setup: [ESP-IDF, step by step](#esp-idf-step-by-step). For reference:

- In `main/idf_component.yml`, `version` is a git ref; `stable` is the
  released one.
- A clone in `components/` is a local component, named after its folder;
  that name is what other components `REQUIRES`. The examples in
  `examples/esp/` need no name: their `main` lists no `REQUIRES`, and ESP-IDF
  gives `main` every component, so they build from any checkout name.
- The component always requires `bt` and `esp_hid`, as esp-ps-controller
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

Setup: [With CMake](#with-cmake-a-cubemx-cmake-project) or
[Without CMake](#without-cmake-stm32cubeide-a-makefile). The
`add_subdirectory()` route builds a static library `gamecontroller`. For
reference:

- `CMSIS_device_header` is your family's header, such as `"stm32f4xx.h"`;
  built here against the F4 HAL only.
- Each `CONFIG_GAMECONTROLLER_<PAD>` (and `CONFIG_GAMECONTROLLER_DEBUG`)
  that is true reaches the compiler as `CONFIG_GAMECONTROLLER_<PAD>=1`, for
  the library and your app. `GAMECONTROLLER_MAX_PADS`, a number, is the
  Bluetooth glue's and is not read here.
- `MCU_HAL_TARGET` is a target that carries your `USE_HAL_DRIVER`, device
  define (`STM32F405xx`, ...) and the include dirs of the HAL, CMSIS and your
  `stm32f4xx_hal_conf.h`. The library copies only those include dirs and
  defines, never the target's sources, so this works with any CubeMX
  version. `MCU_DEVICE_HEADER` becomes `CMSIS_device_header`, which CubeMX
  does not define. Every `mcu-*` library reads these two, so set them once.
  Leave them unset if your project already provides both.
- The CPU flags (`-mcpu`, `-mfloat-abi`, ...) must reach the library too;
  CubeMX's toolchain file sets them for every target.
- Without CMake, `src/` only needs to be on the include path of this
  library's own sources; the defines are what stm-forge passes.

**There is no Bluetooth transport on the HAL.** What builds is the core,
the bus layer, and the pads' report parsers and output builders (`ds4.h`,
`ds5.h`, `xbox.h`): plain C for an app that gets the pad's HID reports some
other way. `src/gamecontroller_bt.c` and the `ps_controller_*` calls are
ESP-IDF only and compile to nothing here. With the library on and no pad
picked, the CMakeLists warns (ESP-IDF and `add_subdirectory()`) and
`src/gamecontroller_port_stm.c` emits a `#warning`.

**printf() is the log**, as in every category library: retarget newlib's
`_write()` to a UART.
