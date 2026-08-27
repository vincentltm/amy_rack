# UI Navigation & Visualizer Reference

AMY Rack features a streamlined, high-density interface optimized for a 128×128 pixel SH1107 OLED display and a single click rotary encoder.

---

## 1. Display Layout Structure

```
+----------------------------------------------------+  Y = 0
| [MAIN] [SYNTH] [DRUM] [ENV] [FX] [MIDI]            |  Top Header (Status + MIDI)
+----------------------------------------------------+  Y = 14
|                                                    |
|            DYNAMIC VISUALIZER ZONE                 |  Visualizer Zone (Y: 14 to 61)
|      (FM Tree / Juno Sliders / 8-Pad Grid / KB)    |
|                                                    |
+----------------------------------------------------+  Y = 62
| [MAIN] [SYNTH] [DRUM] [ENV] [FX] [MIDI]            |  Tab Bar (Y: 63 to 74)
+----------------------------------------------------+  Y = 75
| >Synth: JUNO-106                                   |
|  Patch: 00:Warm Brass                              |  5-Row Parameter
|  Drums: TR-808                                     |  Scroll List (Y: 76 to 127)
|  Synth Vol: 85 %                                   |
|  Drum Vol: 85 %                                    |
+----------------------------------------------------+  Y = 127
```

---

## 2. Current Tab Configuration

| Tab | Visualizer | Parameters & Purpose |
|---|---|---|
| **`[MAIN]`** | Active Synth Visualizer | **`Synth`** (DX7, Juno, Analog, Sampler, Piano), **`Patch`** (0..127), **`Drums`** (9 kits), **`Synth Vol`**, **`Drum Vol`**, **`Audio In`**, **`Voice Mode`**, **`Glide`** |
| **`[SYNTH]`** | Active Synth Visualizer | Engine-specific controls: DX7 Operators, Juno DCO/PWM/HPF/VCF, Analog Waveforms/Detune, Sampler Record/In Vol/Trim/Gain |
| **`[DRUM]`** | **8-Pad Drum Grid** (`[BD][SD][CH][OH] / [CP][TM][CB][MA]`) with hit flash | **`Drums`** (9 Kits: TR-808, 909, Linn 9000, MR-12, Tokyo Synth, Power, Percussion, etc.), **`Drum Vol`**, **`Kick Tune`**, **`Snare Tune`**, **`Tom Tune`**, **`Drive`** |
| **`[ENV]`** | **Dual Plot**: VCF Bode Plot + ADSR Curve | **`Cutoff`**, **`Resonance`**, **`Attack`**, **`Decay`**, **`Sustain`**, **`Release`** |
| **`[FX]`** | **3-Way FX Plot**: Chorus Waves + Reverb Tail + Delay Echoes | **`Chorus`**, **`Reverb`**, **`Rev Damp`**, **`Delay Mix`**, **`Delay Time`**, **`Delay Fdbk`** |
| **`[MIDI]`** | **14-Key Animated MIDI Keyboard** | **`Synth MIDI`** (Ch 1..16, Omni), **`Drum MIDI`** (Ch 10 default, Ch 1..16), **`CV 1 Out`**, **`CV 2 Out`** |

---

## 3. Proposed Hierarchical 2-Level UI Architecture (Next Phase)

To accommodate complex modules like **16-Step Melodic Sequencer**, **16-Step Drum Step Sequencer**, **Polyphonic Arpeggiator**, and **Custom FX Routing**, the UI will transition to a hierarchical 4-Category model with Sub-Pages:

```
[SYNTH]                     [DRUMS]                 [FX]                [SETUP]
  ├── Preset / Engine         ├── Kit & 8-Pad Grid    ├── Chorus          ├── MIDI Routing (Synth/Drum)
  ├── Filter & ADSR           ├── Tuning & Drive      ├── Reverb          ├── CV / Gate In & Out
  ├── 16-Step Sequencer / Arp └── 16-Step Drum Grid   ├── Delay           └── Hardware Calibration
  └── Voice / Glide                                   └── Live Audio In
```

### Proposed Navigation Behavior:
1. **Top Bar**: Shows the 4 primary hubs: `[SYNTH]`, `[DRUMS]`, `[FX]`, `[SETUP]`.
2. **Sub-Page Dots / Pill Selector**: Clicking top bar or holding encoder allows scrolling between sub-pages.
3. **Dedicated Sequencer Pages**:
   - `SYNTH SEQ`: 16-step pitch & velocity editor with visual running step cursor.
   - `DRUM SEQ`: 16-step drum pattern grid (BD, SD, CH, OH) with step toggling.
