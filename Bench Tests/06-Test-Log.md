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
| 2026-10-05 | Duty at typed value (unknown) | 30.01% (IMG_8996, high 3.00 µs) | Unknown | BLOCKED | Typed value for this capture not recorded |
| 2026-10-05 | Duty at typed value (unknown) | 10.00% (IMG_8997, high 1.00 µs) | Unknown | BLOCKED | Typed value for this capture not recorded |
| | Duty at 26, 27, 28 | | Distinct steps | BLOCKED | Not yet measured. High times fall on whole µs, which suggests 10% steps at this period. Needs confirmation before porting `BoostConverter` |

## Issues Found

| Date | Issue | Cause (if known) | Fix | Retested? |
|---|---|---|---|---|
| 2026-10-05 | Mega 2560 would not upload: USB dropped and timeouts, even with a minimal sketch | Not determined. Board swapped to Giga R1 WiFi | Using Giga instead | |
| 2026-10-05 | Giga compile: `TCCR1A`, `ICR1` and related Timer1 registers not declared | Mega-only register code in `BoostConverter.cpp` | Port required once PWM resolution is confirmed | |

## Changes Made to Firmware or Config During Testing

| Date | File / constant | Old value | New value | Reason |
|---|---|---|---|---|
| | | | | |
