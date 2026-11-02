# 4. Boost Converter — Full Design Walkthrough (Altium)

Everything needed to actually design this board, start to finish, in one place: licensing, the BOM, schematic capture, component placement, sensor wiring, routing, thermal, and fabrication.

## 4.1 Licensing

Altium Designer is commercial, paid software — no free tier. Confirm which license path applies before starting real work:

| Path | Notes |
|---|---|
| **University-provided license** | Check with your department/IT first — many engineering programs provide free or discounted seats. Most likely path for a student project. |
| **Altium 365 free trial** | Time-limited — fine for evaluating, risky to rely on for a whole semester. |
| **Altium Student license program** | Apply through Altium's own education page; check current eligibility directly on their site. |

## 4.1a Getting Started — Creating a Project

Once licensing is sorted and Altium Designer is installed and opened:

1. **File → New → Project.** Altium calls this a "PCB Project" — pick that type when prompted (not a library project or a script project).
2. **Name it and choose a save location.** This creates a `.PrjPcb` file — that's your project file, the thing that ties everything else together. It doesn't contain your actual schematic or board yet, just references to the documents you add to it.
3. **Add a schematic document:** right-click the project in the **Projects** panel (usually docked on the left) → **Add New to Project** → **Schematic**. This creates a `.SchDoc` file — this is where you'll do the work in §4.4.
4. **Add a PCB document the same way:** right-click the project → **Add New to Project** → **PCB**. This creates a `.PcbDoc` file — empty for now, this is where layout happens later (§4.5 onward). You don't need to touch this yet; just get it added to the project so it's ready.
5. **Save everything** (`File → Save All`, or save each document individually) — Altium will prompt you to name the schematic and PCB files if you haven't already.

That's the whole setup: one project file referencing one schematic and one PCB document. Everything in the rest of this walkthrough happens inside the schematic document first, then the PCB document.

## 4.2 Library Sourcing

Altium has no LCSC-linked built-in library the way EasyEDA does — nothing auto-resolves a part number into a ready symbol/footprint. For each part below:

1. Check the manufacturer's own published Altium library first (Renesas, onsemi, Microchip, TDK, KEMET, and Phoenix Contact all publish some).
2. SnapEDA or Ultra Librarian for anything missing — both export directly to Altium format.
3. Octopart to cross-check which source actually has a given part before hunting through each individually.
4. Hand-built symbol/footprint as a last resort, built against real datasheet dimensions.

Passive footprints (the four capacitors) are usually fine straight from the Content Vault — just verify case size against the datasheet rather than trusting the first match.

**How to actually add a downloaded library to the project:**

1. Download the library file from SnapEDA/Ultra Librarian/the manufacturer's site — it'll typically be an `.IntLib` (integrated library) or a `.SchLib`/`.PcbLib` pair (separate symbol/footprint files).
2. Open the **Components** panel (usually docked bottom-right; if not visible, **View → Panels → Components**).
3. Click the small dropdown/panel-options icon near the top of that panel, then **File-based Libraries Preferences** (older Altium versions may show this as **DXP → Preferences → Data Management → File-based Libraries**).
4. In the dialog that opens, go to the **Installed** tab and click **Add Library...**
5. Browse to the file you downloaded and select it. It appears in the list and is now searchable from the Libraries panel, per §4.4's search steps.
6. Repeat for each downloaded library — there isn't one single library that covers this whole BOM; each manufacturer-specific part generally needs its own.

**Which libraries you actually need to go get, per part in the BOM (§4.3):**

| Part needing an external library | Where to get it |
|---|---|
| Q1 — Renesas TP65H150G4PS | Confirmed: Renesas's product page has a direct Altium library download alongside the datasheet — no SnapEDA needed |
| D1 — onsemi 1N5822 (`C3757288`) | onsemi's site first; SnapEDA as fallback |
| L1 — Würth WE-PD 7447706470 | Würth publishes its own CAD libraries (including Altium) directly on their site; SnapEDA also likely has it, being a major brand |
| U1 — Microchip TC4427AEPA | Microchip's site first; SnapEDA as fallback |
| C2 — KEMET A755KS227M1EAAE025 (or the A759 alternative) | KEMET's site first; SnapEDA as fallback |
| J1/J2 — Phoenix Contact 1729128 | Phoenix Contact publishes Altium libraries for its connector series directly |

