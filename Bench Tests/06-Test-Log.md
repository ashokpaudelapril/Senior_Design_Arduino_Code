# 6. Test Log

Record every run here, including failures and blocked tests. One row per test run; add a new table each session.

## Session Information

| Field | Entry |
|---|---|
| Date | |
| Firmware version (last edit date of `UWBG_Power_Converter/`) | |
| `MANUAL_PWM_TEST_MODE` setting | |
| Tester | |
| Ambient conditions (temp, notes) | |

## Results Log

| Date | Section | Test | Measured | Expected | PASS / FAIL / BLOCKED | Notes / action taken |
|---|---|---|---|---|---|---|
| | 01 | Power-off short check | | No short | | |
| | 01 | First power-up messages | | Both init lines | | |
| | 02 | Solar voltage | | | | |
| | 02 | Battery voltage | | | | |
| | 02 | Wind voltage | | | | |
| | 02 | Common bus voltage | | | | |
| | 02 | Solar current (1 kΩ) | | 0.0120 A | | |
| | 02 | Battery current (1 kΩ) | | 0.0120 A | | |
| | 02 | Wind current (1 kΩ) | | 0.0120 A | | |
| | 02 | Common bus current (1 kΩ) | | 0.0120 A | | |
| | 03 | PWM frequency | | 100 kHz | | |
| | 03 | Duty at typed 25 | | 24.4% | | |
| | 03 | Duty at typed 50 | | 49.4% | | |
| | 03 | Duty at typed 80 | | 79.4% | | |
| | 04 | Hard voltage trip | | Trips at >15 V | | |
| | 04 | Hard current trip | | Trips at >1.5 A | | |
| | 04 | Sensor missing fault | | Fault appears | | |
| | 04 | Open-circuit fault | | Fault after ~11 s | | |
| | 04 | Manual input refused | | Refused in FAULT | | |
| | 04 | Reset clears fault | | Fault cleared | | |
| | 05 | CC phase current | | 1.0 A ±0.1 | | |
| | 05 | CV phase voltage | | ≤14.6 V | | |
| | 05 | Efficiency vs hand calc | | ±3 points | | |

## Giga R1 WiFi PWM Results (board swap from Mega 2560)

| Date | Test | Measured | Expected | PASS / FAIL / BLOCKED | Notes |
|---|---|---|---|---|---|
| 2026-10-05 | Frequency, `PWM_Giga_Test` on pin 11 | 100.01 kHz (IMG_8996), 100.02 kHz (IMG_8997), period 10.00 µs | 100 kHz | PASS | Scope captures in project folder |
| 2026-10-05 | Duty at typed value (unknown) | 30.01% (IMG_8996, high 3.00 µs) | Unknown | FAIL | Typed value not recorded, but superseded by the finding below |
| 2026-10-05 | Duty at typed value (unknown) | 10.00% (IMG_8997, high 1.00 µs) | Unknown | FAIL | Typed value not recorded, but superseded by the finding below |
| 2026-10-10 | Duty at 25, 26, 27, 28, 29, 30 (Mbed `PwmOut`, `write()`/`pulsewidth()`) | 25–29% all read 20%; 30% read 30% | Distinct steps | FAIL | Confirms floor-to-whole-microsecond rounding inside Mbed's PWM implementation itself (both `write()` and `pulsewidth()` affected, not just input handling) |
| 2026-10-10 | Duty at 35, 55, 58 (direct STM32 TIM8 HAL control, bypassing Mbed `PwmOut`) | Correct, distinct values at each | Distinct steps | PASS | Root cause fixed. See `PWM_Giga_Test.ino` and the ported `BoostConverter.cpp`. TIM8 kernel clock measured (not assumed) at 240MHz via `HAL_RCC_GetPCLK2Freq()` + `HAL_RCC_GetClockConfig()` — confirmed the APB2 prescaler's 2x multiplier applies. 2400 ticks/period = ~0.042% duty resolution |

## Full Firmware Compile (post-`BoostConverter` port)

| Date | Test | Result | Notes |
|---|---|---|---|
| 2026-10-10 | `UWBG_Power_Converter.ino` — all modules (Sensors, BoostConverter, ChargeController, TestMode, Display, Efficiency) compiled together for Giga R1 WiFi | PASS | 142264 bytes flash (7% of 1966080), 54424 bytes RAM (10% of 523624). No errors. This is the first clean full-firmware compile since the Mega→Giga board swap |

## Issues Found

| Date | Issue | Cause (if known) | Fix | Retested? |
|---|---|---|---|---|
| 2026-10-05 | Mega 2560 would not upload: USB dropped and timeouts, even with a minimal sketch | Not determined. Board swapped to Giga R1 WiFi | Using Giga instead | |
| 2026-10-05 | Giga compile: `TCCR1A`, `ICR1` and related Timer1 registers not declared | Mega-only register code in `BoostConverter.cpp` | Port required once PWM resolution is confirmed | |
| 2026-10-10 | Giga compile: `Serial`/`F()`/`millis()` not declared in `ChargeController.cpp`; touch object name clash + missing colors in `Display.cpp` | Missing `#include <Arduino.h>`; library's `TouchStatusEnum::touch` clashing with our `touch` object; `Adafruit_GFX` not providing `BLACK`/`WHITE`/etc. macros on this core | Added the include; renamed to `touchCtrl`; defined local `COLOR_*` RGB565 constants | Yes — compiles clean |

## Changes Made to Firmware or Config During Testing

| Date | File / constant | Old value | New value | Reason |
|---|---|---|---|---|
| 2026-10-10 | `Config.h`: `PWM_TOP_VALUE` / `F_CPU`-based math | AVR-only, 159 ticks/period | `TIM8_CLOCK_HZ = 240000000UL`, `PWM_PERIOD_TICKS = 2400`, `PWM_ARR_VALUE = 2399` | Giga port — AVR registers don't exist on STM32H7; clock value measured on hardware, not assumed |
| 2026-10-10 | `BoostConverter.cpp`: entire implementation | AVR Timer1 register config | STM32 TIM8 Channel 2 via HAL | Giga port; public interface (`boost_init()`, `boost_setDutyCycle()`) unchanged |
