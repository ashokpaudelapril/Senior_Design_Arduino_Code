# 1. Boost Converter — Full Design Walkthrough (EasyEDA)

Everything needed to actually design this board, start to finish, in one place: edition/setup, the BOM, schematic capture, component placement, sensor wiring, routing, thermal, and fabrication. Kept alongside `04-Altium-Design-Walkthrough.md` (the same walkthrough for Altium) since the team evaluated both — this one for EasyEDA specifically.

## 1.1 Edition

This project's actual project file (`Boost Convertor.eprj2`) confirms **EasyEDA Pro**, not Standard — a `.eprj2` file is Pro's own project database format and won't open in Standard.

| Edition | Best for | Get it |
|---|---|---|
| **Pro** (what this project uses) | Teams/enterprise — built-in auto-routing, netlist comparison, project version history/branches, desktop-only | Free desktop client at [easyeda.com/page/download](https://easyeda.com/page/download) |
| Standard | Students/education — simpler, free, browser-based | [easyeda.com](https://easyeda.com) in-browser |

**Practical note:** the `.eprj2` file is a SQLite-based internal database — don't hand-edit it outside the Pro application itself; any schematic/PCB changes go through the actual Pro editor. Back it up by copying the whole file, not by extracting pieces from it.

## 1.2 Library — EasyEDA's Real Advantage

EasyEDA, JLCPCB, and LCSC are sister companies, so EasyEDA's library is directly linked to LCSC's real inventory — searching a real part number usually gives a ready-made symbol, footprint, and 3D model in one step, no separate library-sourcing hunt the way Altium requires. Search EasyEDA's own library first for every part below before falling back to SnapEDA/Ultra Librarian.

## 1.3 The BOM

| Ref | Part | Part number | What to search |
|---|---|---|---|
| Q1 | GaN switch | Renesas TP65H150G4PS | `TP65H150G4PS` — confirmed Active with 872 units in stock directly on Renesas's own product page. If not found in EasyEDA's library, SnapEDA/Ultra Librarian as fallback. |
| D1 | Boost diode | onsemi 1N5822, LCSC `C3757288` (40V 3A Schottky, DO-201AD) | `C3757288` — confirmed exact part, search this code directly |
| L1 | Inductor | Sunltech SLO0650H470MTT, 47µH, 2A, LCSC `C397794` | `C397794` — replaces the originally-specified Vishay IHTH0750IZEB470M5A, which is real but being discontinued by Vishay (confirmed via RS-Online) and never properly listed in EasyEDA's library as a result. 2A rating comfortably covers the circuit's actual ~2A common-bus current. Smaller/less-established manufacturer than Vishay — worth pulling the real datasheet and verifying DC resistance/saturation current at the bench rather than trusting the LCSC summary spec alone. |
| U1 | Gate driver | Microchip TC4427AEPA | `TC4427AEPA` |
| C1 | Input cap | Panasonic EEU-FR1E471, 470µF 25V | `EEU-FR1E471` |
| C2 | Output cap | KEMET A755KS227M1EAAE025, 220µF 25V | `C599541` — search the LCSC code, not the raw part number string, which didn't surface it directly. Availability signals are mixed (RS lists it as discontinued, DigiKey shows Active with stock) — confirm current stock on LCSC before committing. |
| C3 | Bypass cap | TDK FG28X7R1H104KNT00, 0.1µF 50V X7R | `FG28X7R1H104KNT00` |
| C4 | Bypass cap | TDK FG24X7R1H105KRT00, 1µF 50V X7R | `FG24X7R1H105KRT00` |
| U2 | INA226 sensor (off-board module) | CJMCU-226, R010 shunt | **Don't search "INA226"** — that finds the bare 8-pin IC, not this module. The CJMCU-226 is a separate physical board connected by wire; what you place here is a generic 2-pin header (search `Header` or `Connector`) at the power break point, plus a shared connection into the I2C bus header (J4). Common-bus (input-side) sense, I2C addr `0x43`. |
| U3 | INA226 sensor (off-board module) | CJMCU-226, R010 shunt | Same as U2 — generic header, not an "INA226" search. Battery (output-side) sense, I2C addr `0x41`. |
| J1 | Input connector | Phoenix Contact 1729128 (2-pos, 5.08mm) | `1729128` — confirmed/ordered, 10 pcs in stock |
| J2 | Output connector | Phoenix Contact 1729128 | `1729128` — same part as J1 |
| J3 | Gate-drive signal header | Not yet selected | Generic `Conn_01x02` or similar — 2-pin, signal + ground |
| J4 | I2C bus header | Not yet selected | Generic connector — breaks out to the rest of the system's shared I2C bus |

For any part with a long list of search results (common for generic names like `INA226`), confirm the match before placing it: real LCSC `C`-number first if you have one → manufacturer name → package/footprint → listed electrical specs. Don't just take the top result.

## 1.4 Schematic Capture

Build the circuit matching `boost converter ltspice.png`'s topology: input source → L1 → switch node (Q1 drain + D1 anode) → D1 → output cap/load, gate driver feeding Q1's gate, bypass caps on U1's supply pins.

1. Open the Library panel (left sidebar) and search each part by the term in §1.3's table.
2. Click a result to preview it, drag it onto the schematic sheet, press `R` to rotate if needed.
3. Wire the power path: J1 → U2 (in series — see §1.6) → C1 → L1 → switch node → D1 → C2 → U3 (in series) → J2.
4. Wire the gate path: J3 → U1 → Q1 gate.
5. Wire the I2C path: U2 and U3's SDA/SCL/VCC/GND → J4 — shared bus, both sensors' logic pins land on the same two signal nets.
6. Label nets clearly (`VIN`, `SW_NODE`, `VOUT`, `GATE`, `SDA`, `SCL`).
7. Run **Design → Electrical Rule Check (ERC)**. Fix anything it flags before moving to layout.

## 1.5 Component Placement

The governing constraint: **the power loop (C1 → L1 → switch node → D1 → C2) is the highest-current, highest-frequency path on the board** — parasitic inductance from long traces here directly costs efficiency and can cause ringing. Everything else is secondary to keeping that loop physically tight.

1. **Design → Convert Schematic to PCB.**
2. Set the board outline (**Place → Board Outline**) — keep it as small as reasonably possible; cost scales with area.
3. Place components:
   - **Q1, L1, D1 close together**, as small a physical loop as the footprints allow.
   - **U1 as close as possible to Q1's gate pin** — gate trace length matters more than almost anything else on this board.
   - **C3/C4 directly adjacent to U1's supply pins.**
   - **U2 and U3 outside the tight loop**, at the loop's edges where they naturally break the input and output traces.
   - **J1/J2 at the board edges**, in line with the power path's natural entry/exit points.
   - **Ground/thermal copper pour** under Q1 and U1 — double duty as a low-impedance return path and heat spreading.

## 1.6 Series vs. Parallel — Sensor Wiring

U2 and U3 are current sensors — they have to be spliced *into* the power path (current flows through them), not tapped onto it at one point. Cut the trace, route both ends to the module's `IN+`/`OUT+` pins. Their I2C pins (SDA/SCL/VCC/GND) are the opposite — shared bus, same two signal nets as every other I2C device in the system, not broken into anything.

**Address jumpers:** confirm U2's A0/A1 jumper gives `0x43` and U3's gives `0x41` before mounting.

**Pull-up resistors:** each CJMCU-226 module typically ships with its own onboard I2C pull-ups. With multiple sensors sharing one bus, these can stack and push combined resistance too low. Only one set of pull-ups should be active per bus — check whether each module has a way to disable its own if more than one is on the same bus.

## 1.7 Routing

1. Click the **Route** tool.
2. **High-current path (J1→U2→C1→L1→switch node→D1→C2→U3→J2)**: size trace width (or use a copper pour) for the real current this board carries — use EasyEDA's trace width calculator against actual current, not a default signal-trace width.
3. **Gate trace (U1→Q1)**: short and direct matters more than width here.
4. **Ground plane/pour** (**Place → Copper Area**) on at least one layer.
5. Run **Design Rule Check (DRC)**. Fix clearance/unrouted-net issues before generating fab files.

## 1.8 Thermal (Q1)

Pull the TP65H150G4PS datasheet and check its thermal pad guidance. GaN switches at this power level typically need an exposed thermal pad soldered to a copper area, with thermal vias down to a ground/heat-spreading plane on another layer. Skipping this is the classic cause of a board that works fine on the bench for a few minutes, then overheats once run continuously.

## 1.9 Fabrication

Once DRC is clean, click **Fabrication → PCB Fabrication File (Gerber)**. Since EasyEDA/JLCPCB/LCSC are sister companies, EasyEDA offers a **one-click "Order at JLCPCB"** option with your design pre-filled — no manual Gerber export/upload step, unlike Altium's workflow. Still exports standard Gerbers if you want a different fab house (PCBWay, OSH Park).

**Before ordering: bench-test the circuit on the Vector Electronics 8029 perfboard first.** A wiring mistake on a fabricated board costs real money and days of turnaround; the same mistake on perfboard costs you re-soldering one joint.

## 1.10 Full Connection List

Plain pin-to-pin reference for the whole board.

**Power path (series — current flows through every link):**
1. J1 (VIN+) → U2 power-in
2. U2 power-out → C1 (+)
3. C1 (+) → L1 pin 1
4. L1 pin 2 → switch node
5. Switch node → Q1 drain
6. Switch node → D1 anode
7. D1 cathode → C2 (+)
8. C2 (+) → U3 power-in
9. U3 power-out → J2 (VOUT+)

**Ground:** J1 pin 2, C1 (−), Q1 source, C2 (−), J2 pin 2, U1 GND, U2 GND, U3 GND — all one shared net.

**Gate-drive signal:**
- U1 output → Q1 gate (short, direct — trace length matters most here)
- J3 → U1 input (PWM in from the microcontroller)
- U1 VCC → its own gate-driver supply rail (not the same rail as VIN/VOUT)
- C3, C4 → straight across U1's VCC/GND pins

**I2C bus (shared):**
- U2 SDA, U3 SDA → shared SDA net → J4
- U2 SCL, U3 SCL → shared SCL net → J4
- U2/U3 VCC/GND → shared logic power/ground (separate from the high-current sense pins above)
