# UWBG Power Convertor — Build & Operation Manual

This manual covers how to wire, set up, and operate the Arduino firmware for the boost converter / charging control subsystem. For *why* the design decisions were made, see the `Product Specification/` folder instead — this manual is the practical "how to actually build and run this" reference.

## Sections

1. [01-Wiring-and-Assembly.md](01-Wiring-and-Assembly.md) — pin connections for sensors, gate drive, and display
2. [02-Firmware-Setup.md](02-Firmware-Setup.md) — required libraries, Arduino IDE setup, uploading
3. [03-Operating-Modes.md](03-Operating-Modes.md) — manual PWM test mode vs. automatic CC/CV charging
4. [04-Serial-Output-Reference.md](04-Serial-Output-Reference.md) — what every Serial Monitor message means
5. [05-Troubleshooting.md](05-Troubleshooting.md) — faults, common issues, and how to resolve them
6. [06-Display-Guide.md](06-Display-Guide.md) — the tabbed touch screen: what each tab shows and how to switch tabs

## Quick Start

1. Wire the board per [01-Wiring-and-Assembly.md](01-Wiring-and-Assembly.md)
2. Install the libraries listed in [02-Firmware-Setup.md](02-Firmware-Setup.md)
3. Open `UWBG_Power_Converter.ino` in the Arduino IDE, check `Config.h` for `MANUAL_PWM_TEST_MODE` (see [03-Operating-Modes.md](03-Operating-Modes.md))
4. Upload, then open Serial Monitor at **9600 baud**
5. Read output per [04-Serial-Output-Reference.md](04-Serial-Output-Reference.md)

This manual reflects the firmware as of Block 13 (display initialization). Update it as new blocks are added — it should always describe what the code actually does, not what's planned.
