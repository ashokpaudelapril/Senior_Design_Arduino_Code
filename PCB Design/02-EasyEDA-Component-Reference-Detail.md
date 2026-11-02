# 2. EasyEDA Component Reference — Detailed Appendix

**Supplementary detail to `01-EasyEDA-Design-Walkthrough.md`** — that file is the actual start-to-finish sequence; this one goes deeper on component search/placement/caution specifics than fits there. Assumes EasyEDA Pro, per `01`'s §1.1.

## 2.1 Account and Project Setup

1. Go to [easyeda.com](https://easyeda.com) and sign in (or create a free account if you haven't).
2. Click **New Project** (or **File → New → Project**).
3. Name it something clear, e.g. `Boost_Converter_PCB`.
4. Inside the project, click **New → Schematic** to create your schematic sheet — this is where you build the circuit before laying out the physical board.

## 2.2 Build the Schematic

### 2.2.1 Full Component List for This Board

Every part that physically goes on the boost converter PCB — pulled from `01-EasyEDA-Design-Walkthrough.md` §1.3 and `03-Sensor-Placement-and-Design.md` §3.1. The isolation diode (SB540) is listed in the materials doc but is **not** on this board — it sits upstream, near the solar/wind wiring — so it's excluded here.

| Ref. designator | Part | Part number | Qty | Role |
|---|---|---|---|---|
| Q1 | GaN switch | Renesas TP65H150G4PS | 1 | Main switching element |
| D1 | Boost diode | onsemi 1N5822, LCSC `C3757288` (40V 3A Schottky, DO-201AD) | 1 | Rectifies switch-node output to VOUT |
| L1 | Inductor | Vishay IHTH0750IZEB470M5A, 47µH | 1 | Energy-storage element |
| U1 | Gate driver | Microchip TC4427AEPA | 1 | Drives Q1's gate |
| C1 | Input capacitor | Panasonic EEU-FR1E471, 470µF 25V | 1 | Input-side bulk filtering |
| C2 | Output capacitor | KEMET A755KS227M1EAAE025, 220µF 25V | 1 | Output-side bulk filtering |
| C3 | Bypass cap | TDK FG28X7R1H104KNT00, 0.1µF 50V X7R | 1 | Gate driver supply decoupling, close to U1 |
| C4 | Bypass cap | TDK FG24X7R1H105KRT00, 1µF 50V X7R | 1 | Gate driver supply decoupling, close to U1 |
| U2 | Current/voltage sensor | CJMCU-226 (INA226), R010 shunt | 1 | Common Bus sense (input side, addr `0x43`) — see `03-Sensor-Placement-and-Design.md` §3.3 |
| U3 | Current/voltage sensor | CJMCU-226 (INA226), R010 shunt | 1 | Battery sense (output side, addr `0x41`) — see `03-Sensor-Placement-and-Design.md` §3.3 |

**Also needed, not in the schematic parts search (board-edge connectors):**

| Item | Part number | Purpose | Notes |
|---|---|---|---|
| Input power connector | Phoenix Contact 1729128 (2-position, 5.08 mm PCB terminal block) | Brings the combined solar+wind DC bus onto the board | Confirmed/ordered (10 pcs) — side-entry, screw-terminal, sized for input-side current (see §2.4) |
| Output power connector | Phoenix Contact 1729128 (same part) | Carries charging current off the board to the battery | Same part as input, from the same ordered stock |
| Gate-drive signal header | Not yet selected | Brings the PWM gate-drive signal in from the microcontroller | Low-current signal header, 2-pin (signal + ground) |
| Sensor headers (×2) | Not yet selected | Connects U2/U3 to the shared I2C bus (SDA/SCL) if socketed rather than edge-mounted | See `03-Sensor-Placement-and-Design.md` §3.3 for the socketed-vs-edge-mounted tradeoff |

Work through this list one part at a time in EasyEDA:

1. Click the **Library** icon (usually on the left sidebar) to open the parts search panel.
2. For each part, search its exact part number (not a generic description) — e.g. `TP65H150G4PS`, `C3757288`, `IHTH0750IZEB470M5A`, `TC4427AEPA`, `EEU-FR1E471`, `A755KS227M1EAAE025`, `FG28X7R1H104KNT00`, `FG24X7R1H105KRT00`, `INA226` (for both U2 and U3), `1729128` (for both the input and output connectors). If `1729128` isn't in EasyEDA's own library, pull the footprint from DigiKey's footprint export or SnapEDA. For parts without a confirmed LCSC `C`-number yet, verify the search result matches before placing it — see `05-Wiring-Reference-and-EasyEDA-Basics.md` §5.3.1.
3. If a search returns no result, widen it to just the base part number (drop letter suffixes) — manufacturers sometimes list a slightly different exact variant in the library than what's on the BOM. Confirm voltage/current ratings still match before using a near-match.
4. If a part genuinely isn't in EasyEDA's library, fall back to SnapEDA or Ultra Librarian (both export directly to EasyEDA format) — see `01-EasyEDA-Design-Walkthrough.md` §1.3 for links.
5. Drag each part onto the schematic sheet.
6. **Wire the circuit to match `boost converter ltspice.png`** — same topology: input source → inductor → switch node (GaN switch drain + diode anode) → diode → output capacitor/load, with the gate driver feeding the GaN switch's gate, and the bypass capacitors placed close to the gate driver's supply pins (standard practice, not shown in the simplified LTspice sim but expected on a real board).
7. Add power/ground symbols and label your nets (e.g., `VIN`, `SW_NODE`, `VOUT`, `GATE`) — makes the next steps much easier to follow.
8. Run **Design → Electrical Rule Check (ERC)** (or similar, under the Design/Tools menu) to catch unconnected pins or conflicts before moving to layout.

## 2.3 Convert to PCB Layout

1. With the schematic open, click **Design → Convert Schematic to PCB** (exact menu wording may vary slightly by EasyEDA version).
2. EasyEDA creates a new PCB sheet with all your parts' footprints already placed in a pile, connected by thin "ratsnest" lines showing what needs to connect to what.
3. Set your board outline first — **Place → Board Outline** (or draw a rectangle on the Board Outline layer) — this is the physical edge of your PCB. Keep it as small as reasonably possible; board cost scales with area.
4. Drag each footprint into a sensible physical position. For a switching power converter specifically:
   - Keep the GaN switch, diode, and inductor physically close together — this loop (input cap → inductor → switch node → diode → output cap) is the highest-current, highest-frequency path, and parasitic inductance from long traces here directly hurts efficiency and can cause ringing/EMI.
   - Keep the gate driver close to the GaN switch's gate pin — a long gate trace adds inductance that slows switching and can cause instability.
   - Place the two bypass capacitors directly adjacent to the gate driver's power pins.

### 2.3.1 Per-Component: Search Term, Placement, and What to Watch For

One reference table covering every part from §2.2.1 — what to search for its footprint, where it physically goes on the board, and the specific mistake to avoid for that part.

| Ref. | Search term | Where to place | Watch out for |
|---|---|---|---|
| Q1 (GaN switch) | `TP65H150G4PS` | Center of the tight switching loop, right next to L1 and D1 | Needs an exposed thermal pad + thermal vias (§2.5) — don't treat it as a normal small-signal footprint. Confirm pin/pad orientation against the real datasheet pinout before trusting the library footprint blindly; GaN packages don't always match standard MOSFET pin layouts. |
| D1 (boost diode) | `1N5822` | Directly between the switch node and the output capacitor | Orientation matters — reversed polarity effectively shorts the switch node on every pulse. Keep lead/trace length to this part minimal; it's inside the high-frequency loop. |
| L1 (inductor) | `IHTH0750IZEB470M5A` | Between the input capacitor and the switch node, closing the tight loop with Q1/D1 | This is a physically large part — verify the library footprint's pad spacing matches the real package dimensions from Vishay's datasheet rather than assuming it's correct. Its magnetic field can couple into nearby traces, so keep it away from the gate-drive trace and the sensor I2C lines. |
| U1 (gate driver) | `TC4427AEPA` | As close as physically possible to Q1's gate pin | A long gate trace adds inductance that slows switching and risks instability — this is the single highest-priority placement constraint on the board. Also confirm its output drive voltage actually matches what Q1's gate needs. |
| C1 (input cap) | `EEU-FR1E471` | Right at the input side, immediately before L1 | Electrolytic — has polarity. Reversed, it can fail or vent. It's physically large; leave room for it before placing smaller parts around it. |
| C2 (output cap) | `A755KS227M1EAAE025` | Right at the output side, immediately after D1 | Same polarity caution as C1. |
| C3/C4 (bypass caps) | `FG28X7R1H104KNT00` / `FG24X7R1H105KRT00` | Directly adjacent to U1's supply pins — millimeters away, not just "somewhere nearby" | Small ceramic parts are easy to place too far from the pin they're decoupling, which defeats the point. Double-check which value goes on which supply pin per the TC4427AEPA datasheet. |
| U2/U3 (INA226 sensors) | `INA226` | U2 breaks the input trace before L1 (Common Bus, `0x43`); U3 breaks the output trace after C2 (Battery, `0x41`) — full detail in `03-Sensor-Placement-and-Design.md` §3.3 | Keep both sensor break-points **outside** the tight Q1/D1/L1 loop. Watch for I2C pull-up resistors stacking if other modules share the bus (`03-Sensor-Placement-and-Design.md` §3.5). Confirm each module's A0/A1 address jumper before mounting — wrong jumper means wrong I2C address and a silent sensor failure. |
| Connectors (input/output/gate/sensor headers) | Not in the parts search — generic header/terminal footprints | Board edges, positioned so the attached wire (22 AWG per the materials list) reaches its destination without strain | Size the connector's current rating to what it actually carries, not just physical fit. Silkscreen polarity/pinout clearly before populating — easy to mix up once the board is built and no longer looks like the schematic. |

**General cautions that apply across all of the above:**
- Don't trust a library footprint at face value for any part with a nonstandard or thermally-demanding package (Q1 and L1 especially) — cross-check against the real datasheet footprint drawing.
- Keep the ERC (§2.2, step 8) and DRC (§2.4, step 6) checks as your safety net, but they only catch electrical/clearance issues — polarity orientation and physical footprint accuracy are on you to verify by eye.
- If anything in this table conflicts with what the actual datasheet says once you pull it up, trust the datasheet — these placements are the general switching-converter layout pattern, not a substitute for checking the specific part.

## 2.4 Route the Traces

1. Click the **Route** tool (or press the keyboard shortcut EasyEDA lists for it).
2. Click a ratsnest line's starting pad, then click through to its destination to lay a trace.
3. **Trace width matters here, not just connectivity** — the high-current path (input cap → inductor → switch node → diode → output cap) needs a wide trace (or a copper pour/plane) to handle the real current without excessive heating or voltage drop. Use EasyEDA's trace width calculator (often built into the routing tool, or check the datasheet-driven IPC-2221 width tables) based on the actual current this board carries, not a default thin signal trace.
4. Route the gate signal as a short, direct trace — this one cares more about length/parasitic inductance than width, since it's low current but high-frequency switching.
5. Add a ground plane/pour (**Place → Copper Area** or similar) on at least one layer, if your board design supports it — reduces ground loop issues and helps with thermal dissipation for the GaN switch.
6. Run **Design Rule Check (DRC)** before finishing — catches trace clearance violations, unrouted nets, and other manufacturability issues.

## 2.5 Thermal Considerations for the GaN Switch

Pull up the TP65H150G4PS datasheet (per `01-EasyEDA-Design-Walkthrough.md` §1.5) and check its recommended thermal pad/footprint guidance — GaN switches in this power range typically need:
- An exposed thermal pad soldered to a copper area on the PCB, not just the signal pins
- Thermal vias underneath that pad, connecting to a ground/heat-spreading plane on another layer, if the datasheet recommends it

Skipping this is a common cause of a board that works briefly on the bench, then overheats and fails once run continuously — worth getting right the first time rather than discovering it after fabrication.

## 2.6 Generate Manufacturing Files and Order

1. Once DRC passes clean, click **Fabrication → PCB Fabrication File (Gerber)** (exact wording may vary).
2. EasyEDA will either let you download the Gerber files directly, or — since EasyEDA/JLCPCB/LCSC are sister companies — offer a **one-click "Order at JLCPCB"** option that uploads the design and takes you straight to their ordering page with your dimensions/layer count pre-filled.
3. Before finalizing an order: double-check board dimensions, layer count, and quantity on the fab house's order page — these directly affect cost, and it's much cheaper to catch a mistake here than after boards arrive.
4. If you'd rather use a different fab house (PCBWay, OSH Park), download the plain Gerber/drill files instead and upload them through that fab house's own site.

## 2.7 Before You Order — Bench-Test First

Per `01-EasyEDA-Design-Walkthrough.md` §1.3, the materials list includes a **Vector Electronics 8029 perfboard** as a prototyping alternative. Strongly worth building this circuit there first (or verifying it on a breadboard/perfboard in some form) before paying for PCB fabrication — a fabricated board with a wiring mistake is much more expensive and slower to fix than a perfboard one.
