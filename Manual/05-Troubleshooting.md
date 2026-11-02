# 5. Troubleshooting

## 5.1 A Fault Has Latched

**Symptom:** Serial output shows `| FAULT -- output disabled, reset required` every second, duty cycle stuck at 0%. In manual mode, typed duty cycle values are refused with `Ignored: system is in a FAULT state`.

**This is deliberate** — the controller will not auto-retry, since a fault that clears itself could ramp right back into the same hazard. To clear it:

1. Figure out *why* it faulted first (don't just reset blindly):
   - `FAULT: battery sensor not responding` → check the battery INA226's wiring and I2C address (`0x41`, A0 jumper to VS) — **automatic mode only**
   - `FAULT: duty cycle pinned at ceiling...` → check that something is actually connected to the boost converter's output (battery or load) — an open circuit is the expected trigger for this fault — **automatic mode only**
   - `FAULT: hard safety limit exceeded...` → voltage went above 15.0V or current above 1.5A. **Can happen in either mode** — in manual mode, this usually means the duty cycle you typed was too high for the current input conditions; try a lower value after resetting
2. Fix the underlying issue
3. Press the Giga's reset button, or power-cycle the board — this re-runs `setup()`, which calls `chargeController_init()` and clears the fault (this clears the fault regardless of which of the three checks tripped it, since they all share one fault flag)

## 5.2 Sensor Readings Look Like 0.00 V / 0.00 A

**Likely cause:** that sensor isn't responding on I2C (wrong address, not wired, not powered).

**Check:**
- Solar: address `0x40` (A0 = GND, A1 = GND — factory default)
- Battery: address `0x41` (A0 = VS)
- Wind: address `0x42` (A0 = SDA)
- Common Bus: address `0x43` (A0 = SCL)
- Verify with a multimeter that each sensor board has power (VCC/GND) before checking I2C
- Two sensors at the same address will collide on the bus — double check jumpers if two sensors both read identically

## 5.3 PWM Signal Looks Wrong on the Oscilloscope

**Not ~100kHz:** Confirm you're probing pin 11. `BoostConverter.cpp` drives TIM8 Channel 2 directly via STM32 HAL at 240MHz with a 2400-tick period, which works out to exactly 100kHz. If the frequency is off, check that `TIM8_CLOCK_HZ` in `Config.h` still reads `240000000UL` — that value was measured on real hardware, not assumed, so don't "correct" it without re-measuring (see `HAL_Access_Test.ino`).

**Duty cycle doesn't seem to move smoothly:** This should no longer happen. Mbed's standard `PwmOut` API (tried first) was confirmed by scope measurement to only give ~10% duty steps at 100kHz, regardless of entry point — that's why `BoostConverter.cpp` bypasses it and drives TIM8 directly instead, giving 2400 steps per period (~0.042% resolution). If you still see coarse, jumpy duty cycle behavior, that's a regression worth investigating, not expected behavior.

**No signal at all:** Confirm `boost_init()` actually ran (check for it being called in `setup()`), and that you're not in a latched fault state forcing 0% duty (0% duty may still show a signal at 0V constant, not "no signal" — if you see a flat line at 0V with no switching, that's 0% duty working correctly, not a failure).

## 5.4 Display Shows Nothing / Garbage

- Confirm all SPI pins are wired, **especially LCD_RST** — per user reports on this exact panel, a disconnected reset pin commonly produces a blank white screen
- Confirm the libraries listed in [02-Firmware-Setup.md](02-Firmware-Setup.md) are installed, particularly `Adafruit_ST7796S_kbv` and `Adafruit GFX Library` together
- If colors appear inverted, try adding `tft.invertDisplay(true);` after `tft.begin();` in `Display.cpp` — several reviewers of this exact panel needed this

## 5.5 Manual Test Mode Doesn't Respond to Typed Input

- Confirm `MANUAL_PWM_TEST_MODE` is `true` in `Config.h` and the sketch was re-uploaded after changing it
- Confirm Serial Monitor's baud rate is set to 9600
- Confirm the line-ending dropdown is set to send a newline character (not "No line ending") — `Serial.parseFloat()` needs the Enter keypress to know input is complete

## 5.6 "It Was Working, Now It's Not" After an Edit

Check `Config.h` first — most behavior changes in this firmware (targets, pins, timing, mode) live there. A misplaced edit to a constant (wrong pin number, wrong address, `MANUAL_PWM_TEST_MODE` left in the wrong state) is the most common cause of a sudden regression.
