# 4. Fault and Limit Tests

These tests deliberately trigger each fault and confirm three things: the fault appears with the right message, the output is forced to 0%, and the fault stays latched until a reset.

**Run these with the battery sensor wired.** The hard limit reads the battery sensor. Without it, the hard limit can't see an overvoltage.

Use a current-limited bench supply for every test. Set a current limit before connecting anything.

## 4.1 Hard Voltage Limit (both modes)

Goal: battery-sensor voltage above 15.0 V trips the hard limit.

**Setup:**
1. Set automatic mode (`MANUAL_PWM_TEST_MODE = false`), upload, and open the Serial Monitor.
2. Disconnect the boost input so no power flows through the boost stage.
3. Connect the bench supply, set to **15.5 V, 0.2 A limit**, across the **battery sensor bus terminals**.

**Expected:** within about 50 ms, the output shows
```
FAULT: hard safety limit exceeded (V=15.5V, I=...A) -- output disabled.
```
and the `Battery:` line shows `| FAULT -- output disabled, reset required` on the next print.

| Step | Expected | Result |
|---|---|---|
| Fault message appears | Yes, with V ≈ 15.5 | |
| Duty cycle reads 0% | Yes | |
| Fault persists after supply is lowered to 12 V | Yes (latched) | |
| Reset clears the fault | Yes, after reset, Battery line has no FAULT | |

Turn the supply off before resetting.

## 4.2 Hard Current Limit (both modes) — Needs a High-Power Load

Goal: current above 1.5 A trips the hard limit.

**Requires a power resistor rated at least 25 W** (e.g. 7.5 Ω at 12 V dissipates 19 W). If you don't have one, mark this section **BLOCKED** and note it in the log. The voltage path in §4.1 is the same comparison code path, so this is a lower-priority check.

If you do have the resistor: connect it across the battery sensor bus with the supply at 12 V. Confirm the fault message shows `I=` above 1.5 A and duty reads 0%.

## 4.3 Sensor Missing (automatic mode only)

Goal: the battery sensor not responding triggers the fault.

1. Automatic mode, battery sensor connected and reading, boost off (duty 0%).
2. Power down, disconnect the battery sensor's **SDA** wire, and power up.
3. Expected within a second: `FAULT: battery sensor not responding -- output disabled.`

| Step | Expected | Result |
|---|---|---|
| Fault message appears | Yes | |
| Duty cycle 0% | Yes | |
| Reconnect SDA, reset | Fault clears, readings return | |

## 4.4 Open Circuit / Pinned at Ceiling (automatic mode only)

Goal: duty pinned at 80% for 3 s without reaching the 1.0 A CC target triggers the open-circuit fault.

**Setup:** automatic mode, battery sensor wired, **no load and no battery** on the boost output. Bench supply on the common bus input at about 12 V, current-limited to **0.5 A**.

**Expected timeline:**
- Duty steps up 0.5% every 50 ms, so reaching the 80% ceiling takes about 8 seconds.
- After 3 more seconds pinned at 80%, the fault appears.
- Total: roughly 11 seconds after power-up.

**Acceptable alternate outcome:** the hard voltage limit can trip before the open-circuit timeout if the output voltage climbs above 15 V. Either fault is correct behavior. Record which one appeared.

| Step | Expected | Result |
|---|---|---|
| Duty climbs toward 80% | Yes, watch the Duty field | |
| Fault message | `FAULT: duty cycle pinned at ceiling ... possible open circuit` OR hard-limit message | |
| Output forced to 0% | Yes | |

## 4.5 Manual Mode: Input Refused After Fault

1. Trigger any fault in manual mode (e.g. the §4.1 supply test, with `MANUAL_PWM_TEST_MODE = true`).
2. Type `25` and press Enter.

**Expected:** `Ignored: system is in a FAULT state. Reset the board to clear it before setting duty cycle.` The duty cycle does not change.

| Step | Expected | Result |
|---|---|---|
| Typed value refused | Yes | |
| Duty stays at 0% | Yes | |

## 4.6 Reset Clears Fault

1. Remove any test wiring that caused the fault, and confirm the supply is off or at a safe level.
2. Press the Mega reset button, or power-cycle.
3. Expected: the startup messages print, and the Battery line has no `| FAULT`.

## 4.7 Results

| Test | Result | Which fault appeared / notes |
|---|---|---|
| 4.1 Hard voltage | | |
| 4.2 Hard current | | |
| 4.3 Sensor missing | | |
| 4.4 Open circuit | | |
| 4.5 Manual refuses input | | |
| 4.6 Reset clears | | |

**Section result:** PASS / FAIL / BLOCKED
