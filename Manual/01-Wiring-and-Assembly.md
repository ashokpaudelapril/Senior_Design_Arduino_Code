# 1. Wiring and Assembly

All pin numbers below refer to the **Arduino GIGA R1 WiFi**, and all come directly from `Config.h` — if you rewire something, update `Config.h` to match, not just this document.

> **Board change:** the project was originally written for the Arduino Mega 2560. The Giga R1 WiFi is now the target board. The pin numbers for signals on digital pins are the same, but the display's SPI wires move to the Giga's main SPI header (see §1.3). The gate-drive PWM code is not yet ported; see §1.2.

## 1.1 I2C Sensor Bus (INA226 × 4, + 1 placeholder)

All INA226 power monitors share the Giga's I2C bus:

| Signal | Giga Pin |
|---|---|
| SDA | 20 |
| SCL | 21 |

Each sensor needs a **unique I2C address**, set by bridging its A0/A1 solder jumpers:

| Sensor | Address | Jumper Setting | Measures |
|---|---|---|---|
| Solar | `0x40` | A0 = GND, A1 = GND (default) | Solar array output, before its SB540 isolation diode |
| Battery | `0x41` | A0 = VS, A1 = GND | Boost converter output / battery charging side |
| Wind | `0x42` | A0 = SDA, A1 = GND | Wind branch output, before its SB540 isolation diode |
| Common DC Bus | `0x43` | A0 = SCL, A1 = GND | After both isolation diodes combine, right before the boost converter's actual input |
| Load *(placeholder — do not wire yet)* | `0x44` | A1 = VS, A0 = GND | **No physical location exists yet.** The buck converter (12.8V→5V) is still an undecided hardware choice — this sensor is a code-only placeholder for its future output. Wiring this up before the buck converter exists would just measure nothing meaningful. |

**Verify each jumper against your specific board's silkscreen before soldering** — jumper labeling can vary slightly between suppliers of the CJMCU-226 module.

All sensors use a **0.010 Ω shunt resistor** (the CJMCU-226 board's onboard value) and are calibrated in firmware for a **0–2.0 A** current range.

## 1.2 Boost Converter Gate Drive

| Signal | Giga Pin | Notes |
|---|---|---|
| Gate drive PWM (to TC4427 driver input) | 11 | Also the SPI5 COPI pin, so keep that SPI bus unused. 100 kHz frequency and ~0.042% duty resolution both confirmed on the scope. |

**Status:** `BoostConverter.cpp` has been ported to the Giga. It drives STM32's TIM8 Channel 2 directly via HAL (not Mbed's `PwmOut`, which was confirmed too coarse), giving true 100 kHz with ~0.042% duty resolution — confirmed on the scope at 35%, 55%, and 58%. This has been validated with a standalone test sketch; the full firmware integration (via `ChargeController`) still needs an end-to-end bench test.

## 1.3 Display and Touch (Hosyond 3.5" ST7796U / FT6336U)

| Signal | Giga Pin | Notes |
|---|---|---|
| LCD_CS | 10 | Display chip-select |
| LCD_RST | 9 | Display reset |
| LCD_RS (DC) | 8 | Display data/command select |
| LED | 7 | Backlight (currently always-on; PWM-capable for future dimming) |
| SDI (MOSI) | D90 (COPI) | Giga main SPI header — fixed by the board, not a firmware choice |
| SCK | D91 | Giga main SPI header — fixed by the board |
| SDO (MISO) | D89 (CIPO) | Giga main SPI header — fixed by the board |
| CTP_SDA | 20 | **Shared** with the INA226 I2C bus above |
| CTP_SCL | 21 | **Shared** with the INA226 I2C bus above |
| CTP_RST | 6 | Touch controller reset |
| CTP_INT | 3 | Touch controller interrupt |
| SD_CS | *(not connected)* | This project doesn't use the display's microSD slot |
| VCC | 5V | Display working voltage (5V recommended over 3.3V — see product spec sheet) |
| GND | GND | |

## 1.3a Temperature Probes (GaN Switch / Inductor DS18B20)

| Signal | Giga Pin | Notes |
|---|---|---|
| Data (1-Wire) | 2 | Shared by BOTH probes — DS18B20 uses one bus, not one pin per sensor |
| Pull-up resistor | 4.7 kΩ, between the data line and **3.3V** | Comes pre-mounted on the DROK adapter board if you use it. **Do not connect to 5V** — the Giga's logic is 3.3V, and a 5V pull-up would overvoltage the GPIO |
| VCC | 3.3V | |
| GND | GND | |

