# Architecture, driver development and tests

[Back to README](../README.md)

Paths mentioned in this guide are relative to the repository root.

## Repository layout

```
include/                    public API, the only directory apps include
  gamecontroller.h          struct gamecontroller_dev and _state, the outputs
  gamecontroller_log.h      gamecontroller_err/warn/info/dbg, gamecontroller_log_state()
  gamecontroller_bt.h       pads over Bluetooth (ESP-IDF)
  ds4.h ds5.h xbox.h        each pad's report parser and output builder
  ps_controller.h           esp-ps-controller's calls, plus ps_controller_dev()
  ps_reports.h              esp-ps-controller's parser, unchanged
src/                        private to the library
  gamecontroller.c          the core: bound checks, optional outputs
  gamecontroller_log.c      gamecontroller_log_state()
  gamecontroller_math.h     stick and trigger scaling, the hat, the PlayStation CRC-32
  gamecontroller_bt.c       Bluedroid, GAP, the HID host, the pads' slots; ESP-IDF only
  gamecontroller_priv.h     bus ops, attach and probe, errno maps -- unused by the pads
  gamecontroller_port_esp.c the bus layer on ESP-IDF; empty elsewhere
  gamecontroller_port_stm.c the bus layer on the STM32 HAL; empty under ESP-IDF
  ps_reports.c              esp-ps-controller's parser, unchanged
  ds4/  ds5/  xbox/         <pad>.c  <pad>_proto.h
test/                       host tests, `make -C test test`
  test_reports.c            esp-ps-controller's test, unchanged
  test_math.c test_ds4.c test_ds5.c test_xbox.c   the pure parts, once
  vectors.h gen_vectors.py  layout-derived reports, and their generator
  core_cases.h              core cases, shared by esp/ and stm/
  esp/                      core, Bluetooth glue against fakes, port
  stm/                      core, port
  idf_stubs/  hal_stubs/    the slice of ESP-IDF and of the HAL the code calls
examples/
  esp/ps_controller/        esp-ps-controller's example, with its sdkconfig
  esp/gamecontroller/       any pad on an ESP32
  esp/xbox/                 an Xbox pad on an ESP32-S3
  stm/parsers/              the parser and builder on their own
docs/                       the five guides
assets/                     README photo
CMakeLists.txt              ESP-IDF component; elsewhere a static library for add_subdirectory(); stm-forge globs src/**/*.c instead
                            the static library's options are Kconfig's bool entries, found by the type line directly under each `config`
Kconfig                     GAMECONTROLLER and GAMECONTROLLER_<PAD>
idf_component.yml           ESP-IDF component manifest
```

Naming inside `src/<pad>/`, the same in every category library:

| File | Holds |
|---|---|
| `<pad>.c` | the pad: parser and builder |
| `<pad>_regs.h` | register map, for parts with registers |
| `<pad>_proto.h` | the protocol: report ids, offsets, bits, each with its source |
| `<pad>_math.h` | pure conversions; here shared, in `src/gamecontroller_math.h` |
| `<pad>_priv.h` | private state, only if needed |

The pads' files and `gamecontroller_bt.c` use esp-ps-controller's
four-space style; the library's own core files use tabs.

## Pads, transport and category

A pad is two things here: what its reports mean, and how they arrive. The
first is plain C in `src/<pad>/`, the same on every platform, and fully
host-tested. The second is ESP-IDF's HID host over Bluedroid, in
`gamecontroller_bt.c`: it brings Bluetooth up, scans, opens a pad, knows it
by its ids or name, feeds its reports to the right parser, and offers the
outputs as the `struct gamecontroller_dev` ops. The category is what every
pad shares: one normalised state, and optional outputs that are
`-EOPNOTSUPP` where a pad has none.

The STM32 HAL has the first half and not the second.

## The bus layer

`src/gamecontroller_priv.h` and the two port files are the bus layer every
category library carries -- I2C, SPI and UART ops, picked at run time --
with `test/{esp,stm}/test_port_*`. No pad uses it: they are on Bluetooth.
It is kept so the library has the same shape as the others.

## Adding a pad

1. `include/foo.h`: `foo_parse(report_id, data, len, &state)` and the
   output builders, plain C.
2. `src/foo/foo.c` and `src/foo/foo_proto.h`: include
   `gamecontroller_priv.h` first, then `#if CONFIG_GAMECONTROLLER_FOO`.
   Every offset cites its source, file:line.
3. In `gamecontroller_bt.c`: know the pad in `identify()`, give it an ops
   table, set it up in `on_open()`, parse it in `on_input()`.
4. `config GAMECONTROLLER_FOO` in `Kconfig`, depending on the SoC feature
   its transport needs, with `bool` directly under the `config` line; its
   file in `CMakeLists.txt` (its ESP-IDF part; the `add_subdirectory()` part
   reads `Kconfig`); `# GAMECONTROLLER_FOO
   is not set` in both forges' defconfigs; vectors in `gen_vectors.py` and
   a `test_foo.c`.

## Tests

```sh
make -C test test
```

Host-only. Nothing is checked against a pad.

**The vectors are layout-derived, not captured.** `test/gen_vectors.py`
writes `test/vectors.h` from the cited layouts, separately from the C, with
zlib's CRC-32; the C must agree byte for byte. That proves the C matches
the documents -- not that the documents match a pad.

| Binary | Checks |
|---|---|
| `test_reports` | esp-ps-controller's own: its three reports, junk refused |
| `test_math` | stick and trigger scaling at the ends; the hat; the CRC-32 against the catalogue check value and the seeded forms |
| `test_ds4` | the full report against its vector: sticks, triggers, every button kind, battery 55 % charging; a flipped bit refused by the CRC; wrong length or id; the short report keeping the battery; seven battery cases; the output report byte for byte |
| `test_ds5` | the full report; CRC, length, id; five battery cases; the simple report; **the output report byte for byte and every S1 offset pinned**, the sequence nibble, the light-bar release on its own |
| `test_xbox` | which ids are pads; the state on a One S and on a Series pad (View or Share); the hat's 0 and 8; the Xbox button moving to report 0x02; four battery cases; refused lengths and ids; the rumble report |
| `esp/test_core`, `stm/test_core` | the state read into scratch; outputs and `-EOPNOTSUPP`; unbound pads; the log lines |
| `esp/test_bt` | the glue against fakes of the controller, Bluedroid, both GAPs and the HID host: bring-up failure returned (U1); nothing released with both kinds built, dual mode (U2); "Xbox Wireless Controller" not opened by the Classic scan (U3); a DS4 found, opened, asked for 0x02 and 0x05, its report in the state and in `ps_controller_get()`, a bad CRC moving neither, outputs as one report; the disconnect cleared under the lock (U4); a DualSense by name, its light-bar release then colour, player LEDs; an Xbox pad by BLE appearance and ids, its security request accepted, rumble; a non-Xbox BLE gamepad and a pad past the slots closed; a timeout on the clock; `ps_controller_start_mac()` |
| `esp/test_port_esp`, `stm/test_port_stm` | the bus layer, as in every category library |

Each fix and each cited offset is checked by a test that fails without it:
the DualSense output in SDL's layout, a DualShock 4 offset moved, the hat
table, the output CRC seed, the trigger scaling, U1 back to
`ESP_ERROR_CHECK`, U2's unconditional release, U3's substring match, U4
without the lock, DualSense rumble v2, the Xbox Share bit, the Xbox motors
swapped, and the DualShock 4's CRC check removed.

All build under `-std=gnu11 -Wall -Wextra -Werror -pedantic` with UBSan.
