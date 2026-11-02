# 3. Sensor Placement and Design — INA226 on the Boost Converter PCB

Covers the CJMCU-226 (INA226) current/voltage sensors that belong on this specific board — not all four sensors in the system, just the two that electrically sit on the boost converter itself.

## 3.1 Which Sensors Belong on This Board

The project has four INA226 sensors total, but only **two** are electrically part of the boost converter — the other two (Solar, Wind) measure points upstream of the common bus, before the two branches even merge, so they physically belong near the solar array/wind generator wiring, not on this board.

| Sensor | I2C Address | What it measures | Belongs on this board? |
|---|---|---|---|
| Common DC Bus | `0x43` | The boost converter's **input** — after both isolation diodes merge, before the inductor | **Yes** |
| Battery | `0x41` | The boost converter's **output** — the charging current/voltage actually reaching the battery | **Yes** |
| Solar | `0x40` | Solar array output, before its isolation diode | No — lives near the solar array |
| Wind | `0x42` | Wind branch output, before its isolation diode | No — lives near the wind generator wiring |

## 3.2 Important: The Shunt Resistor Is Already On the Module

The `INA226_SHUNT_OHMS 0.010` value in `Config.h` is explicitly documented as **the CJMCU-226 board's own onboard shunt** — you're not placing a separate shunt resistor on this PCB. The CJMCU-226 module is a complete, self-contained current-sense unit: the high-current path physically passes *through* the module (in one side, out the other, through its onboard shunt), and the module reports voltage/current over I2C.

This simplifies the PCB design considerably: your job is routing the power path *out to* the module and *back from* it at the right two points, not designing a shunt circuit from scratch.

## 3.3 Physical Placement on the Board

**Common Bus sensor (input side)**: break the input trace between where the combined solar+wind power arrives on this board and the input capacitor/inductor. The module sits in series, in that break.

```
Combined DC bus in --> [CJMCU-226 module, Common Bus sensor] --> Input cap (EEU-FR1E471) --> Inductor (IHTH0750IZEB470M5A) --> switch node
```

**Battery sensor (output side)**: break the output trace between the output capacitor and wherever the battery connector leaves this board.

```
Diode (1N5822) --> Output cap (A755KS227M1EAAE025) --> [CJMCU-226 module, Battery sensor] --> Battery connector/terminal
```

**Practical layout choice**: since these are off-the-shelf breakout modules (not bare ICs you're placing directly), decide now whether they'll be:
- **Socketed/header-mounted** — place 2-pin headers on the PCB at each break point, with wires or pin connections to the external module. Easier to swap/replace a sensor without reflowing anything, and keeps the module physically off the high-density power section of the board.
- **Edge-mounted with screw terminals** — if you want the modules more permanently integrated, route the high-current traces to pads positioned at the board edge where the CJMCU-226's own terminals can sit directly adjacent.

Either way, keep both sensor break-points **out of the tight switch/diode/inductor loop** described in `02-EasyEDA-Component-Reference-Detail.md` §2.3 — that loop needs to stay small and high-current; the sensor break points are a reasonable place for the loop to extend slightly, not a reason to make the loop bigger than necessary elsewhere.

## 3.4 Current-Path Trace Design Through the Sensor

The traces carrying power *to* and *from* each CJMCU-226 module need the same current-capacity sizing as the rest of the power path (see `02-EasyEDA-Component-Reference-Detail.md` §2.4) — these traces carry the real operating current (up to ~2A on the common-bus side, up to the 1.5A hard limit on the battery side), not a low-current signal.

**Avoid a Kelvin-sensing mistake**: don't tap the module's I2C/power-reporting connection from a point that shares resistance with the main current path in a way that adds error — since the shunt itself is entirely inside the module, this mostly just means: route the two power traces (in and out of the module) directly, without any other components or long shared trace segments between the break point and the module's terminals, so the module sees the actual current cleanly.

## 3.5 I2C Bus Layout

Both sensors share the same I2C bus (SDA/SCL) as the other two sensors and the touch controller elsewhere in the system — on this board, you're just adding a stub/header connection to that shared bus, not a new bus.

**Real gotcha to watch for**: each CJMCU-226 module typically ships with its own onboard I2C pull-up resistors by default. If multiple modules are daisy-chained on the same bus, each contributing its own pull-up, the resulting *combined* pull-up resistance (several resistors in parallel) can end up too low, which can distort signal levels or exceed the bus driver's sink current rating. Check whether your specific CJMCU-226 modules have a way to disable their onboard pull-ups (often a trace to cut or a jumper) when more than one is on the same bus — only one set of pull-ups (4.7kΩ typical) should be active per I2C bus, not one set per module.

**Address jumpers**: confirm the A0/A1 solder jumpers on each module are set correctly before mounting —
- Common Bus sensor: A0 → SCL (gives address `0x43`)
- Battery sensor: A0 → VS (gives address `0x41`)

per the existing jumper table in `Manual/01-Wiring-and-Assembly.md` §1.1 — worth double-checking against your specific module's silkscreen, since jumper labeling can vary slightly between suppliers (already flagged in that doc for the same reason).

## 3.6 Silkscreen/Labeling

Label both header/connection points clearly on the PCB silkscreen (e.g., `BUS_SENSE_IN` / `BUS_SENSE_OUT`, `BATT_SENSE_IN` / `BATT_SENSE_OUT`) — with two sensors on one board plus the high-current power path, it's easy to mix up which break point is which once the board is populated and no longer matches the clean schematic in front of you.
