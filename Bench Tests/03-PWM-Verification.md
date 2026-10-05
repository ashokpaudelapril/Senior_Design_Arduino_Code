# 3. PWM Verification (Gate Drive, Pin 11)

This verifies the 100 kHz gate-drive signal and that typed duty-cycle values produce the expected output. Use **manual test mode** (`MANUAL_PWM_TEST_MODE = true` in `Config.h`).

**Do not connect the GaN switch or the TC4427 gate-driver output for this test unless you've confirmed the scope reading first.** Verify the PWM signal at the board pin alone, then connect the gate driver.

## 3.1 Setup

1. Connect the scope probe (10× setting) to **pin 11**, and the ground clip to a **GND** pin. Don't use pin 10 for ground.
2. Set the scope: 20 µs/div time base, 2 V/div, trigger on rising edge of channel 1.
3. Confirm the Serial Monitor shows `MANUAL PWM TEST MODE` and that you can type a value and get `Manual duty cycle set to:`.

## 3.2 Expected Duty Cycles

Timer1 runs with 160 steps per period (TOP = 159), so the achieved duty is the integer step below, **not the exact number you typed**. The firmware computes it as `floor(D/100 × 159) / 160`.

| You type | Expected output duty | Expected period | Expected frequency |
|---|---|---|---|
| 0 | ~0% (see note) | 10 µs | 100 kHz |
| 25 | 24.4% | 10 µs | 100 kHz |
| 50 | 49.4% | 10 µs | 100 kHz |
| 80 | 79.4% | 10 µs | 100 kHz |
| 100 | 99.4% | 10 µs | 100 kHz |

Note at 0%: the output should be low except for a very short pulse at the start of the period, on the order of tens of nanoseconds. Treat this as 0% for test purposes.

Note at 100%: the output reaches 159/160 of the period, not 100%. The remaining 0.6% is the timer's structure, not a bug.

**Proposed tolerance:** frequency ±1% (99–101 kHz); duty within ±0.5 percentage points of the expected value.

## 3.3 Procedure

For each row in §3.2:

1. Type the value in the Serial Monitor and press Enter.
2. Confirm the `Manual duty cycle set to:` echo.
3. On the scope, measure the period (frequency) and the high-time as a percentage of the period. Use the scope's automatic measurements if available.
4. Capture a screenshot or photo of the waveform for the log.

| Typed | Measured frequency (kHz) | Measured duty (%) | Expected duty (%) | PASS/FAIL |
|---|---|---|---|---|
| 0 | | | ~0 | |
| 25 | | | 24.4 | |
| 50 | | | 49.4 | |
| 80 | | | 79.4 | |
| 100 | | | 99.4 | |

## 3.4 Resolution Check

Type `25.5`, then `25.6`, then `25.7`. These should land on the same or adjacent steps (each step is 0.625%), so you should see the same or near-identical duty. This confirms the coarse resolution is real hardware behavior and not a firmware bug.

## 3.5 Clamp Check

Type `-10` and then `150`.

| Typed | Expected behavior | Result |
|---|---|---|
| -10 | Clamped to 0%, output ~0% | |
| 150 | Clamped to 100% (output 99.4%) | |

## 3.6 Caution: Manual Mode Has No 80% Ceiling

In automatic mode, duty cycle never exceeds 80%. Manual mode allows 0–100%. Don't sweep above 50% without the battery sensor wired and the bench supply current-limited, so the hard voltage and current limits can catch an overshoot. See [04-Fault-and-Limit-Tests.md](04-Fault-and-Limit-Tests.md).

## 3.7 Results

**Section result:** PASS / FAIL / BLOCKED

If the frequency is wrong (not ~100 kHz), check that `PWM_FREQUENCY_HZ` is `100000UL` in `Config.h` and that the board was re-uploaded after any edit. If the duty steps don't match the table, check the typed value is the one echoed.
