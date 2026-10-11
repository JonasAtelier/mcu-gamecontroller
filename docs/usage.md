# API usage

[Back to README](../README.md)

Paths mentioned in this guide are relative to the repository root.

## The pad and its state

Every pad is a `struct gamecontroller_dev` (`gamecontroller.h`):

| Call | Does |
|---|---|
| `gamecontroller_get_state(gc, &s)` | the latest state, copied out whole |
| `gamecontroller_pressed(&s, GAMECONTROLLER_BTN_*)` | whether a button is down |
| `gamecontroller_set_rumble(gc, strong, weak)` | the motors, 0..255 each |
| `gamecontroller_set_lightbar(gc, r, g, b)` | the light bar |
| `gamecontroller_set_player_leds(gc, mask)` | the player LEDs, bits 0..4 |
| `gamecontroller_log_state(gc, &s)` | one info line |

An output the pad does not have is `-EOPNOTSUPP`:

| Pad | Rumble | Light bar | Player LEDs |
|---|---|---|---|
| DualShock 4 | yes | yes | -- |
| DualSense | yes | yes | yes |
| Xbox | yes | -- | -- |

`struct gamecontroller_state`, the same for every pad:

| Field | Range | Notes |
|---|---|---|
| `lx`, `ly`, `rx`, `ry` | -32768..32767 | +x right, **+y down**, as the pads and Linux report it |
| `lt`, `rt` | 0..65535 | scaled from 8 bits (PlayStation) or 10 bits (Xbox) |
| `buttons` | `GAMECONTROLLER_BTN_*` | by position, below |
| `battery` | 0..100, or `GAMECONTROLLER_BATTERY_UNKNOWN` | PlayStation in tens; Xbox in four steps |
| `charging` | bool | |
| `connected` | bool | false once the pad has gone |
| `seq` | | +1 for every report that changed the state |

Buttons are named by position, as Linux's `gamepad.rst` names them:

| Bit | DualShock 4 / DualSense | Xbox |
|---|---|---|
| `SOUTH`, `EAST`, `WEST`, `NORTH` | Cross, Circle, Square, Triangle | A, B, X, Y |
| `TL`, `TR` | L1, R1 | LB, RB |
| `TL2`, `TR2` | L2, R2 pressed | -- (the triggers are analog only) |
| `SELECT`, `START`, `MODE` | Share/Create, Options, PS | View, Menu, Xbox |
| `THUMBL`, `THUMBR` | L3, R3 | left, right stick click |
| `DPAD_UP` .. `DPAD_RIGHT` | the d-pad; diagonals set two | the d-pad |
| `TOUCHPAD` | touchpad click | -- |
| `MUTE` | mic mute (DualSense) | -- |
| `CAPTURE` | -- | Share (Series pads) |

## Over Bluetooth (ESP-IDF)

`gamecontroller_bt.h`:

| Call | Does |
|---|---|
| `gamecontroller_bt_start(&cfg)` | Bluetooth up (once), scan, connect the first pad found; blocks until one is connected or `scan_seconds` (0: 20) pass |
| `gamecontroller_bt_pad(i)` | the pad in slot i, or NULL |
| `gamecontroller_bt_count()` | pads connected now |

`cfg.playstation` looks for DualShock 4 and DualSense pads by their
Bluetooth Classic names; `cfg.xbox` looks for a BLE device advertising the
gamepad appearance, and keeps it only if its ids are an Xbox pad's. A kind
the build or chip has no transport for is `ESP_ERR_NOT_SUPPORTED`.

On connect a DualShock 4 is asked for feature reports 0x02 and 0x05, a
DualSense for 0x05: the reads that switch them from their short report to
the full one, with battery. Until then the short report still gives
sticks, triggers and buttons.

A pad that goes away reads `connected = false`, everything else zero, and
its slot is freed; `gamecontroller_bt_pad()` returns NULL for it.

### esp-ps-controller's calls

`ps_controller.h` and `ps_reports.h` are esp-ps-controller's, unchanged:

