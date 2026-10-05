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
| **Status** | Charge-controller state: **OK** in green, or **FAULT** in red |

The active tab is shown in white in the tab bar. Inactive tabs are grey.

## 6.2 Switching Tabs

Tap any tab in the top bar. The screen switches right away.

Tapping anywhere below the tab bar does nothing. Only the tab bar changes tabs.

## 6.3 How Values Update

- Values refresh about **twice per second**.
- The tab you're on always shows the latest reading. Switching tabs redraws it immediately.
- Each number is shown with a fixed number of decimal places: two for voltage, current, and power, and one for duty cycle and efficiency.
- Efficiency shows **0.0 %** whenever the input is below 0.1 W. That's expected, not a fault. See [04-Serial-Output-Reference.md](04-Serial-Output-Reference.md) for why.

## 6.4 Status Tab Meanings

| Shown | Meaning | What to do |
|---|---|---|
| **OK** (green), "Running normally." | No fault latched | Nothing |
| **FAULT** (red), "Output off. Reset the board to clear." | A fault has latched and duty cycle is forced to 0 % | Find the cause using [05-Troubleshooting.md](05-Troubleshooting.md), then reset |

The Status tab doesn't say which fault occurred. For the specific reason, check the Serial Monitor, which prints a fault message the moment it trips.

## 6.5 Not Shown on the Display

- **Load sensor:** not shown. It's a placeholder until the buck converter exists. See [01-Wiring-and-Assembly.md](01-Wiring-and-Assembly.md) §1.1.
- **Operating mode:** the display doesn't show whether manual or automatic mode is active. Check `MANUAL_PWM_TEST_MODE` in `Config.h`.

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
