# NRPN to CC — VST3 (MIDI instrument for Cubase)

A VST3 plugin (VST3 only) of type **instrument (VSTi)** that receives MIDI, **passes through**
everything that is not NRPN, and **converts NRPN messages into CC**, sending the result to its
MIDI output.

> **Why an instrument and not a "MIDI effect"?** Cubase does **not** load VST3 MIDI-effect
> plugins in its MIDI Insert slots (those accept only Cubase's native MIDI plug-ins), and as an
> audio insert it would not receive the track's MIDI. An **instrument**, instead, is fed the
> whole MIDI stream of the track: so the plugin receives the NRPN messages and can emit the CC
> on its output, which you route to a virtual instrument or an external MIDI track. As a bonus,
> the controller's MIDI port is opened by Cubase, not by the plugin: no port conflict.

## What it does

- Receives NRPN (CC 99/98 = parameter number, CC 6/38 = value).
- Generates a CC with **automatic mapping**: `CC number = NRPN parameter number` (low 7 bits).
- From the interface you can set:
  - **Input channel** (Omni or 1–16): which channel to listen to.
  - **Output channel** (Same as input or 1–16).
  - **Output format**:
    - `7-bit auto-range` (default) → a single CC. The plugin **learns**, per NRPN (and per
      channel), the maximum value it receives and scales it to **0–127**. Generic, with no
      device-specific tables: **switches** (max 1) become 0/127, **knobs** become a smooth
      0–127. Do **one full sweep** of each knob to calibrate it; the learned ranges are
      **saved in the project**. The **Reset learning** button clears them.
    - `7-bit fixed scale` → a single CC scaled with a fixed maximum (**Max (fixed scale)**),
      useful when you know the range and want deterministic behavior.
    - `14-bit (MSB+LSB)` → two CCs: `CC#n` = MSB and `CC#n+32` = LSB (high resolution).
  - **Max (fixed scale)**: used only by the "fixed scale" mode.
  - **Pass through original NRPN**: if enabled, the raw NRPN messages are forwarded too, in
    addition to the CC.
  - **Filter repeated CC** (default ON): in 7-bit mode, avoids sending consecutive CCs with the
    same value (useful because scaling 0–255 → 0–127 makes many steps map to the same CC).
  - **Monitor**: shows the last NRPN received, the CC sent, and a **bar** of the outgoing CC value.

> Note on **MIDI ports**: a VST3 plugin inserted in a track does not open hardware ports. The
> physical port (your MIDI interface) is chosen in the **DAW's track routing**. In the plugin
> you choose the **channel**; the port is handled by Cubase.

---

## Prerequisites (Windows)

You need a C++ compiler and CMake:

1. **Visual Studio 2022 or later, Community edition** (free) — during installation select the
   **"Desktop development with C++"** workload. It includes the MSVC compiler, the Windows SDK
   and the "C++ CMake tools" (hence CMake).
   Download: https://visualstudio.microsoft.com/downloads/
2. **Git**.

(Alternatively you can install CMake standalone from https://cmake.org/download/ plus the Build
Tools for Visual Studio, but the full IDE is the simplest route.)

---

## Building

Open the **"Developer PowerShell for VS"** (from the Start menu) and go to the project folder:

```
cd "path\to\NrpnToCc"
```

Configure (the first time this downloads JUCE automatically: a few minutes):

```
cmake -B build -G "Visual Studio 17 2022" -A x64
```

> On Visual Studio 2026 use the generator `"Visual Studio 18 2026"` instead.

Build in Release:

```
cmake --build build --config Release
```

The compiled plugin is at:

```
build\NrpnToCc_artefacts\Release\VST3\NRPN to CC.vst3
```

---

## Installation

The build does **not** copy the plugin automatically. Copy the `NRPN to CC.vst3` folder into one
of these locations:

- `C:\Program Files\Common Files\VST3`  (all users — requires admin)
- `%LOCALAPPDATA%\Programs\Common\VST3`  (current user only — no admin)

---

## Using it in Cubase (recommended routing: instrument rack)

Goal: **controller → plugin → (pass-through + CC) → virtual instrument / external instrument.**

1. Rescan the VST3 plugins (Studio → VST Plug-in Manager → Update). The plugin appears among the
   **instruments** (Instrument), not among the inserts.
2. Open the **instrument rack**: `Studio → VST Instruments` (**F11**).
   Add **NRPN to CC**. When Cubase asks to create an associated MIDI track, accept.
3. **"IN" track** (the one created in step 2, pointing at the plugin):
   - MIDI input = your controller (or "All MIDI Inputs").
   - Enable **Monitor** (the speaker icon) or record-enable, so MIDI reaches the plugin.
   - Open the plugin GUI: when the controller sends NRPN, the **Monitor** must light up.
4. **Enable the plugin's MIDI output**: in the instrument rack, on the NRPN to CC slot, enable
   the plug-in MIDI output (in many versions there is a small "MIDI Out"/connector control on the
   slot; its position varies with the Cubase version).
5. **"OUT" track** (new MIDI track):
   - MIDI input = **NRPN to CC** (the plugin's MIDI output now appears in the input list).
   - MIDI output = the desired destination: a **virtual instrument** (another instrument track)
     or an **external MIDI port** (hardware synth).
   - Enable **Monitor** on this track so the converted stream reaches the destination.

Final flow: `controller → IN track → NRPN to CC → OUT track → instrument/hardware`.

**Cubase 15 Pro** — specific notes:
- Use the **rack** (F11), not a plain instrument track: only the rack instance reliably exposes
  the plugin's MIDI output as a selectable *input* on another track.
- In Cubase 15 the instrument's MIDI output appears **automatically** among the MIDI inputs: no
  global switch is needed.
- If "NRPN to CC" does not appear among the inputs in step 5: close/reopen the routing window or
  reload the project after adding the instrument in the rack.

### MIDI behavior

- **Notes, pitch bend, regular CC, etc.** → pass through unchanged.
- **NRPN** (CC 99/98/6/38) → converted to **CC** (automatic mapping: NRPN number → CC number).
- Raw NRPN messages are **absorbed** (not forwarded), unless you enable *Pass through original
  NRPN* in the GUI.

---

## Project structure

```
NrpnToCc/
├─ CMakeLists.txt          # VST3-only build, downloads JUCE via FetchContent
├─ LICENSE                 # GNU GPL v3
└─ Source/
   ├─ PluginProcessor.h/.cpp   # NRPN -> CC logic
   └─ PluginEditor.h/.cpp      # interface (channels, modes, monitor)
```

---

## License

This project is distributed under the **GNU General Public License v3.0** (see [LICENSE](LICENSE)).

The plugin is built with [JUCE](https://juce.com), used under its **GPLv3** license. As a result
this project is released as GPLv3 free software. JUCE is **not** included in the repository: it is
downloaded automatically by CMake (FetchContent) at build time.

© GPR Music Project.
