# Bench Test Procedures — UWBG Power Converter

These procedures walk through verifying the firmware and hardware on the bench, in order. Run them in sequence: each step assumes the ones before it passed.

**Status:** Proposed procedures written before hardware was available. Treat every expected value as a starting point to confirm, and correct the document if a real result says otherwise.

## Test Order

1. [01-Safety-and-Setup.md](01-Safety-and-Setup.md) — power-off checks and first power-up
2. [02-Sensor-Verification.md](02-Sensor-Verification.md) — each INA226 against a multimeter
3. [03-PWM-Verification.md](03-PWM-Verification.md) — the 100 kHz gate-drive signal on an oscilloscope
4. [04-Fault-and-Limit-Tests.md](04-Fault-and-Limit-Tests.md) — deliberately trigger each fault and confirm it latches and clears
5. [05-CC-CV-and-Efficiency.md](05-CC-CV-and-Efficiency.md) — closed-loop charging and efficiency check
6. [06-Test-Log.md](06-Test-Log.md) — record of every run, with results

## Equipment

- Arduino GIGA R1 WiFi with the firmware uploaded (the Mega 2560 is no longer the target board)
- USB cable and laptop running the Arduino IDE Serial Monitor (9600 baud)
- Digital multimeter (voltage and current/DC mA range)
- Oscilloscope with at least two channels, and a 10× probe
- Bench power supply, current-limited, 0–20 V (for the sensors and charging tests)
- Known resistive load(s) for current tests (e.g. 10 Ω, 5 W, and a 1 kΩ power resistor)
- Battery pack (KBT LiFePO4 12.8 V 5 Ah) — only for tests in 05
- Insulated tools and a current-limited supply for every powered test

## Rules for Every Test

- **Always start with the power supply at its lowest voltage and current limit, then raise slowly.**
- **Keep the battery sensor wired for any test that sets duty cycle above 0%.** The hard voltage/current limit reads the battery sensor. If that sensor is disconnected, the hard limit cannot protect the circuit.
- **Don't touch the power stage (inductor, GaN switch, diodes) while the boost is switching.**
- **Record every number** in [06-Test-Log.md](06-Test-Log.md), including failures.
- If a result is unexpected, stop, power down, and record it before changing anything.

## Pass / Fail Convention

- **PASS:** measured value is within the tolerance listed in that test.
- **FAIL:** outside tolerance. Log the value, stop, and troubleshoot using [../Manual/05-Troubleshooting.md](../Manual/05-Troubleshooting.md).
- **BLOCKED:** can't run the test yet (missing part, or prior test failed). Say which.

Tolerances in these documents are proposed. Confirm them with your instructor or team before using them as acceptance criteria in the report.