**Which probe is which is NOT guaranteed by wiring order.** The firmware identifies probes by search index (0 = GaN switch, 1 = inductor), which is determined by each probe's unique factory ID, not which one you plugged in first. **Verify this on the bench**: warm one probe (finger or hair dryer) and confirm the Serial Monitor's `Temp:` line shows the change on the expected reading (GaN vs. Inductor), not the other one.

## 1.3b Relay Module (Solar / Wind / Load Switching)

| Signal | Giga Pin | Notes |
|---|---|---|
| Relay channel 1 (Solar) | 4 | Solar array → common bus |
| Relay channel 2 (Wind) | 5 | Whole wind subsystem (drive motor + generators) → common bus |
| Relay channel 3 (Data Center load) | 12 | |
| Relay channel 4 (EV Charging load) | 22 | |
| Relay channel 5 (LED Lamp load) | 23 | |
| Relay channel 6 (Other load) | 24 | |
| VCC (logic side) | 3.3V | **Not** the module's `JD-VCC` pin — see below |
| GND | GND | Shared with the rest of the system |
| JD-VCC (relay coil side) | separate 5V supply | **Not** the Giga's 3.3V or 5V pin — see below |

**This board replaces what was originally a plan for 6 manually-flipped toggle switches** (solar, wind, and 4 demo loads) with Arduino-driven relays — a touchscreen button sets intent, and `LoadControl.cpp` decides whether it actually energizes. Everything else from the original 14-switch plan (battery disconnect, master power, bus-to-boost-input, etc.) is **still a manual physical switch**, not relay-controlled — see the switch sourcing discussion for why (several of those exist specifically so a human can isolate something for bench work, or need to stay connected for the charge controller to function).

**The `JD-VCC` jumper matters here.** Most of these common 8-channel relay boards ship with a jumper bridging `VCC` (logic/optocoupler side) and `JD-VCC` (relay coil side) into one shared 5V rail. The Giga's GPIO is 3.3V logic, so **remove that jumper** and power the two sides separately:
- `VCC` ← Giga 3.3V (this is the side the GPIO pins actually talk to)
- `JD-VCC` ← a separate 5V supply (this is what actually powers the relay coils)
- `GND` is shared between both

This keeps the Giga's GPIO pins only ever driving the optocoupler input, never carrying the relay coil's current.

