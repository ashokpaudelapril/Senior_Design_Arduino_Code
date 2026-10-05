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

## Issues Found

| Date | Issue | Cause (if known) | Fix | Retested? |
|---|---|---|---|---|
| | | | | |

## Changes Made to Firmware or Config During Testing

| Date | File / constant | Old value | New value | Reason |
|---|---|---|---|---|
| | | | | |
