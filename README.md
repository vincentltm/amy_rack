# AMY Rack

A full-featured Eurorack-compatible synthesizer module built on the [AmyBoard](https://amyboard.com) hardware platform (ESP32-S3, 10HP Eurorack), porting and significantly expanding [Spark Synth](https://github.com/povle/spark-synth) for modular synthesizers.

---

## Features

- **5 Dedicated Synthesizer Engines**:
  - **DX7**: 6-operator FM synthesis with 128 curated Yamaha DX7 patches and dynamic 32-algorithm visual routing graphs.
  - **Juno-106**: 5-voice Virtual Analog synthesizer with 128 factory Roland Juno-106 patches, authentic sysex logarithmic filter math, DCO pulse-width modulation, sub-oscillator, and HPF/VCF slider graphs.
  - **Analog**: Dual-oscillator subtractive synthesizer with 12 curated factory presets, poly/mono-legato glide modes, continuous detune/mix, noise generator, LFO modulation, and real-time oscillator waveshape previews (Sine, Pulse, Saw, Triangle, Noise).
  - **Sampler**: 6-voice polyphonic PCM sampler featuring 11 classic Roland TR-808 drum/percussion kits directly from ROM + 1 Live RAM/PSRAM recording slot with start/end trim controls and audio waveform previews.
  - **Additive Spectral Piano**: 24-partial additive synthesis acoustic grand piano with interactive animated keyboard preview.
- **6-Tab Unified Navigation System**:
  - `[MAIN]`: Master engine select, patch browser, master volume, and quick macros.
  - `[SYNTH]`: Engine-specific oscillator waveforms, detuning, filter cutoff, resonance, FM algorithm/operators, DCO/HPF, and sampler recording/trimming.
  - `[ENV]`: Dynamic ADSR amplitude and filter envelopes with responsive envelope curve visualizers.
  - `[FX]`: Global stereo Chorus, Reverb (liveness, damping, crossover), and Stereo Echo/Delay with feedback.
  - `[MIDI]`: MIDI channel selector (Ch 1..16 + Omni), octave transpose, pitch bend range, and real-time MIDI monitor.
  - `[CV]`: Modular CV input trigger/tracking routing (V/Oct, Cutoff, Volume) and GP8413 15-bit DAC dual CV output mapping (V/Oct, Gate, Velocity, ModWheel).
- **Display & Visualizations**: High-contrast 128×128 SH1107 OLED with bespoke real-time instrument visualizers and snappy UI updates.
- **Controls**: Single rotary encoder with push-button and adaptive acceleration for instant tactile navigation.
- **Audio & I/O**: Stereo 24-bit 48kHz I2S in/out + S/PDIF via PCM9211 codec, switchable Eurorack / Line audio levels.

---

## Hardware Specifications

- **Processor**: AmyBoard (ESP32-S3 dual-core @ 240MHz, 8MB PSRAM, 16MB Flash)
- **Format**: 10HP Eurorack module or standalone USB-C desktop synth
- **Display**: Adafruit 1.5" 128×128 OLED (SH1107 driver over front STEMMA QT I2C: SDA=GPIO 17, SCL=GPIO 18)
- **Rotary Encoder**: Adafruit I2C seesaw Rotary Encoder (Address `0x36`)
- **CV DAC**: GP8413 15-bit dual-channel I2C DAC (Address `0x58`, -10V to +10V output range)
- **Audio Codec**: PCM9211 stereo codec (Slave mode I2S: BCLK=8, LRC=2, DOUT=6, DIN=9, MCLK=3)
- **Power**: Standard 10-pin Eurorack power header (+12V/-12V/5V) or USB-C 5V bus power

---

## Building and Flashing

### Using Arduino CLI (Recommended)

```bash
# Compile firmware
arduino-cli compile --fqbn esp32:esp32:amyboard firmware/

# Upload to AmyBoard via USB-C
arduino-cli upload -p /dev/cu.usbmodem2101 --fqbn esp32:esp32:amyboard firmware/
```

### Using Arduino IDE 2.x

1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software).
2. Add the ESP32 Board URL in Preferences: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
3. Select board: **Tools → Board → ESP32 Arduino → AMYboard**.
4. Install required libraries:
   - `AMY Synthesizer`
   - `U8g2`
   - `Adafruit seesaw Library`
   - `Adafruit BusIO`
5. Open `firmware/firmware.ino` and click **Upload**.

---

## Documentation

- [Architecture & Data Flow](docs/architecture.md)
- [Hardware Reference & Pinouts](docs/hardware.md)
- [UI Navigation & State Machine](docs/ui_navigation.md)
- [Developer Handoff Guide](docs/handoff.md)
- [AI & Audio Engine Technical Reference](docs/ai_documentation.md)

---

## License

MIT License
