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

### Fast Terminal Workflow
```bash
# 1. Compile firmware for AmyBoard target
arduino-cli compile --fqbn esp32:esp32:amyboard firmware/

# 2. Flash to AmyBoard via USB-C
arduino-cli upload -p /dev/cu.usbmodem2101 --fqbn esp32:esp32:amyboard firmware/

# 3. Monitor debug logs (optional)
python3 -c 'import serial; s=serial.Serial("/dev/cu.usbmodem2101", 115200); [print(s.readline().decode("utf-8", "replace").strip()) for _ in iter(int, 1)]'
```

---

## 3. How to Add a New Instrument Engine

To add a new synthesizer engine (e.g. `InstrumentWavetable`):

1. **Subclass `Instrument` (`InstrumentWavetable.h`)**:
   ```cpp
   #pragma once
   #include "Instrument.h"

   class InstrumentWavetable : public Instrument {
   public:
       InstrumentWavetable();
       void init() override;
       void start() override;
       void stop() override;
       void drawUI(U8G2 &u8g2) override;
       void onParamChanged(uint8_t paramIndex) override;
       const ParamDescriptor *getParams() const override { return _customParams; }
       uint8_t getParamCount() const override { return _customParamCount; }
   private:
       ParamDescriptor _customParams[MAX_PARAMS];
       uint8_t _customParamCount = 0;
   };
   ```

2. **Register Parameters in `init()`**:
   ```cpp
   void InstrumentWavetable::init() {
       buildBaseParams();
       _customParams[0] = PARAM_PCT("Wavetable Pos", 0.0f, 100.0f, 1.0f, &_wt_pos, TAB_SYNTH);
       _customParamCount = 1;
       // Append shared ENV and FX base parameters
       for (int i = 0; i < _baseParamCount; i++) {
           _customParams[_customParamCount++] = _baseParams[i];
       }
   }
   ```

3. **Instantiate Voice Oscillators in `start()`**:
   - Define custom patch template on patch number `1024` or higher.
   - Define individual oscillator behaviors (`e.wave`, `e.amp_coefs`, `e.eg0_times`).
   - Call `e.synth = getSynthChannel(); e.patch_number = 1024; e.num_voices = 6; amy_add_event(&e);`.

4. **Register in `System.h` / `System.cpp`**:
   - Increment `NUM_INSTRUMENTS`.
   - Add new engine enum and pointer to `_instruments[]` in `System::initInstruments()`.

---

## 4. How to Add a New Parameter to an Existing Instrument

1. Add the variable storage to the instrument class or struct.
2. In `init()`, assign a `PARAM_*` macro to `_params[i]` (e.g., `PARAM_FLOAT`, `PARAM_INT`, `PARAM_PCT`, `PARAM_ENUM`).
3. Make sure to specify the target tab: `TAB_MAIN`, `TAB_SYNTH`, `TAB_ENV`, `TAB_FX`, `TAB_MIDI`, or `TAB_CV`.
4. In `onParamChanged(uint8_t paramIndex)`, send the corresponding delta `amy_event` to update the active voice oscillators.

---

## 5. Critical Audio Engine Rules

1. **Avoid DC Amplitude Bias**: Never set `amp_coefs[COEF_CONST]` on voice oscillators to non-zero values unless gated by envelope/velocity, as DC offsets produce loud clicks and thumps on note boundaries.
2. **Synchronous Reset on Switch**: Always use `amy_reset_oscs()` directly when switching active engines to guarantee that all voices, envelopes, and patch deltas are cleared before instantiating new oscillators.
3. **Single Filter Application**: Do not iterate filter updates across individual voice oscillators in series with `for (int i=0; i<4; i++)`; apply filters once at the synth channel root (`e.synth = getSynthChannel()`) to avoid cascading 24dB filters.