**Active-low, not active-high.** These modules commonly turn a relay ON with a LOW signal, not HIGH — the opposite of what most people assume. `RELAY_ACTIVE_LOW` in `Config.h` already assumes this, but **confirm it on the bench** (watch/listen for the relay's click while toggling a channel both ways) before trusting it, same as the other "verify before relying on this" items in this project.

**Motor relay note (wind):** the relay module's own coil already has flyback protection built in, but that protects the relay's coil, not the wind drive motor circuit it switches. Add a flyback diode across the motor terminals themselves — breaking a running motor's circuit lets its collapsing magnetic field spike voltage across the opening contacts otherwise.

## 1.3c EV-Charging Demo Load (Li-ion Cell + TP4056) and Demo-Day Windmill Drive

These are two SEPARATE batteries, each doing one job — not one battery doing both (that was considered and rejected twice: a single 18650 cell can't drive an R280 motor, and a pack big enough to drive one can't be charged from the 5V bus; simultaneously charging and discharging through a TP4056 isn't something that module supports either).

**EV-charging demo load** — represents "charging an EV" for the demo, and is one of the 4 relay-controlled loads:

| Signal | Connects to | Notes |
|---|---|---|
| TP4056 `IN+`/`IN-` | 5V load bus, through `RELAY_LOAD_EV` | Downstream of the buck converter (12.8V→5V) — same as the other 3 demo loads. **Not wireable yet** — the buck converter itself doesn't exist (pending LTspice design), same placeholder status as `INA226_LOAD_ADDR` |
| TP4056 `OUT+`/`OUT-` (sometimes `B+`/`B-`) | Single 18650 cell, via its holder | The module handles the full CC/CV charge profile and overcurrent/overvoltage protection internally — nothing for firmware to do here |

Parts: TP4056 charger module, a **protected** 18650 cell (2nd safety layer on top of the module), single-cell holder with leads — see the parts-sourcing discussion for real Amazon listings. This battery is never wired to any motor — it only ever receives charge.

**Demo-day windmill drive** — replaces the single shared drive-motor/belt arrangement with **4 independent driver motors, one per windmill**, each belt-coupled to its own generator motor:

- **4 driver motors** (R280, 6–24V) — new parts, separate from the 4 generator motors. A motor can't simultaneously be driven by battery power and produce a meaningful generated reading, so driving and generating are different physical motors now, not the same one.
- Each driver motor connects via its **own small belt** to its paired **generator motor** (the existing 4 R280 motors from the original design) — not a single shared belt across all 4.
- **All 4 driver motors are powered from a dedicated 3S Li-ion pack** (11.1V nominal, 12.6V max charge, 2200mAh/24.42Wh, Model OV0000007-04) — a separate battery bought specifically for this, not the project's main 12.8V LiFePO4 battery. This connection is manual (its own switch per motor, or a shared one), isn't read by any sensor, and isn't gated by any code — purely a physical demo, separate from the charge-controller firmware entirely.

**Known failure mode — confirmed on the bench**: a motor whose shaft can't turn freely (belt too tight, pulley misaligned, blades catching the frame) draws many times its rated current and overheats within seconds — this already happened to one driver motor during testing. Before wiring any driver motor to this battery:
- **Hand-spin each motor with its belt already attached** and confirm it turns smoothly before ever applying power. Do this again any time the belt, pulley, or mount is touched.
- **Fuse each driver motor individually** — a ~1A fast-blow or resettable PTC fuse in line with each motor (rated current is only up to ~0.5A, so 1A gives headroom for normal running while still tripping well before a stall can cook the windings).
- **A PWM speed controller is recommended inline** (battery → speed controller → motor) — besides slowing the visible rotation down (not running at full rated speed isn't needed for this demo), lowering the effective voltage also reduces worst-case stall current, adding a second layer of margin on top of the fuse. This does **not** replace the fuse or the hand-spin check — a motor that's still mechanically bound can overheat over time at any speed-controller setting.
- A motor that has visibly smoked (charred windings, burnt smell from inside the housing) should be replaced, not reused — winding damage tends to recur even if the motor still turns afterward.

This firmware doesn't know or care which drive arrangement is physically installed — the generator motors still just feed the wind-branch INA226 sensor the same way regardless of what's spinning them.

## 1.4 Power

- The Giga, sensors, and display share a common ground with the boost converter's power stage.
- The 3S driver-motor battery is **electrically separate** from the renewable energy path — it only powers the 4 demo-day windmill driver motors (§1.3c), and should never be wired into the solar/wind/battery sensing or charging circuit.
- The EV-charging demo load's 18650 cell (§1.3c) is likewise electrically separate from the main battery and from the driver-motor battery — it's charged BY the system (through the buck converter and TP4056), not a power source for anything.

## 1.5 Assembly Checklist

- [ ] All four active INA226 sensors have distinct, verified I2C addresses
- [ ] Gate drive signal (pin 11) routes to the TC4427 driver input, not directly to the GaN gate
- [ ] Display and touch controller share the I2C bus correctly (no address conflict with INA226 sensors — display's touch chip uses its own fixed I2C address separate from 0x40/0x41/0x42)
- [ ] Common ground verified across all modules with a multimeter before first power-up
- [ ] Driver-motor battery confirmed isolated from the renewable energy/charging circuit
- [ ] EV-charging demo load's 18650 cell is protected, and confirmed isolated from the main battery/charging circuit (it's charged BY the system, not feeding it)
- [ ] Each of the 4 driver motors' connection to the battery is manual (its own switch), not wired through any relay
- [ ] Each driver motor hand-spins freely (belt attached) before any power is applied — **a motor already smoked from skipping this check**
- [ ] A ~1A fuse (or PTC) is in line with each individual driver motor
- [ ] A PWM speed controller sits between the battery and the driver motors
- [ ] Temperature probe pull-up resistor connects to 3.3V, not 5V
- [ ] GaN switch vs. inductor probe identity verified on the bench (see §1.3a), not just assumed from wiring order
- [ ] Relay module's `VCC`/`JD-VCC` jumper removed, with `VCC` on 3.3V and `JD-VCC` on its own 5V supply (see §1.3b)
- [ ] Relay active-high/active-low sense confirmed on the bench, not just assumed from `RELAY_ACTIVE_LOW`
- [ ] Flyback diode across the wind drive motor terminals (not the relay)