**Parts that likely don't need an external download at all** — just use Altium's built-in Content Vault, verifying case size/pin count against the datasheet:
- C1 (generic electrolytic cap footprint)
- C3, C4 (generic ceramic cap footprints)
- J3, J4, and the sensor break-point headers (generic connector footprints)

## 4.3 The BOM

Every part that goes on this board, confirmed against real datasheets and, where noted, already ordered.

| Ref | Part | Part number | Notes |
|---|---|---|---|
| Q1 | GaN switch | Renesas TP65H150G4PS | Main switching element. Confirmed Active with 872 units in stock directly on Renesas's own product page — Renesas also provides a direct Altium library download on that same page, so no SnapEDA fallback needed for this part. |
| D1 | Boost diode | onsemi 1N5822, LCSC `C3757288` (40V 3A Schottky, DO-201AD) | Confirmed exact part — search this LCSC code, not "1N5822" generically |
| L1 | Inductor | Würth Elektronik WE-PD 7447706470, 47µH, 2.7A | Replaces the originally-specified Vishay IHTH0750IZEB470M5A, which is real but being discontinued by Vishay (confirmed via RS-Online) with an 18-week lead time elsewhere. Würth was picked specifically for Altium because it's a major brand with real SnapEDA/manufacturer CAD library support — unlike the EasyEDA-side replacement (a smaller LCSC-stocked brand), which has no confirmed Altium footprint path. 2.7A rating comfortably covers the circuit's actual ~2A common-bus current. |
| U1 | Gate driver | Microchip TC4427AEPA | Drives Q1's gate |
| C1 | Input cap | Panasonic EEU-FR1E471, 470µF 25V | Electrolytic, has polarity |
| C2 | Output cap | KEMET A755KS227M1EAAE025, 220µF 25V | Electrolytic, has polarity. LCSC `C599541` confirms this part exists with an active listing, though availability signals are mixed (RS lists it as discontinued, DigiKey shows Active with stock) — check KEMET's own site or a manufacturer-direct distributor for current status before committing, since Altium sourcing doesn't benefit from the LCSC link the way EasyEDA does. |
| C3 | Bypass cap | TDK FG28X7R1H104KNT00, 0.1µF 50V X7R | Goes directly across U1's supply pins |
| C4 | Bypass cap | TDK FG24X7R1H105KRT00, 1µF 50V X7R | Same, adjacent to U1 |
| U2 | INA226 sensor (off-board module) | CJMCU-226, R010 shunt | **Don't search "INA226"** — that finds the bare 8-pin IC, not this module. The CJMCU-226 is a separate physical board connected by wire; what you place here is a generic 2-pin header/connector from the Content Vault at the power break point, plus a shared connection into the I2C bus header (J4). Common-bus (input-side) sense, I2C addr `0x43`. |
| U3 | INA226 sensor (off-board module) | CJMCU-226, R010 shunt | Same as U2 — generic header, not an "INA226" search. Battery (output-side) sense, I2C addr `0x41`. |
| J1 | Input connector | Phoenix Contact 1729128 (2-pos, 5.08mm) | Confirmed/ordered, 10 pcs in stock |
| J2 | Output connector | Phoenix Contact 1729128 | Same part as J1 |
| J3 | Gate-drive signal header | Not yet selected | 2-pin, signal + ground |
| J4 | I2C bus header | Not yet selected | Breaks out to the rest of the system's shared I2C bus |

## 4.4 Schematic Capture

Build the circuit matching `boost converter ltspice.png`'s topology: input source → L1 → switch node (Q1 drain + D1 anode) → D1 → output cap/load, gate driver feeding Q1's gate, bypass caps on U1's supply pins.

**Searching for and placing a part:**

1. Open the **Libraries** panel (usually docked on the right; if not visible, **View → Panels → Libraries**).
2. Make sure the library you need is actually loaded first — for anything sourced via SnapEDA/Ultra Librarian or a manufacturer library (§4.2), add it via the **Components** panel → the "..." / dropdown menu → **File-based Libraries Preferences** → **Add Library**, and point it at the downloaded library file. Altium only searches libraries that are currently added to the project.
3. Type the part number into the Libraries panel's search box.
4. Click a result to preview its schematic symbol.
5. Click **Place** (or drag the result onto the schematic sheet).
6. The part "floats" on your cursor — press **spacebar** to rotate it 90° at a time (Altium's rotate shortcut, different from EasyEDA/KiCad's `R`), then click to drop it in place.
7. Press **Esc** to stop placing.

