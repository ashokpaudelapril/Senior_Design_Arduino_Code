# 6. KiCad Beginner Walkthrough — Boost Converter PCB

A parallel manual alongside `04-Altium-Design-Walkthrough.md` — Altium stays the active tool for this project; this is a from-scratch, no-assumptions walkthrough for KiCad specifically (free, different enough from both EasyEDA and Altium that it needs its own guide). Every real part number and placement decision is the same as already confirmed in `01`/`02`/`03`/`05` — only the button-clicking is different here.

## 6.1 What KiCad Actually Is

KiCad is a free program you install on your computer, like installing any other app. It does the same overall job as EasyEDA or Altium — let you draw a circuit and turn it into a real PCB — just with its own layout and its own way of doing things.

**To install it:** go to [kicad.org](https://www.kicad.org), click Download, pick your operating system (Windows/Mac/Linux), and install it the normal way you'd install any program. It's completely free, no account or license needed.

## 6.2 The Three Windows You'll Be Using (understand this before anything else)

This is the single most important thing to get straight before starting, because it trips up almost everyone new to KiCad: **KiCad isn't one window, it's three**, and you move between them constantly.

| Window | What it's called | What it's for |
|---|---|---|
| **Project Manager** | KiCad's main launcher | The first thing that opens. It's basically a file browser for your project — you click into the other two windows from here. |
| **Schematic Editor** | Also called "Eeschema" in KiCad's own documentation | Where you draw the circuit — place parts, draw wires between them. This is like drawing a map of connections, not the physical board yet. |
| **PCB Editor** | Also called "Pcbnew" | Where you lay out the physical board — drag parts into real positions, draw copper traces. This comes *after* the schematic. |

You'll do almost everything in this guide inside the **Schematic Editor**. Don't worry about the PCB Editor yet — that's a later step, covered briefly at the end.

## 6.3 Creating Your Project

1. Open KiCad. The **Project Manager** window appears — this is just a launcher, not where you'll draw anything.
2. Click **File → New Project** (top-left menu).
3. Give it a name, like `Boost_Converter_PCB`, and pick a folder to save it in.
4. KiCad creates a project file and shows you a small window listing a schematic file and a PCB file, both currently empty.
5. **Double-click the schematic file** (it'll have a `.kicad_sch` extension) — this opens the Schematic Editor, the window you'll spend most of your time in.

## 6.4 Placing Your First Part — the Actual Clicks

This is the part you specifically asked about: where to search, what to search, how to select.

1. With the Schematic Editor open, look at the toolbar on the **right side** of the window (this is where KiCad's part-placement tools live, different from EasyEDA's left-side panel).
2. Find the icon that looks like a small chip/IC symbol — this is **"Add Symbol."** Click it. (Or press the keyboard shortcut **`A`** on your keyboard, which does the same thing.)
3. A new window pops up called the **Symbol Chooser** — this is KiCad's search tool, equivalent to EasyEDA's library search panel.
4. At the top of this window is a **search box**. Click into it and type the part name.
5. As you type, matching results appear in a list below. Click once on a result to see a preview of its symbol on the right side of that window.
6. Click **OK** (or double-click the result) to pick it.
7. Your cursor now "carries" the part — move your mouse over the schematic sheet and **click once** to drop it in place.
8. Press **Escape** when you're done placing that part, or right-click to get more options.

**To rotate a part** before or after placing it: select it, then press **`R`** on your keyboard — same shortcut idea as EasyEDA.

## 6.5 The Big Difference From EasyEDA: No Built-In Real-Parts Library

This is important to understand clearly, because it changes how you get your *specific* real components.

EasyEDA's library is directly linked to LCSC's actual stock — searching a real part number often just works. **KiCad's built-in library is different: it only has generic, generic-shaped symbols** (a generic diode shape, a generic resistor shape, a generic 2-pin connector shape) — it is **not** linked to any distributor, and it won't recognize a specific manufacturer part number like `C3757288` or `TP65H150G4PS` by default.

So for this board's *specific, real* parts, you have two paths:

### Path A: Generic parts (resistors, capacitors, basic connectors)

For simple things like the bypass capacitors or a generic header, KiCad's own built-in library already has what you need — just search generic terms:
- For a capacitor: search `C` or `Capacitor`
- For a generic 2-pin connector/header: search `Conn_01x02` (KiCad's own naming convention for a 1-row, 2-pin connector) — this is exactly the kind of generic header used for the sensor connection points, covered in §6.7 below.

### Path B: Specific real parts (the GaN switch, the exact diode, the exact connector)

For parts where the *exact* real component matters — like the onsemi 1N5822 diode (LCSC `C3757288`) or the Phoenix Contact 1729128 connector — KiCad's built-in library won't have them. You need to pull them in from outside, the same SnapEDA/Ultra Librarian fallback already mentioned in `01-EasyEDA-Design-Walkthrough.md` §1.3, just used differently here:

1. Go to [snapeda.com](https://www.snapeda.com) in your browser.
2. Search the exact part number (e.g. `C3757288`, or the manufacturer's own part number if the LCSC code doesn't turn up a result — e.g. search the raw onsemi part number for the diode).
3. On the part's page, look for a **Download** button, and pick **KiCad** as the format (SnapEDA lets you choose which EDA tool's format you want — make sure KiCad is selected, not EasyEDA or Altium).
4. This downloads a small set of library files (symbol + footprint) to your computer.
5. Back in KiCad's Schematic Editor, go to **Preferences → Manage Symbol Libraries** (or **File → Add Library**, depending on your KiCad version), and point it at the folder where you saved those downloaded files. This tells KiCad "also look here" in addition to its own built-in library.
6. Now when you search for that part in the Symbol Chooser (§6.4), it'll show up, because KiCad now knows about it.

This extra step (download from SnapEDA, then tell KiCad where to find it) is the main thing that's more work in KiCad than EasyEDA — EasyEDA does this automatically for LCSC-stocked parts, KiCad doesn't do it for anything by default.

## 6.6 Full List of What to Search — One at a Time

Same real BOM as `02-EasyEDA-Component-Reference-Detail.md` §2.2.1. For each one, search the term below in the Symbol Chooser (§6.4); if it's not found in KiCad's own library, that means it needs the SnapEDA path (§6.5, Path B).

| Ref. | Part | What to search | Likely needs SnapEDA? |
|---|---|---|---|
| Q1 | GaN switch | `TP65H150G4PS` | Yes — specific part, check SnapEDA |
| D1 | Boost diode (onsemi) | `C3757288` or `1N5822` | Yes — likely needs SnapEDA for the exact footprint |
| L1 | Inductor | `IHTH0750IZEB470M5A` | Yes |
| U1 | Gate driver | `TC4427AEPA` | Yes |
| C1 | Input cap | `EEU-FR1E471` | Maybe — try KiCad's generic electrolytic cap first if the exact footprint doesn't matter much, otherwise SnapEDA |
| C2 | Output cap | `A755KS227M1EAAE025` | Same as above |
| C3 | Bypass cap | `FG28X7R1H104KNT00`, or just a generic `C_Small` from KiCad's own library (0.1µF ceramic is common enough that exact footprint usually doesn't matter) | Usually no |
| C4 | Bypass cap | `FG24X7R1H105KRT00`, or generic `C_Small` | Usually no |
| J1 | Input connector | `1729128`, or fall back to KiCad's generic `Conn_01x02` if the exact footprint isn't critical yet | Yes, for the exact real footprint |
| J2 | Output connector | Same as J1 | Yes |

## 6.7 Setting Up the Sensors — Same Idea as Before, KiCad-Specific Clicks

Per `05-Wiring-Reference-and-EasyEDA-Basics.md` §5.1-§5.2: the CJMCU-226 sensor modules aren't something you search for by name — they're separate physical boards connected by wire. What you place on your schematic is a **generic connector symbol** representing each connection point, not the sensor itself.

In KiCad specifically:

1. Open the Symbol Chooser (§6.4).
2. Search `Conn_01x02` for a simple 2-pin connection point (used for each power-sensing break point, `IN+`/`OUT+`).
3. Search `Conn_01x04` for a 4-pin connector, useful for the shared I2C bus connection (`VCC`, `GND`, `SDA`, `SCL` together in one connector if you want them grouped).
4. Place two of the 2-pin ones for each sensor's power break (one for U2, one for U3) — label them clearly using KiCad's **label tool** (press **`L`** on your keyboard with nothing selected, or find "Add Label" in the Place menu) so you can tell `U2_IN` from `U2_OUT` once it's wired.
5. Wire these in exactly the same way described in `05-Wiring-Reference-and-EasyEDA-Basics.md` §5.2's connection list — the electrical connections don't change, only the tool drawing them does.

## 6.8 Wiring Parts Together

Once a part is placed, connecting it to another part:

1. Click the **wire tool** in the right-side toolbar (looks like a zigzag line), or press **`W`**.
2. Click once on a pin of one part, then click once on a pin of another part — KiCad draws a wire connecting them.
3. Press **Escape** to stop drawing wires.
4. To label a wire with a net name (like `VIN`, matching `05-Wiring-Reference-and-EasyEDA-Basics.md` §5.4's net-labeling step): select the wire, press **`L`**, type the name.

## 6.9 After the Schematic — Brief Note on What Comes Next

Once every part from §6.6 is placed and wired according to the connection list in `05-Wiring-Reference-and-EasyEDA-Basics.md` §5.2, the next step is converting to the PCB Editor (**Tools → Update PCB from Schematic**, inside the Schematic Editor) — this is the KiCad equivalent of EasyEDA's "Convert Schematic to PCB" step. The placement reasoning from `02-EasyEDA-Component-Reference-Detail.md` §2.3/§2.3.1 (tight switching loop, gate driver proximity, sensor break points outside the loop) applies exactly the same way here — that's a separate, later walkthrough once the schematic side is actually done.
