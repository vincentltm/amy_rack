# AMY Rack

A full-featured Eurorack-compatible synthesizer and drum workstation built on the [AmyBoard](https://amyboard.com) hardware platform (ESP32-S3, 10HP Eurorack), powered by the [AMY](https://github.com/shorepine/amy) synthesizer engine.

---

## Features

- **Multi-Timbral Synth & Dedicated Background Drum Machine**:
  - **Melodic Synthesizer (`synth = 1`, MIDI Ch 1)**:
    - **DX7**: 6-operator FM synthesis with curated Yamaha DX7 patches and dynamic 32-algorithm visual routing graphs.
    - **Juno-106**: 5-voice Virtual Analog synthesizer with 128 factory Roland Juno-106 patches, authentic sysex logarithmic filter math, DCO pulse-width modulation, sub-oscillator, and HPF/VCF slider graphs.
    - **Analog**: Dual-oscillator subtractive synthesizer with poly/mono-legato glide modes, continuous detune/mix, noise generator, LFO modulation, and real-time oscillator waveshape previews (Sine, Pulse, Saw, Triangle, Noise).
    - **Sampler**: Polyphonic PCM sampler featuring 11 classic Roland TR-808 drum/percussion kits directly from ROM + 1 Live RAM/PSRAM recording slot with start/end trim controls and audio waveform previews.
    - **Additive Spectral Piano**: 24-partial additive synthesis acoustic grand piano with interactive animated keyboard preview.
  - **Dedicated Parallel Drum Synthesizer (`synth = 10`, MIDI Ch 10)**:
    - 6-voice polyphonic GM percussion running permanently in the background.
    - 9 Drum Kits: `TR-808`, `TR-909`, `Linn 9000`, `MR-12`, `Tokyo Synth`, `Power Kit`, `Percussion`, `808 Electro`, `808 Sub Boom`.
    - Real-time Kick, Snare, Tom tuning ($\pm 12\text{ st}$) and analog drive saturation.
  - **Live Stereo Audio In (`osc = 60`)**:
    - Direct PCM9211 ADC streaming to master Chorus, Reverb, and Delay FX bus.
- **6-Tab Unified Navigation System**:
  - `[MAIN]`: Command center with `Synth`, `Patch`, `Drums`, `Synth Vol`, `Drum Vol`, `Audio In`, `Voice Mode`, `Glide`.
  - `[SYNTH]`: Engine-specific oscillator waveforms, detuning, filter cutoff, resonance, FM algorithm/operators, DCO/HPF, and sampler recording/trimming.
  - `[DRUM]`: 8-Pad Drum visualizer grid (`[BD][SD][CH][OH] / [CP][TM][CB][MA]`) with hit flash animations, kit select, drum volume, tuning, and drive.
  - `[ENV]`: Dynamic ADSR amplitude and filter envelopes with dual VCF Bode + ADSR curve visualizers.
  - `[FX]`: Global stereo Chorus, Reverb (liveness, damping), and Stereo Echo/Delay with feedback.
  - `[MIDI]`: Independent `Synth MIDI` channel, `Drum MIDI` channel, and modular CV routing.
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
