# Boost Converter Power-Stage Test

This is the first test of the actual power stage: GaN switch, inductor, diode, and capacitors, with real voltage step-up on the output. Fill in the `[FILL IN]` spots tomorrow with your actual bench setup, then work through the test in order.

**Why this test is different from the Bench Tests folder:** `Bench Tests/05-CC-CV-and-Efficiency.md` assumes the full firmware is running, with the fault latch and hard voltage/current cutoffs active. That firmware doesn't run on the Giga yet, because `BoostConverter.cpp` hasn't been ported from the Mega's Timer1 registers. Tomorrow's test uses the bare `PWM_Giga_Test` sketch instead, which only sets a duty cycle you type in — **nothing in software is watching voltage or current**. Every safety check here is manual.

## 1. What You Need

| Item | Status |
|---|---|
| Assembled power stage (GaN switch, inductor, diode, caps) | Ready |
| Load resistor | Ready — value/wattage: `[FILL IN]` |
| Current-limited bench supply | `[FILL IN: confirm available]` |
| Oscilloscope | `[FILL IN: confirm available]` |
| Multimeter (DC volts and current) | `[FILL IN: confirm available]` |
| Giga R1 WiFi with `PWM_Giga_Test` uploaded | Ready (confirmed 100 kHz on pin 11) |

## 2. Numbers to Fill In Before Powering Anything

Compute these from your actual resistor before starting:

| Value | How to get it | Your number |
|---|---|---|
| Load resistance (Ω) | Read off the resistor | `[FILL IN]` |
| Load power rating (W) | Read off the resistor | `[FILL IN]` |
| Planned input voltage (V) | What the bench supply will be set to | `[FILL IN]` |
| Max safe output voltage for this resistor | V = sqrt(rated_W × R) | `[FILL IN]` |
| Max safe output current for this resistor | I = sqrt(rated_W / R) | `[FILL IN]` |
| **Hard stop voltage** (whichever is lower: 15.0 V project limit, or the resistor's max safe voltage above) | | `[FILL IN]` |
| **Hard stop current** (whichever is lower: 1.5 A project limit, or the resistor's max safe current above) | | `[FILL IN]` |

Don't skip this. If the resistor's rating is lower than the project's own 15.0 V / 1.5 A limits, the resistor is the real limit for tomorrow, not the firmware numbers.

## 3. Pre-Power Checks

With everything unpowered, multimeter in continuity mode:

| Check | Expected |
|---|---|
| GaN switch orientation matches the schematic | `[FILL IN: confirm]` |
| Diode orientation (cathode toward output) | `[FILL IN: confirm]` |
| Inductor correctly in the switching path | `[FILL IN: confirm]` |
| No shorts: input+ to input−, output+ to output−, switch node to ground | Open on all |
| Gate driver (TC4427) output wired to the GaN gate, not directly to the Giga pin | `[FILL IN: confirm]` |
| Load resistor connected across the output **before** power-up | Connected |

## 4. Power-Up and Duty Ramp

**Rule: never jump duty cycle. Always step up gradually and watch the meter/scope between each step.**

1. Set the bench supply to the planned input voltage, **current limit set low** (start around 0.2–0.3 A).
2. Power on with duty cycle at **0%** (the sketch starts here).
3. Confirm input voltage reads correctly with no current flowing (no switching yet).
4. Type `5` in the Serial Monitor. Wait a few seconds. Check:
   - Output voltage on the multimeter or scope
   - Output current (or compute from V/R across the load)
   - Any heat, smell, or noise from the inductor or switch
5. If everything looks normal, increase by small steps (5, 10, 15, 20...). Stop and record at each step.
6. **Stop immediately and set duty back to 0 if:**
   - Output voltage reaches your hard stop voltage (§2)
   - Output current reaches your hard stop current (§2)
   - Anything gets hot, smells, or makes noise
   - The input current limit trips repeatedly

## 5. Results Log

| Duty typed | Input V | Input I | Output V | Output I | Notes |
|---|---|---|---|---|---|
| 0 | | | | | |
| 5 | | | | | |
| 10 | | | | | |
| 15 | | | | | |
| 20 | | | | | |
| | | | | | |

## 6. After the Test

1. Set duty to 0, then power off the supply before disconnecting anything.
2. Compare measured output voltage at each duty cycle against the ideal boost formula: **V_out = V_in / (1 − D)**. Large differences suggest a wiring issue or a component problem, not just efficiency loss.
3. Note the highest duty cycle reached safely, and why you stopped (hit a limit, or ran out of planned steps).
4. Copy the key results into `Bench Tests/06-Test-Log.md` once done, so everything stays in one place.

## 7. Known Open Items Going Into This Test

- `BoostConverter.cpp` is not yet ported to the Giga, so this test uses manual PWM only, not the automatic CC/CV firmware.
- The duty-cycle step resolution on the Giga at 100 kHz is still unconfirmed (two earlier readings suggested ~10% steps). Expect coarse control, not fine 1% adjustments.
- No automatic fault protection is active during this test. You are the fault protection.
