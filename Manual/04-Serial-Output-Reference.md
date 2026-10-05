# 4. Serial Output Reference

All output is at **9600 baud**. Below is every message the firmware prints and what it means.

## 4.1 Startup Messages (printed once, from `setup()`)

| Message | Meaning |
|---|---|
| `Solar, battery, and wind INA226 sensors initialized.` | I2C bus is up and all three sensors were configured (does NOT confirm they're actually responding — see [05-Troubleshooting.md](05-Troubleshooting.md)) |
| `Display and touch controller initialized.` | Display SPI and touch I2C setup completed |
| `MANUAL PWM TEST MODE: type a duty cycle 0-100 and press Enter.` | Only appears if `MANUAL_PWM_TEST_MODE` is `true` |

## 4.2 Recurring Output (every ~1 second, from `loop()`)

```
Solar: 11.98 V, 0.51 A, 6.11 W
Wind: 0.02 V, 0.00 A, 0.00 W
Common Bus: 11.85 V, 0.53 A, 6.28 W
Battery: 13.85 V, 0.97 A, 13.44 W | Duty: 42.50 %
Load (placeholder): 0.00 V, 0.00 A, 0.00 W
Efficiency: In=6.28W, Out=5.42W, Eff=86.31%
```

| Field | Meaning |
|---|---|
| `Solar:` line | Solar array bus voltage / current / power |
| `Wind:` line | Wind branch bus voltage / current / power |
| `Common Bus:` line | Voltage/current/power right after both isolation diodes combine, before the boost converter's input |
| `Battery:` line | Boost converter output (battery-side) voltage / current / power |
| `Duty:` | Current commanded PWM duty cycle (0–80% in automatic mode, whatever you last typed in manual mode) |
| `\| FAULT -- output disabled, reset required` | Only appears if the controller has latched into a fault state — see 4.4 below |
| `Load (placeholder):` line | **Ignore these numbers.** No buck converter exists yet, so this sensor has nothing real to measure — see [01-Wiring-and-Assembly.md](01-Wiring-and-Assembly.md) §1.1. Kept in the output so the print pattern is already in place once the buck converter is built. |
| `Efficiency:` line | `In` = common DC bus power (the boost converter's true input), `Out` = battery power, `Eff` = Out/In as a percentage. Shows 0% whenever input is below 0.1W, to avoid a meaningless noisy ratio near zero. |

## 4.3 Manual Mode Confirmation

```
Manual duty cycle set to: 25.00 %
```

Printed every time you type a new value in manual mode. If you don't see this after typing a number and pressing Enter, check that Serial Monitor's line ending is set to send a newline (usually "Newline" or "Both NL & CR" in the dropdown next to the baud rate).

## 4.4 Fault Messages

```
FAULT: battery sensor not responding -- output disabled.
```
**Automatic mode only.** The battery INA226 didn't acknowledge on I2C. Check wiring/address before resetting.

```
FAULT: duty cycle pinned at ceiling without reaching CC target -- possible open circuit. Output disabled.
```
**Automatic mode only.** Duty cycle sat at 80% for 3+ seconds without current reaching 1.0A. Usually means nothing is actually connected to charge, or the battery is disconnected.

```
FAULT: hard safety limit exceeded (V=15.23V, I=1.62A) -- output disabled.
```
**Both modes.** Battery voltage exceeded 15.0V or current exceeded 1.5A — an absolute limit independent of the CC/CV targets or which mode is active. This is the only fault that can trigger during **manual PWM test mode**, since manual mode has no other protection. If you see this while manually sweeping duty cycle values, the value you typed pushed voltage/current past a safe limit.

All faults **latch** — the `Battery:` line will keep showing `| FAULT` every second, and (in manual mode) any further typed duty cycle values will be refused with an `Ignored: system is in a FAULT state` message, until you power-cycle or reset the board. See [05-Troubleshooting.md](05-Troubleshooting.md) for next steps.