Repeat for every part in the BOM (§4.3) — search the exact part number for the confirmed parts, or the LCSC code where one's given (e.g. `C599541` for the output cap, `C5933665` if using the A759 alternative).

**Once every part is placed:**

1. Wire the power path: J1 → U2 (in series — see §4.6) → C1 → L1 → switch node → D1 → C2 → U3 (in series) → J2.
2. Wire the gate path: J3 → U1 → Q1 gate.
3. Wire the I2C path: U2 and U3's SDA/SCL/VCC/GND → J4. Shared bus, not point-to-point — both sensors' logic pins land on the same two signal nets.
4. Label nets clearly (`VIN`, `SW_NODE`, `VOUT`, `GATE`, `SDA`, `SCL`).
5. Run ERC. Fix anything it flags before moving to layout.

## 4.5 Component Placement

This is where most of the actual engineering judgment in this board lives. The governing constraint: **the power loop (C1 → L1 → switch node → D1 → C2) is the highest-current, highest-frequency path on the board** — parasitic inductance from long traces here directly costs efficiency and can cause ringing. Everything else is secondary to keeping that loop physically tight.

- **Q1, L1, D1 close together**, forming as small a physical loop as the footprints allow.
- **U1 as close as possible to Q1's gate pin** — gate trace length matters more than almost anything else on this board; a long gate trace adds inductance that slows switching and risks instability.
- **C3/C4 directly adjacent to U1's supply pins** — a bypass cap that isn't physically close to what it's decoupling isn't doing its job.
- **U2 and U3 sit outside the tight loop**, at the loop's edges where they naturally break the input and output traces.
- **J1/J2 at the board edges**, in line with the power path's natural entry/exit points.
- **Ground/thermal copper pour** under Q1 and U1 — double duty as a low-impedance return path and heat spreading for Q1's thermal pad.

## 4.6 Series vs. Parallel — Sensor Wiring

U2 and U3 are current sensors — they have to be spliced *into* the power path (current flows through them), not tapped onto it at one point. Cut the trace, route both ends to the module's `IN+`/`OUT+` pins. Their I2C pins (SDA/SCL/VCC/GND) are the opposite — shared bus, same two signal nets as every other I2C device in the system, not broken into anything.

**Address jumpers:** confirm U2's A0/A1 jumper gives `0x43` and U3's gives `0x41` before mounting — wrong jumper means an address collision or a silently missing sensor.

**Pull-up resistors:** each CJMCU-226 module typically ships with its own onboard I2C pull-ups. With multiple sensors sharing one bus, these can stack and push combined resistance too low. Only one set of pull-ups should be active per bus — check whether each module has a way to disable its own (often a trace to cut or a jumper) if more than one is on the same bus.

## 4.7 Routing

1. **High-current path (J1→U2→C1→L1→switch node→D1→C2→U3→J2)**: size the trace width (or use a copper pour) for the real current this board carries — use Altium's trace width calculator against your actual current, not a default signal-trace width.
2. **Gate trace (U1→Q1)**: short and direct matters more than width here — low current, high frequency.
3. **Ground plane/pour** on at least one layer.
4. Run DRC. Fix clearance/unrouted-net issues before generating fab files.

## 4.8 Thermal (Q1)

Pull the TP65H150G4PS datasheet and check its thermal pad guidance. GaN switches at this power level typically need an exposed thermal pad soldered to a copper area, with thermal vias down to a ground/heat-spreading plane on another layer. Skipping this is the classic cause of a board that works fine on the bench for a few minutes, then overheats once run continuously.

## 4.9 Fabrication

Once DRC is clean: export Gerbers/ODB++ (no one-click JLCPCB integration in Altium, unlike EasyEDA), upload to your fab house of choice (JLCPCB, PCBWay, OSH Park all accept standard Gerber uploads). Double-check board dimensions, layer count, and quantity on the fab house's order page before finalizing — cheaper to catch a mistake there than after boards arrive.

**Before ordering: bench-test the circuit on the Vector Electronics 8029 perfboard first.** A wiring mistake on a fabricated board costs real money and days of turnaround; the same mistake on perfboard costs you re-soldering one joint.

## 4.10 Full Connection List

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
