# 5. CC/CV Charging and Efficiency

This is the first full closed-loop test. Run it only after §2 (sensors), §3 (PWM), and §4 (faults) have passed.

**Proposed tolerances:**
- CC phase: battery current within ±0.1 A of 1.0 A
- CV phase: battery voltage not above 14.6 V (the hard limit is 15.0 V; 14.4 V is the target)
- Efficiency: Serial value within ±3 percentage points of the hand calculation in §5.4

## 5.1 Setup

1. **Automatic mode** (`MANUAL_PWM_TEST_MODE = false`). Upload and open the Serial Monitor.
2. Connect the battery pack (KBT LiFePO4, 12.8 V nominal) to the battery output through the battery sensor. Record its open-circuit voltage before connecting.
3. Connect the bench supply to the common bus input, set to **12.0 V, 1.5 A limit** to stand in for the solar array (≈12 V, 1 A).
4. Keep the power resistor disconnected.

**Note:** a 12 V input boosted to 14.4 V needs only about 17% duty, so the CC phase may be short or the battery may start near its CV limit. If the battery is already near 14.4 V at the start, the test is effectively CV-only. Record the starting state of charge.

## 5.2 CC Phase

Watch the `Battery:` line every second. Record the first 10 readings.

| Time | Battery V | Battery I (A) | Duty (%) | Notes |
|---|---|---|---|---|
| 0 s | | | | |
| 10 s | | | | |
| 20 s | | | | |
| 30 s | | | | |
| 60 s | | | | |

**Expected:**
- Duty rises from 0% and current rises toward 1.0 A.
- Once current reaches about 1.0 A, duty holds steady or eases slightly.
- The step controller should not oscillate wildly. Small ±0.5% wiggles in duty are normal.

## 5.3 CV Phase

Continue until battery voltage reaches the 14.4 V cap, then watch for another 5 minutes or until current drops below about 0.1 A.

**Expected:**
- Voltage holds near 14.4 V.
- Current tapers as the battery fills.
- Duty steps down when voltage is at or above the cap.

| Check | Expected | Result |
|---|---|---|
| Peak battery voltage | ≤ 14.6 V | |
| Peak battery current | ≤ 1.1 A | |
| No FAULT appears | Yes | |
| Duty stays within 0–80% | Yes | |

Record the time from start to the first CV-phase step, and the final current when you stop the test.

## 5.4 Efficiency Check

The Serial `Efficiency:` line reports In (common bus power), Out (battery power), and Eff. Check it against an independent hand calculation.

1. At one steady moment in CC phase, record the common bus voltage and current from the Serial output.
2. Measure the same common bus voltage and current with the multimeter (current in series with the bus, DC current range). Use the meter's reading for the hand calculation.
3. Compute P_in = V_in × I_in and P_out = V_bat × I_bat, using the multimeter values.
4. Compute Eff = P_out / P_in × 100.

| Source | P_in (W) | P_out (W) | Eff (%) |
|---|---|---|---|
| Serial `Efficiency:` line | | | |
| Hand calc from multimeter | | | |
| Difference (percentage points) | | | |

**Expected:** within ±3 points. Efficiency reads 0% when input is below 0.1 W. That's correct behavior, not a fault.

## 5.5 Notes for Tuning

If the duty cycle oscillates or the battery voltage overshoots:

- The step size (`CHARGE_DUTY_STEP_PERCENT`, 0.5%) and control rate (`CONTROL_LOOP_INTERVAL_MS`, 50 ms) are untested guesses. Record what you see and propose changes in the log. Don't tune them mid-test; change one value, re-upload, and repeat the run.
- The achieved duty steps in 0.625% increments (see [03-PWM-Verification.md](03-PWM-Verification.md) §3.2). A 0.5% step sometimes produces no visible change, which can look like the controller is ignoring feedback. That's expected.

## 5.6 Results

**Section result:** PASS / FAIL / BLOCKED

After the test, power down the supply first, then disconnect the battery.
