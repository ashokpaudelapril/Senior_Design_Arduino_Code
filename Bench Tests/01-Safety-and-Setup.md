# 1. Safety and Setup

## 1.1 Power-Off Checks (before any supply is connected)

Use the multimeter in continuity/resistance mode with everything unpowered.

| # | Check | Expected | Result |
|---|---|---|---|
| 1 | Mega 5V pin to GND | Open (no short) | |
| 2 | Battery+ to battery− | Open (no short) | |
| 3 | Gate-drive pin 11 to GND | Open or high Ω | |
| 4 | Sensor VCC to GND, each INA226 board | Open (no short) | |
| 5 | Display VCC to GND | Open (no short) | |
| 6 | Common ground: Mega GND, sensor GND, display GND, boost GND | Continuity between all | |
| 7 | Wind-drive battery (12 V Li-ion) to renewable/charging circuit | **Open** (must be isolated) | |
| 8 | Each SB540 diode: correct orientation (cathode toward the common bus) | Matches schematic | |

**Stop if any short is found.** Fix it before powering up.

## 1.2 Jumper and Address Check

Confirm each INA226 board's A0/A1 jumpers match [../Manual/01-Wiring-and-Assembly.md](../Manual/01-Wiring-and-Assembly.md) §1.1:

| Sensor | Expected address | A0 | A1 | Confirmed |
|---|---|---|---|---|
| Solar | 0x40 | GND | GND | |
| Battery | 0x41 | VS | GND | |
| Wind | 0x42 | SDA | GND | |
| Common bus | 0x43 | SCL | GND | |

Don't check the load sensor (0x44) yet. It's a placeholder with no circuit to measure.

## 1.3 First Power-Up

1. Set the bench supply to **5.0 V, 0.5 A limit**, and connect it to the Mega's 5 V input (or via USB only, for the first power-up).
2. Power on. Watch for smoke, heat, or unusual current draw. Stop immediately if any appear.
3. Open the Serial Monitor at **9600 baud**.
4. Expected startup lines, in order:
   ```
   Solar, battery, wind, common bus, and load INA226 sensors initialized.
   Display and touch controller initialized.
   ```
   If `MANUAL_PWM_TEST_MODE` is `true`, you'll also see:
   ```
   MANUAL PWM TEST MODE: type a duty cycle 0-100 and press Enter.
   ```
5. Within about 1 second, you should see the recurring output lines (Solar, Wind, Common Bus, Battery, Load placeholder, Efficiency).

| Check | Expected | Result |
|---|---|---|
| Serial startup messages appear | Both lines above | |
| Recurring lines print every ~1 s | Yes | |
| Tab bar shows six tabs: Solar, Wind, Bus, Battery, Control, Status | Yes | |
| Solar tab (default) shows voltage, current, and power, updating about every 0.5 s | Yes | |
| Tapping each tab switches to its screen | Yes | |
| Status tab shows OK in green | Yes | |
| Display backlight on | Yes | |
| Board doesn't reset repeatedly | No resets | |
| Mega and display don't get hot | Yes | |

**If the display stays blank:** check LCD_RST (pin 9) first. Reviewers of this panel report a white or blank screen when RST isn't connected.

**If startup messages don't appear:** check the USB connection and the Serial Monitor baud rate before checking the hardware.

**Result of this section:** PASS / FAIL / BLOCKED
