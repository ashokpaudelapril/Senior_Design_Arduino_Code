# 6. Display Guide

The 3.5" touch screen shows a tabbed status display. A tab bar runs across the top, and one tab's information is shown below it at a time.

## 6.1 Tab Bar

| Tab | Shows |
|---|---|
| **Solar** | Voltage, current, and power of the solar array (default tab on startup) |
| **Wind** | Voltage, current, and power of the wind branch |
| **Bus** | Voltage, current, and power of the common DC bus, the boost converter's input |
| **Battery** | Voltage, current, and power at the battery / boost output |
| **Control** | Duty cycle (PWM, as a percentage) and boost efficiency |
| **Temp** | GaN switch and inductor temperatures, from the two DS18B20 probes |
| **Relays** | Six buttons (Solar, Wind, Data Ctr, EV Chg, LED Lamp, Other) — tap to turn each relay-controlled circuit on/off. See §6.1a |
| **Status** | Charge-controller state: **OK** in green, or **FAULT** in red, with the reason |

The active tab is shown in white in the tab bar. Inactive tabs are grey.

Every tab also shows a footer at the bottom: **Mode: AUTOMATIC** or **Mode: MANUAL TEST**, reflecting `MANUAL_PWM_TEST_MODE` in `Config.h`. Since this is a compile-time setting, it can't change without a re-upload, so the footer is accurate for as long as the board is running.

## 6.1a The Relays Tab

Each button controls one relay-controlled circuit (the solar array, the whole wind subsystem, and 4 demo loads — data center, EV charging, LED lamp, other). Tapping a button toggles what you're asking for, not necessarily what happens immediately — the button's color tells you which:

| Color | Means |
|---|---|
| **Grey**, "OFF" | You haven't asked for this one to be on |
| **Green**, "ON" | You asked for it, and it's actually energized |
| **Yellow**, "WAIT" | You asked for it, but it's being held off — **load buttons only** |

**Why a load button can show WAIT:** the 4 load relays (Data Ctr, EV Chg, LED Lamp, Other) have an automatic check on top of your button press — `LoadControl.cpp` only actually energizes one if the battery/charge-output sensor shows enough power available (`LOAD_ENABLE_MIN_BATTERY_POWER_W` in `Config.h`). Tapping the button sets your intent; it doesn't override this check. The **Solar** and **Wind** buttons have no such check — they go straight to ON or OFF.

**This is a deliberate, demonstrable feature**, not a bug: e.g. tap all 4 load buttons ON, then watch one or more flip to WAIT if you block the solar panel and power drops below the threshold.

## 6.2 Switching Tabs

Tap any tab in the top bar. The screen switches right away.

Tapping anywhere below the tab bar does nothing, **except on the Relays tab**, where tapping one of its 6 buttons toggles that relay (see §6.1a).

## 6.3 How Values Update

- Values refresh about **twice per second**.
- The tab you're on always shows the latest reading. Switching tabs redraws it immediately.
- Each number is shown with a fixed number of decimal places: two for voltage, current, and power, and one for duty cycle and efficiency.
- Efficiency shows **0.0 %** whenever the input is below 0.1 W. That's expected, not a fault. See [04-Serial-Output-Reference.md](04-Serial-Output-Reference.md) for why.
- The **Temp** tab shows **"--"** instead of a number until the first temperature reading completes (DS18B20 probes take up to ~800ms per reading) or if a probe isn't connected. That's expected, not a fault.

## 6.4 Status Tab Meanings

| Shown | Meaning | What to do |
|---|---|---|
| **OK** (green), "Running normally." | No fault latched | Nothing |
| **FAULT** (red), with a reason line, "Output off. Reset the board to clear." | A fault has latched and duty cycle is forced to 0 % | Find the cause using [05-Troubleshooting.md](05-Troubleshooting.md), then reset |

The reason line shows one of four short messages, matching the fault checks in `ChargeController.cpp`:

| Reason shown | Which check tripped |
|---|---|
| "Battery sensor not responding" | Sensor-missing check (automatic mode only) |
| "Possible open circuit (no battery/load)" | Open-circuit / pinned-duty timeout (automatic mode only) |
| "Hard voltage/current limit exceeded" | Hard safety cutoff (both modes) |
| "Overtemperature (GaN switch or inductor)" | Thermal cutoff (both modes) — only trips if the probes have a valid reading; if they're not connected, this protection is silently absent, see [01-Wiring-and-Assembly.md](01-Wiring-and-Assembly.md) §1.3a |

This is the same information the Serial Monitor prints at the moment of the fault, just also available on the Status tab after the fact, without needing a laptop attached.

## 6.5 Not Shown on the Display

- **Load sensor:** not shown. It's a placeholder until the buck converter exists. See [01-Wiring-and-Assembly.md](01-Wiring-and-Assembly.md) §1.1.
- **Exact fault numbers:** the Status tab shows the fault reason, but not the measured voltage/current at the moment it tripped. Those numbers are still Serial-only — see [04-Serial-Output-Reference.md](04-Serial-Output-Reference.md).

## 6.6 Troubleshooting the Display

| Symptom | Likely cause | Fix |
|---|---|---|
| Blank screen | `LCD_RST` (pin 9) not connected | Connect it, then reset |
| Colors look wrong (e.g. red and blue swapped) | Panel color order | Add `tft.invertDisplay(true);` after `tft.begin();` in `Display.cpp`, per user reports on this panel |
| Tapping a tab does nothing | Touch controller not responding, or tap zones off | Check CTP_SDA/SCL (pins 20/21) and CTP_RST/INT (pins 6/3). If the tab is wrong, touch coordinates may need flipping in `display_handleTouch()` |
| Tapping opens the wrong tab | Touch x-axis is mirrored or swapped for landscape | Adjust the coordinate mapping in `display_handleTouch()` |
| Numbers stop updating | Serial output still works but the screen doesn't, or the board is stuck | Check the Serial Monitor. If it updates there, check the display SPI wiring (control pins 8, 9, 10; SPI on the Giga's main header D89–D91) |
| Text overlaps or is cut off | Panel orientation differs | Change `tft.setRotation(1)` to 0–3 in `Display.cpp` |

If a fix changes the code, update this section and the bench test log.
