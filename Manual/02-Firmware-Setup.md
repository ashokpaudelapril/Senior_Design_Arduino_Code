# 2. Firmware Setup

## 2.1 Required Arduino Libraries

Install these via **Sketch → Include Library → Manage Libraries** in the Arduino IDE (or manually if not listed there):

| Library | Purpose | Notes |
|---|---|---|
| `INA226_WE` (by Wolfgang Ewald) | Reading the three INA226 power monitors | Chosen over Adafruit's INA226 library for its explicit shunt-resistor/current-range calibration |
| `Adafruit_ST7796S_kbv` | Driving the display | Confirmed working on Arduino Mega2560 by a verified buyer review of this exact panel — the bundled Hosyond demo library is not used (multiple reviewers reported it buggy/outdated) |
| `Adafruit GFX Library` | Required dependency of `Adafruit_ST7796S_kbv` | Provides the drawing primitives (text, shapes) the display driver builds on |
| `Arduino-FT6336U` (by aselectroworks, GitHub) | Reading the capacitive touch controller | May need manual ZIP install if not in the Library Manager index — search GitHub `aselectroworks/Arduino-FT6336U` |
| `OneWire` | 1-Wire bus protocol for the DS18B20 temperature probes | Standard, well-maintained library |
| `DallasTemperature` | DS18B20-specific reading on top of `OneWire` | Depends on `OneWire` being installed |

`Wire.h` and `SPI.h` are built into the Arduino core and don't need separate installation.

## 2.2 Board Selection

In the Arduino IDE: install **Arduino Mbed OS Giga Boards** from Boards Manager, then select **Tools → Board → Arduino GIGA R1 WiFi**. The old Mega 2560 setup (ATmega2560 processor) no longer applies.

## 2.3 Project Structure

The sketch is modular — each file has one responsibility:

| File | Responsibility |
|---|---|
| `UWBG_Power_Converter.ino` | Top-level `setup()`/`loop()` only — calls into modules |
| `Config.h` | All pin numbers, I2C addresses, and tuning constants in one place |
| `Sensors.h` / `.cpp` | INA226 sensor init and reading (solar, wind, battery) |
| `BoostConverter.h` / `.cpp` | Gate-drive PWM generation. Drives STM32 TIM8 Channel 2 directly via HAL (ported from the Mega's AVR Timer1 code). |
| `ChargeController.h` / `.cpp` | CC/CV charging algorithm and fault detection |
| `TestMode.h` / `.cpp` | Manual duty-cycle override for bench testing |
| `Display.h` / `.cpp` | Display and touch controller |
| `TemperatureSensors.h` / `.cpp` | GaN switch / inductor DS18B20 probes, over 1-Wire |
| `Efficiency.h` / `.cpp` | Power-in vs. power-out efficiency calculation |
| `Relays.h` / `.cpp` | Mechanism: drives the 6 relay-module channels (solar, wind, 4 demo loads) on/off, hiding the active-low electrical detail |
| `LoadControl.h` / `.cpp` | Policy: tracks what the touchscreen's Relays tab asked for, and applies the automatic "enough power available" check before actually energizing a load relay |

All `.h`/`.cpp` files must remain in the same folder as `UWBG_Power_Converter.ino` — the Arduino IDE compiles everything in the sketch folder together automatically.

## 2.4 Uploading

1. Connect the Giga via USB (use the USB-C port)
2. Select the correct **Tools → Port**
3. Click **Upload**
4. Once uploaded, open **Tools → Serial Monitor**, set baud rate to **9600**

## 2.5 Before Every Upload — Check `Config.h`

The single most important thing to check before uploading is `MANUAL_PWM_TEST_MODE` at the bottom of `Config.h` — see [03-Operating-Modes.md](03-Operating-Modes.md) for what each setting does. Uploading with the wrong mode active is the most common source of "why isn't it charging" or "why won't the PWM move" confusion.
