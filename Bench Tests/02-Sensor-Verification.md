# 2. Sensor Verification

Each INA226 is checked against the multimeter. Do one sensor at a time, in this order: solar, battery, wind, common bus.

**Proposed tolerance:** voltage ±1% or ±0.05 V (whichever is larger); current ±2% or ±5 mA (whichever is larger); power derived from the two, so it isn't tested separately.

## 2.1 Voltage Check (no current)

For each sensor board:

1. Connect a known voltage source (bench supply or battery) across the sensor's **bus** terminals. Set it to 12.00 V, current limit 0.5 A.
2. Measure the same voltage at the same terminals with the multimeter in DC volts.
3. Compare to the Serial value for that sensor (e.g. `Solar: 12.01 V, ...`).

| Sensor | Multimeter (V) | Serial reading (V) | Difference | PASS/FAIL |
|---|---|---|---|---|
| Solar | | | | |
| Battery | | | | |
| Wind | | | | |
| Common bus | | | | |

**If a reading is 0.00 V:** the sensor isn't responding. Check its address (§1.2) and that VCC/GND are powered.

**If two sensors show identical readings:** two boards have the same address. Fix the jumpers before continuing.

## 2.2 Current Check (known load)

1. Put a known resistor across the sensor's output, in series with the bus supply, so the current flows through the sensor's shunt.
2. Set the supply to 12.00 V. Calculate the expected current: I = V / R. Use a **1 kΩ** load (12 mA, 0.14 W) for these checks. Don't use low-resistance loads here: a 10 Ω load at 12 V draws 1.2 A and dissipates 14.4 W, well beyond a standard resistor, and pushes the circuit toward the 1.5 A hard limit.
3. Measure the current with the multimeter in series, using the mA or A range.
4. Compare to the Serial current value.

| Sensor | Load (Ω) | Expected I (A) | Multimeter I (A) | Serial I (A) | PASS/FAIL |
|---|---|---|---|---|---|
| Solar | 1000 | 0.0120 | | | |
| Battery | 1000 | 0.0120 | | | |
| Wind | 1000 | 0.0120 | | | |
| Common bus | 1000 | 0.0120 | | | |

A higher-current check (around 0.5 A) is covered in [05-CC-CV-and-Efficiency.md](05-CC-CV-and-Efficiency.md), once the power resistor and supply limits are chosen.

**If current reads consistently off by a fixed factor** (e.g. 2× or 0.5×): check `INA226_SHUNT_OHMS` (0.010 Ω) and the shunt value on the board. A wrong shunt value scales every current and power reading.

## 2.3 Battery Sensor Presence

This tests the function that tells "sensor missing" from "sensor reading zero," which the automatic charging loop relies on.

1. With the battery sensor wired and powered, the Serial output should not show any fault from this check.
2. Disconnect the battery sensor's SDA wire, power-cycle the board, then (in automatic mode, see [05-CC-CV-and-Efficiency.md](05-CC-CV-and-Efficiency.md)) observe the fault message.

| Condition | Expected | Result |
|---|---|---|
| Battery sensor connected | No "not responding" fault | |
| Battery sensor SDA disconnected | `FAULT: battery sensor not responding` (automatic mode only) | |

Reconnect the wire and reset before moving on.

## 2.4 Results

**Section result:** PASS / FAIL / BLOCKED
