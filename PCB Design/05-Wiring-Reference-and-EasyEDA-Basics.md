# 5. Wiring Reference and EasyEDA Basics — Boost Converter

This fills in the gaps the other docs assumed you already knew: how the sensors actually connect (series vs. parallel), a full pin-by-pin connection list for the whole board, and the actual EasyEDA mechanics for finding, selecting, and placing parts and setting up the board's input/output connectors.

## 5.1 Series vs. Parallel — Why This Matters for the Sensors

This is the single most important concept to get right, because getting it backwards doesn't just give bad readings — it can be a wiring mistake.

**A current sensor (the INA226/CJMCU-226) must be wired IN SERIES with the power path, not in parallel.**

- **In series** means the current physically flows *through* the sensor. You take the wire that would normally run straight from one point to the next, **cut it**, and connect the two cut ends to the sensor's two power pins (often labeled `IN+`/`OUT+` or similar on the CJMCU-226 module). Current goes in one side, through the module's internal shunt resistor, and out the other side. This is the only way the sensor can actually measure *current* — it has to be in the current's path.
- **In parallel** means connecting both ends to the *same* point without cutting anything — like clipping both leads of a voltmeter onto one wire. That's how you'd measure *voltage* at a point, not current. Wiring a current sensor this way doesn't measure current correctly and isn't what you want here.

Think of it like a water meter on a pipe: to measure water flow, the meter has to be spliced into the pipe so all the water passes through it. You wouldn't just tap a sensor onto the side of the pipe and expect a flow reading.

**The module has two separate groups of pins, and they follow different rules:**

| Pin group | Wiring style | Why |
|---|---|---|
| Power-sensing pins (`IN+`, `OUT+`, and the ground return) | **Series** — spliced into the power path | This is what lets the sensor see the actual current flowing to/from the battery or bus |
| I2C pins (`SDA`, `SCL`, `VCC`, `GND`) | **Parallel / shared bus** — same two wires daisy-chained to every I2C device | I2C is a shared bus by design; every sensor and the touch controller all connect to the *same* SDA and SCL lines, each with its own unique address (0x40/0x41/0x42/0x43) so they don't conflict |

So on this board: the common-bus sensor (U2) and battery sensor (U3) are each **spliced into** the power path (series), while all their I2C pins connect to the **same shared pair of wires** (parallel bus) that also reaches the other sensors and the touchscreen elsewhere in the system.

## 5.2 Full Connection List for This Board

A plain pin-to-pin list of everything that connects to everything else, matching the layout in `boost-layout-artifact.html` and the circuit in `boost converter ltspice.png`.

**Power path (series — current flows through every link in this chain):**

1. `J1` (input connector, pin 1 / VIN+) → `U2` power-in pin
2. `U2` power-out pin → `C1` positive terminal
3. `C1` positive terminal → `L1` pin 1
4. `L1` pin 2 → **switch node** (this one point connects to both of the next two)
5. Switch node → `Q1` drain
6. Switch node → `D1` anode
7. `D1` cathode → `C2` positive terminal
8. `C2` positive terminal → `U3` power-in pin
9. `U3` power-out pin → `J2` (output connector, pin 1 / VOUT+)

**Ground (all of these connect to one shared ground net):**

`J1` pin 2, `C1` negative terminal, `Q1` source, `C2` negative terminal, `J2` pin 2, `U1` GND, `U2` GND, `U3` GND — all tie together to the same ground net/plane.

**Gate-drive signal (separate from the power path above):**

