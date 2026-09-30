# 2. Firmware Setup

## 2.1 Required Arduino Libraries

Install these via **Sketch → Include Library → Manage Libraries** in the Arduino IDE (or manually if not listed there):

| Library | Purpose | Notes |
|---|---|---|
| `INA226_WE` (by Wolfgang Ewald) | Reading the three INA226 power monitors | Chosen over Adafruit's INA226 library for its explicit shunt-resistor/current-range calibration |
| `Adafruit_ST7796S_kbv` | Driving the display | Confirmed working on Arduino Mega2560 by a verified buyer review of this exact panel — the bundled Hosyond demo library is not used (multiple reviewers reported it buggy/outdated) |
| `Adafruit GFX Library` | Required dependency of `Adafruit_ST7796S_kbv` | Provides the drawing primitives (text, shapes) the display driver builds on |
| `Arduino-FT6336U` (by aselectroworks, GitHub) | Reading the capacitive touch controller | May need manual ZIP install if not in the Library Manager index — search GitHub `aselectroworks/Arduino-FT6336U` |

`Wire.h` and `SPI.h` are built into the Arduino core and don't need separate installation.

## 2.2 Board Selection

In the Arduino IDE: **Tools → Board → Arduino AVR Boards → Arduino Mega or Mega 2560**, then **Tools → Processor → ATmega2560 (Mega 2560)**.

## 2.3 Project Structure

The sketch is modular — each file has one responsibility:

| File | Responsibility |
|---|---|
| `UWBG_Power_Converter.ino` | Top-level `setup()`/`loop()` only — calls into modules |
| `Config.h` | All pin numbers, I2C addresses, and tuning constants in one place |
| `Sensors.h` / `.cpp` | INA226 sensor init and reading (solar, wind, battery) |
| `BoostConverter.h` / `.cpp` | Timer1 hardware PWM generation for the gate drive |
| `ChargeController.h` / `.cpp` | CC/CV charging algorithm and fault detection |
| `TestMode.h` / `.cpp` | Manual duty-cycle override for bench testing |
| `Display.h` / `.cpp` | Display and touch controller |

All `.h`/`.cpp` files must remain in the same folder as `UWBG_Power_Converter.ino` — the Arduino IDE compiles everything in the sketch folder together automatically.

## 2.4 Uploading

1. Connect the Mega via USB
2. Select the correct **Tools → Port**
3. Click **Upload**
4. Once uploaded, open **Tools → Serial Monitor**, set baud rate to **9600**

## 2.5 Before Every Upload — Check `Config.h`

The single most important thing to check before uploading is `MANUAL_PWM_TEST_MODE` at the bottom of `Config.h` — see [03-Operating-Modes.md](03-Operating-Modes.md) for what each setting does. Uploading with the wrong mode active is the most common source of "why isn't it charging" or "why won't the PWM move" confusion.