| Call | Now |
|---|---|
| `ps_controller_start(seconds)` | `gamecontroller_bt_start()` for PlayStation pads |
| `ps_controller_start_mac(mac)` | connects to that address over Classic, waits 10 s |
| `ps_controller_connected()` | a PlayStation pad is connected |
| `ps_controller_get(&s)` | the first PlayStation pad, in `ps_state_t`'s int8 ranges |
| `ps_parse(id, data, len, &s)` | esp-ps-controller's parser, unchanged |
| `ps_controller_dev()` | new: that pad as a `struct gamecontroller_dev` |

## Threads

`gamecontroller_get_state()` and `ps_controller_get()` copy the state under
a lock and may be called from any task. The outputs build a report and hand
it to the HID host; call them from one task per pad. The HID host's event
task parses the reports.

## Without Bluetooth (any platform)

`ds4.h`, `ds5.h` and `xbox.h` are the pads' report parsers and output
builders, plain C:

```c
struct gamecontroller_state s = { .battery = GAMECONTROLLER_BATTERY_UNKNOWN };
if (ds4_parse(report_id, data, len, &s))      /* data: after the report id */
    use(&s);

uint8_t out[DS4_OUTPUT_LEN];
struct ds4_output o = { .strong = 128, .weak = 64, .blue = 255 };
send(out, ds4_build_output(&o, out));         /* the whole report, id and CRC */
```

A parser updates the state only on a report it understands and whose CRC
matches; otherwise it returns false and leaves the state alone.

## Logging

On ESP-IDF everything goes under the log tag `gamecontroller`, prefixed with
the pad: `ds4`, `ds5`, `xbox`, or `bt` for the scan. On the HAL, through
`printf()`. `gamecontroller_err/warn/info/dbg()` are in
`gamecontroller_log.h`.

## Errors

`gamecontroller_*` calls return 0 or a negative errno; the Bluetooth calls
return `esp_err_t`:

| Meaning | errno | esp_err_t |
|---|---|---|
| bad argument, NULL pad | `-EINVAL` | `ESP_ERR_INVALID_ARG` |
| the pad has no such output; a pad kind without transport | `-EOPNOTSUPP` | `ESP_ERR_NOT_SUPPORTED` |
| no pad within the time | -- | `ESP_ERR_TIMEOUT` |
| the HID host refused an output report | its errno | -- |
| a Bluetooth bring-up step failed | -- | that step's code |

## Changes from esp-ps-controller

`ps_controller.h` is token-for-token the original's, plus
`ps_controller_dev()` and the `gamecontroller.h` it needs; `ps_reports.h`,
`ps_reports.c` and `test/test_reports.c` are byte-identical. Behaviour
changes, deliberately:

- **Bring-up failures are returned** (U1). The original wrapped every
  Bluetooth step in `ESP_ERROR_CHECK`, which aborted the app when, say,
  another component had brought Bluedroid up first. Now each step runs only
  if it is not done yet, and a failure is the call's return value.
- **The BLE memory is kept** when the build has a BLE pad (U2). The original
  released it for good on every start, after which no BLE device could ever
  be used. Now memory is released only for a mode the build can never use.
- **The scan matches whole names** (U3): "Wireless Controller" and
  "DualSense Wireless Controller". The original matched "Wireless
  Controller" anywhere in a name, which "Xbox Wireless Controller" also
  contains, and would open an Xbox pad as a PlayStation one.
- **A disconnect clears the state under the lock** (U4). The original
  cleared it outside the critical section `ps_controller_get()` takes, so a
  read on the other core could see half of it.
- **Full reports are checked**: a 0x11 or 0x31 report whose CRC does not
  match (hid-playstation.c:1341, 2250) is dropped, for `ps_controller_get()`
  too.
- **The DualShock 4 is asked for 0x02 and 0x05, the DualSense for 0x05**,
  where the original asked every pad for both.
- The example `examples/esp/ps_controller` is the original's `main.c`
  unchanged, with the `sdkconfig.defaults` it lacked: without them Bluetooth
  Classic HID host is off and the original example does not build.
- Log lines go under the tag `gamecontroller`, not `ps_controller`.
