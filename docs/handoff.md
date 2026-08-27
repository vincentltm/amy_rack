# Developer Handoff & Maintenance Guide

This document serves as the primary onboarding and maintenance reference for developers maintaining or expanding AMY Rack.

---

## 1. Development Environment Setup

### Required Toolchain
1. **Arduino CLI** (or Arduino IDE 2.x):
   ```bash
   brew install arduino-cli
   ```
2. **ESP32 Arduino Core**: Version $\ge 3.3.11$
   Add package index: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. **Core Dependencies**:
   - `AMY Synthesizer` (Version 1.2.4+)
   - `U8g2` (Version 2.35.30+)
   - `Adafruit seesaw Library` (Version 1.7.9+)
   - `Adafruit BusIO` (Version 1.17.4+)

---

## 2. Compilation and Flashing Workflow

### Fast Terminal Commands
```bash
# 1. Compile firmware for AmyBoard target
arduino-cli compile --fqbn esp32:esp32:amyboard firmware/

# 2. Flash to AmyBoard via USB-C
arduino-cli upload -p /dev/cu.usbmodem2101 --fqbn esp32:esp32:amyboard firmware/

# 3. Monitor serial debug output
python3 -c 'import serial; s=serial.Serial("/dev/cu.usbmodem2101", 115200); [print(s.readline().decode("utf-8", "replace").strip()) for _ in iter(int, 1)]'
```

---

## 3. Architecture & Engine State Reference

- **Melodic Synthesizer (`synth = 1`)**:
  - Exposes 5 selectable engines (`DX7`, `Juno-106`, `Analog`, `Sampler`, `Piano`).
  - Controlled on MIDI Channel 1 (or user configured in `[MIDI]` tab).
- **Dedicated Background Drum Machine (`synth = 10`)**:
  - Runs parallel 6-voice polyphonic GM percussion on MIDI Channel 10 continuously.
  - Supports 9 Gamma9001 / 808 drum kits with real-time tuning and drive.
- **Stereo Audio In**:
  - Live hardware monitoring via `amy_config.features.audio_in = 1` and `AUDIO_IN0` oscillator.
  - Passes external audio through master Chorus, Reverb, and Delay.

---

## 4. UI Architecture & Planned Next Phase

The current UI operates on 6 flat tabs (`[MAIN] [SYNTH] [DRUM] [ENV] [FX] [MIDI]`).

For the upcoming phase, the architecture will transition to a **Hierarchical 4-Hub Structure with Sub-Pages**:
- **`[SYNTH]`**: Sub-pages for `Engine/Preset`, `Filter/ADSR`, `16-Step Melodic Sequencer / Arp`, `Voicing/Glide`.
- **`[DRUMS]`**: Sub-pages for `8-Pad Live Grid`, `Kit Tuning & Drive`, `16-Step Drum Sequencer`.
- **`[FX]`**: Sub-pages for `Chorus`, `Reverb`, `Delay`, `Live Audio In`.
- **`[SETUP]`**: Sub-pages for `MIDI Routing`, `CV/Gate Configuration`, `Hardware Calibration`.

---

## 5. Critical Audio Engine Safety Rules

1. **Envelope Clamping**: Never truncate PCM percussion on Note-Off messages; `_SYNTH_FLAGS_IGNORE_NOTE_OFFS` must remain set for percussion synths.
2. **Display Dirty Detection**: Always compare `currentVal != lastParamVal` when in `PARAM_EDIT` so real-time parameter sweeps immediately trigger display updates.
3. **Safe Parameter Pointers**: All master and drum parameters must bind to persistent variables in `System` rather than transient memory.
