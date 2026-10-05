# 1. Wiring and Assembly

All pin numbers below refer to the **Arduino GIGA R1 WiFi**, and all come directly from `Config.h` — if you rewire something, update `Config.h` to match, not just this document.

> **Board change:** the project was originally written for the Arduino Mega 2560. The Giga R1 WiFi is now the target board. The pin numbers for signals on digital pins are the same, but the display's SPI wires move to the Giga's main SPI header (see §1.3). The gate-drive PWM code is not yet ported; see §1.2.

## 1.1 I2C Sensor Bus (INA226 × 4, + 1 placeholder)

All INA226 power monitors share the Giga's I2C bus:

| Signal | Giga Pin |
|---|---|
| SDA | 20 |
| SCL | 21 |

Each sensor needs a **unique I2C address**, set by bridging its A0/A1 solder jumpers:

| Sensor | Address | Jumper Setting | Measures |
|---|---|---|---|
| Solar | `0x40` | A0 = GND, A1 = GND (default) | Solar array output, before its SB540 isolation diode |
| Battery | `0x41` | A0 = VS, A1 = GND | Boost converter output / battery charging side |
| Wind | `0x42` | A0 = SDA, A1 = GND | Wind branch output, before its SB540 isolation diode |
| Common DC Bus | `0x43` | A0 = SCL, A1 = GND | After both isolation diodes combine, right before the boost converter's actual input |
| Load *(placeholder — do not wire yet)* | `0x44` | A1 = VS, A0 = GND | **No physical location exists yet.** The buck converter (12.8V→5V) is still an undecided hardware choice — this sensor is a code-only placeholder for its future output. Wiring this up before the buck converter exists would just measure nothing meaningful. |

**Verify each jumper against your specific board's silkscreen before soldering** — jumper labeling can vary slightly between suppliers of the CJMCU-226 module.

All three sensors use a **0.010 Ω shunt resistor** (the CJMCU-226 board's onboard value) and are calibrated in firmware for a **0–2.0 A** current range.

## 1.2 Boost Converter Gate Drive

| Signal | Giga Pin | Notes |
|---|---|---|
| Gate drive PWM (to TC4427 driver input) | 11 | Also the SPI5 COPI pin, so keep that SPI bus unused. 100 kHz frequency confirmed on the scope. Duty-step resolution still open. |

**Status:** the Giga's frequency is confirmed at 100 kHz, but the `BoostConverter.cpp` port is blocked until the duty-step readings are in. The current file still uses Mega Timer1 registers and will not compile on the Giga.

## 1.3 Display and Touch (Hosyond 3.5" ST7796U / FT6336U)

| Signal | Giga Pin | Notes |
|---|---|---|
| LCD_CS | 10 | Display chip-select |
| LCD_RST | 9 | Display reset |
| LCD_RS (DC) | 8 | Display data/command select |
| LED | 7 | Backlight (currently always-on; PWM-capable for future dimming) |
| SDI (MOSI) | D90 (COPI) | Giga main SPI header — fixed by the board, not a firmware choice |
| SCK | D91 | Giga main SPI header — fixed by the board |
| SDO (MISO) | D89 (CIPO) | Giga main SPI header — fixed by the board |
| CTP_SDA | 20 | **Shared** with the INA226 I2C bus above |
| CTP_SCL | 21 | **Shared** with the INA226 I2C bus above |
| CTP_RST | 6 | Touch controller reset |
| CTP_INT | 3 | Touch controller interrupt |
| SD_CS | *(not connected)* | This project doesn't use the display's microSD slot |
| VCC | 5V | Display working voltage (5V recommended over 3.3V — see product spec sheet) |
| GND | GND | |

## 1.4 Power

- The Giga, sensors, and display share a common ground with the boost converter's power stage.
- The wind-drive motor's battery (Hyper Tough 12V Li-ion) is **electrically separate** from the renewable energy path — it only powers the mechanical wind-simulation motor, and should never be wired into the solar/wind/battery sensing or charging circuit.

## 1.5 Assembly Checklist

- [ ] All three INA226 sensors have distinct, verified I2C addresses
- [ ] Gate drive signal (pin 11) routes to the TC4427 driver input, not directly to the GaN gate
- [ ] Display and touch controller share the I2C bus correctly (no address conflict with INA226 sensors — display's touch chip uses its own fixed I2C address separate from 0x40/0x41/0x42)
- [ ] Common ground verified across all modules with a multimeter before first power-up
- [ ] Wind-drive battery confirmed isolated from the renewable energy/charging circuit