- `U1` output pin → `Q1` gate (short, direct trace — this is the one where trace length matters most)
- `J3` (gate signal header) → `U1` input pin (this is where the PWM signal comes in from the microcontroller)
- `U1` VCC → its own gate-driver supply rail (not the same rail as the boost converter's VIN/VOUT — check the TC4427AEPA datasheet for its supply requirement)
- `C3` and `C4` → both connect straight across `U1`'s VCC and GND pins, physically right next to the chip (this is what "bypass capacitor" means — they sit in parallel with the supply pins, placed as close as possible)

**I2C bus (shared/parallel, as explained in §5.1):**

- `U2` SDA → shared SDA net → `U3` SDA → shared SDA net → `J4` (I2C header, which continues on to the rest of the system's sensors and touch controller elsewhere)
- `U2` SCL → shared SCL net → `U3` SCL → shared SCL net → `J4`
- `U2` VCC/GND and `U3` VCC/GND → shared power/ground for the sensor logic (separate from the high-current power-sensing pins in the table above — the module needs its own small logic supply to talk I2C)

## 5.3 Finding and Placing Parts in EasyEDA (Pro)

**Updated for Pro**, since the project's `.eprj2` file confirms that's what's actually in use (not Standard, as the rest of this doc originally assumed) — the overall idea is the same, but it's a desktop app with its own project tree rather than a single browser tab.

1. **Open your project in the Pro desktop client**, then open the schematic document inside it (Pro projects can hold multiple documents — schematic, PCB, etc. — navigated through a project/document panel, usually down the left side).
2. **Find the parts search panel.** In Pro this is typically a **Library** panel docked on the side of the schematic editor, with its own search box — look for a tab labeled "Library" or a magnifying-glass search icon. If it's not visible, check the **Place** menu at the top for a "Symbol"/"Part" entry that opens it, or look for a toggle in the **View** menu for side panels.
   - Exact panel placement can vary by Pro version — if what you see doesn't match this description, hover over the side-panel icons; the Library one is usually self-labeled.
3. **Type the exact part number** into the search box (e.g. `C3757288` for the onsemi 1N5822, or `1729128` for the Phoenix Contact connector) and press Enter.
4. **Select a result.** Click once on a search result to preview it — you should see the schematic symbol and usually the footprint. Make sure the preview actually matches what you expect (right pin count, right package) before placing it. **If a search for one part number returns a long list of results, don't just pick the top one — see §5.3.1 below.**
5. **Place it on the canvas.** Either drag the result directly onto the schematic sheet, or click it once then click again on the canvas where you want it to land — EasyEDA will show the part "carried" by your cursor until you click to drop it.
6. **Rotate if needed.** With the part selected (or while it's still carried by your cursor before dropping), press `R` to rotate it 90° at a time — useful for lining up pins with the direction your wires need to run.
7. **Press Escape** to stop placing additional copies of the same part once you're done.
8. **Repeat** for each part in the BOM table (`02-EasyEDA-Component-Reference-Detail.md` §2.2.1).

### 5.3.1 Confirming a Search Result Is Actually the Right Part

Searching a part name like `1N5822` or `INA226` returns a long list, because many manufacturers make their own version of the same standard part, in different packages, at different prices — this is exactly what happened with the diode search a few messages back (30 results for one part name). Don't pick the top result by default. Check these, in order:

1. **Do you already have a real LCSC part number (the `C` + digits code)?** This is the strongest check — if your BOM or a prior search already confirmed one (e.g. `C3757288` for the onsemi 1N5822), search that exact code instead of the generic name. An LCSC `C`-number is a unique identifier for one specific manufacturer's specific part — there's no ambiguity once you search by it directly, unlike searching a generic name that many parts share.
2. **No confirmed C-number yet? Check the manufacturer name in the result.** If your BOM or datasheet names a specific manufacturer (e.g. "Renesas" for the GaN switch, "Microchip" for the gate driver), only consider results from that manufacturer — skip unbranded/no-name listings, since you can't verify those against a real datasheet.
3. **Check the package/footprint column matches what you expect.** The same part number often comes in several physical packages (through-hole vs. surface-mount, different case sizes) — these are *not* interchangeable on your board. If you already know which package you're building for (e.g. through-hole for hand-soldering), filter out anything else immediately, even if the part number matches.
4. **Check the listed electrical specs against your confirmed requirement**, when the search results show them (voltage rating, current rating, etc.) — compare against the real number from your BOM or the component's datasheet, not against another search result. If the listing is missing specs entirely, that's a reason to skip it, not assume it's fine.
5. **When still unsure, click into the result for its full detail page** (usually opens a larger preview with a datasheet link) and compare it directly against the actual manufacturer datasheet for your confirmed part — the search list view doesn't always show everything you need to tell two similar listings apart.

If you get stuck narrowing a list like this down yourself, paste the search results here the same way as the diode search — the comparison process is always the same four checks above, whether it's a diode, a connector, or a capacitor.

## 5.4 Setting Up the Board's Inputs and Outputs

The "input" and "output" of the board aren't a special tool feature — they're just the connector parts (`J1`, `J2`) wired in like any other component, placed at the edges of the board. Here's the full process:

1. **Place the connector footprints** (`J1` = Phoenix Contact 1729128 for input, `J2` = same part for output) on the schematic, same as any other part from §5.3.
2. **Wire them in** using the regular wire tool — connect `J1`'s two pins to the start of your power path (into `U2`, and to ground) and `J2`'s two pins to the end of it (out of `U3`, and to ground), following the connection list in §5.2.
3. **Label the nets** (optional, but worth doing): use the **Net Label** tool (usually under the Place menu) to name the wires coming off `J1` as `VIN` and `GND`, and the wires at `J2` as `VOUT` and `GND`. This doesn't change the electrical connection — wires with the same net label are already connected to each other even without a direct line drawn between them — but it makes the schematic far easier to read, especially once you convert it to the PCB layout.
4. **On the PCB side**, after converting the schematic to a PCB (`02-EasyEDA-Component-Reference-Detail.md` §2.3), drag `J1` and `J2`'s footprints to the physical edges of the board outline — left edge for input, right edge for output, matching the layout in `boost-layout-artifact.html`. Those footprint pads are the literal physical points where external wires get screwed into the terminal block once the board is populated.

That's the entire mechanism — there's no separate "define board I/O" step beyond placing, wiring, and positioning these connector parts like you would any other component.
