# LED Cube 4×4×4 — ESP32-C3 + BLE

A 64-LED cube controlled by an ESP32-C3 SuperMini, two daisy-chained TPIC6B595 shift registers, and four layers switched by NPN-driven P-channel MOSFETs. Arduino firmware with BLE commands and five sequential animation effects.

## Firmware

Open `led_cube_ble/led_cube_ble.ino` in the Arduino IDE with the **esp32 board package by Espressif Systems** installed. Select the appropriate ESP32-C3 board (ESP32C3 Dev Module where applicable) and serial port. The code uses the BLE library included in the ESP32 package, along with `<atomic>`.

This version preserves the behavior of the animation code provided in the conversation. It has not been compiled or validated on hardware as part of this delivery; no specific ESP32 board package version has been pinned.

## Pin Mapping

| Function | GPIO |
|---|---:|
| TPIC SER IN / Data | 4 |
| TPIC SRCK / Clock | 5 |
| TPIC RCK / Latch | 6 |
| Layer 1 / NPN base | 7 |
| Layer 2 / NPN base | 8 |
| Layer 3 / NPN base | 9 |
| Layer 4 / NPN base | 10 |

Driving a layer GPIO high turns on the NPN transistor, which in turn switches on the P-channel MOSFET. A bit set to 1 in the TPIC activates the corresponding current-sinking output. The physical column order depends on the wiring.

## Using nRF Connect

1. Power on the board and connect to `Cubo_LED_4x4x4`.
2. Open service `6e400001-b5a3-f393-e0a9-e50e24dcca9e`.
3. Write UTF-8 text to characteristic `6e400002-b5a3-f393-e0a9-e50e24dcca9e`.

| Command | Action |
|---|---|
| `ON` or `1` | Start/restart the sequence |
| `OFF` or `0` | Turn off the cube |

The cube starts with all LEDs off when powered on. Disconnecting the app does not stop an ongoing animation; the device resumes advertising to allow reconnection. Commands do not require BLE authentication.

## Effects

1. Layers moving up and down.
2. Vertical columns lighting up in sequence.
3. Eight twinkling points.
4. Filling and emptying the cube layer by layer.
5. Three flashes followed by a pause.

The sequence repeats until `OFF` is received. Animation updates use `millis()`, and multiplexing uses `micros()`, with a nominal period of 2 ms per layer and a blanking interval of 30 µs. At most one layer is activated at a time. Timing and duration can be adjusted through the constants and the `updateAnimation()` function.

## Hardware Notes

- Cube supply: 5 V, with a common ground shared with the ESP. The power supply's current rating does not determine the current capacity of the SuperMini's PCB traces.
- GPIO8 and GPIO9 are boot strapping pins. The NPN base connections may interfere with startup, especially on GPIO9; initializing the outputs in firmware does not correct their levels during reset.
- According to its datasheet, a TPIC6B595 powered at 5 V requires a minimum logic-high input of 0.85 × VCC = 4.25 V. Direct drive from 3.3 V GPIOs is not guaranteed; appropriate logic-level conversion should be provided.
- With 100 Ω column resistors and an LED forward voltage of approximately 3 V, the simplified estimate is 20 mA per LED and 320 mA for a full layer, plus the control electronics. Measure the actual current before sizing the power supply.

References: [TPIC6B595 — TI](https://www.ti.com/lit/ds/symlink/tpic6b595.pdf), [ESP32-C3 Boot Mode Selection — Espressif](https://docs.espressif.com/projects/esptool/en/latest/esp32c3/advanced-topics/boot-mode-selection.html).
