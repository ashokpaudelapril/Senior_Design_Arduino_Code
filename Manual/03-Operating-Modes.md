# 3. Operating Modes

The firmware has two mutually-exclusive modes, controlled by one line in `Config.h`:

```cpp
#define MANUAL_PWM_TEST_MODE true    // or false
```

This is a **compile-time** switch — changing it requires re-uploading the sketch, not just resetting the board.

## 3.1 Manual PWM Test Mode (`true`)

**Purpose:** Bench-test the boost converter's PWM output in isolation, without needing the battery sensor wired up or trusting the CC/CV algorithm.

**How to use:**
1. Upload with `MANUAL_PWM_TEST_MODE` set to `true`
2. Open Serial Monitor at 9600 baud — you'll see:
   ```
   MANUAL PWM TEST MODE: type a duty cycle 0-100 and press Enter.
   ```
3. Type a number (e.g. `25`) and press Enter
4. The gate drive pin (11) immediately switches to that duty cycle at ~100kHz
5. Confirmation prints back: `Manual duty cycle set to: 25.00 %`

**What's disabled in this mode:** `ChargeController`'s automatic CC/CV loop never runs. Sensor readings (solar/wind/battery) still print every second, but nothing acts on them — duty cycle only changes when you type a new value.

**Use this mode when:** verifying PWM frequency/duty cycle on an oscilloscope, or testing the gate driver and GaN switch hardware before trusting the full closed loop.

## 3.2 Automatic Mode (`false`)

**Purpose:** Real closed-loop CC/CV battery charging.

**Behavior:** `ChargeController` reads the battery sensor every 50ms and adjusts duty cycle to chase two targets:
- **Constant Current (CC):** 1.0 A into the battery, while voltage is below the cap
- **Constant Voltage (CV):** 14.4 V cap, once reached — current tapers as duty cycle backs off

Duty cycle is capped at 80% as a safety ceiling, separate from the hardware's own 0–100% clamp.

**Fault protection:** if the battery sensor stops responding, or duty cycle sits pinned at 80% for more than 3 seconds without current reaching the 1.0A target (an open-circuit signature), the controller **latches into a fault state** — output forced to 0%, requiring a power cycle or reset to clear. See [05-Troubleshooting.md](05-Troubleshooting.md) for what to do if this happens.

**Use this mode when:** actually charging the battery from the solar/wind array, or once the manual-mode PWM test has confirmed the hardware works correctly.

## 3.3 Switching Between Modes

| From | To | Steps |
|---|---|---|
| Manual → Automatic | Edit `Config.h`: `MANUAL_PWM_TEST_MODE` → `false`, re-upload | Confirm the battery sensor is wired and responding first |
| Automatic → Manual | Edit `Config.h`: `MANUAL_PWM_TEST_MODE` → `true`, re-upload | Safe to do anytime — resets duty cycle to 0% on boot either way |

There is currently no way to switch modes without re-uploading (e.g. no Serial command or physical switch) — this is a deliberate simplification, not an oversight.
