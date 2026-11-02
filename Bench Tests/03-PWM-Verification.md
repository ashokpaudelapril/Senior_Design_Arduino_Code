# 3. PWM Verification (Gate Drive, Pin 11)

This verifies the 100 kHz gate-drive signal and that typed duty-cycle values produce the expected output. Use **manual test mode** (`MANUAL_PWM_TEST_MODE = true` in `Config.h`).

**Do not connect the GaN switch or the TC4427 gate-driver output for this test unless you've confirmed the scope reading first.** Verify the PWM signal at the board pin alone, then connect the gate driver.

## 3.1 Setup

1. Connect the scope probe (10× setting) to **pin 11**, and the ground clip to a **GND** pin. Don't use pin 10 for ground.
2. Set the scope: 20 µs/div time base, 2 V/div, trigger on rising edge of channel 1.
3. Confirm the Serial Monitor shows `MANUAL PWM TEST MODE` and that you can type a value and get `Manual duty cycle set to:`.

## 3.2 Expected Duty Cycles

`BoostConverter.cpp` drives STM32's TIM8 directly at a measured 240MHz kernel clock, with 2400 steps per period (~0.042% resolution per step — about 15× finer than the Mega's original Timer1 design). At this resolution, the achieved duty matches the typed value almost exactly for any reasonable test value — unlike the Mega, where the 160-step resolution visibly rounded values down.

| You type | Expected output duty | Expected period | Expected frequency |
|---|---|---|---|
| 0 | 0% | 10 µs | 100 kHz |
| 25 | 25.0% | 10 µs | 100 kHz |
| 50 | 50.0% | 10 µs | 100 kHz |
| 80 | 80.0% | 10 µs | 100 kHz |
| 100 | 100.0% | 10 µs | 100 kHz |

Note at 100%: unlike the Mega version (which topped out at 99.4% due to its timer's structure), TIM8's compare value at 100% exceeds its count range entirely, so the output is genuinely 100% with no gap.

**Proposed tolerance:** frequency ±1% (99–101 kHz); duty within ±0.5 percentage points of the expected value.

## 3.3 Procedure

For each row in §3.2:

1. Type the value in the Serial Monitor and press Enter.
2. Confirm the `Manual duty cycle set to:` echo.
3. On the scope, measure the period (frequency) and the high-time as a percentage of the period. Use the scope's automatic measurements if available.
4. Capture a screenshot or photo of the waveform for the log.

| Typed | Measured frequency (kHz) | Measured duty (%) | Expected duty (%) | PASS/FAIL |
|---|---|---|---|---|
| 0 | | | 0 | |
| 25 | | | 25.0 | |
| 50 | | | 50.0 | |
| 80 | | | 80.0 | |
| 100 | | | 100.0 | |

## 3.4 Resolution Check

This test now confirms **fine** resolution, the opposite of what it checked on the Mega. Type `35`, then `55`, then `58` — these specific values were chosen because they previously exposed the old Mbed `PwmOut` rounding (which snapped everything to multiples of 10%). You should see each one land at its own correct value, not rounded to a nearby 10% mark.

Note: at 0.042% per step, two values only 0.1% apart (e.g. `35.0` vs `35.1`) may *not* look visibly different on the scope — the real tick-level difference is only a few nanoseconds of pulse width, likely below what the scope's display or your eye can resolve at normal time scales. That's expected and fine; it doesn't mean the resolution isn't there, just that this scope can't visually confirm differences that small.

## 3.5 Clamp Check

Type `-10` and then `150`.

| Typed | Expected behavior | Result |
|---|---|---|
| -10 | Clamped to 0%, output 0% | |
| 150 | Clamped to 100%, output 100% | |

## 3.6 Caution: Manual Mode Has No 80% Ceiling

In automatic mode, duty cycle never exceeds 80%. Manual mode allows 0–100%. Don't sweep above 50% without the battery sensor wired and the bench supply current-limited, so the hard voltage and current limits can catch an overshoot. See [04-Fault-and-Limit-Tests.md](04-Fault-and-Limit-Tests.md).

## 3.7 Results

**Section result:** PASS / FAIL / BLOCKED

If the frequency is wrong (not ~100 kHz), check that `TIM8_CLOCK_HZ` in `Config.h` still reads `240000000UL` and that the board was re-uploaded after any edit. That value was measured on real hardware (see `HAL_Access_Test.ino`), not calculated from a datasheet assumption — if your specific board reports a different APB2 clock or prescaler setting, this constant would need updating to match. If the duty steps don't match the table, check the typed value is the one echoed.
