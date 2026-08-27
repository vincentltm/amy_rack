# System Architecture & Multi-Timbral Engine Design

This document details the software architecture, threading model, memory layout, and synthesis pipeline of the AMY Rack Eurorack module.

---

## 1. High-Level System Architecture

```mermaid
graph TD
    subgraph Hardware I/O
        MIDI_TRS[TRS MIDI In / Out]
        MIDI_USB[USB MIDI In / Out]
        AUDIO_IN[Stereo Audio In Jacks]
        CV_IN[CV In: Gate + V/Oct]
        ENC[I2C Rotary Encoder 0x36]
        OLED[128x128 SH1107 OLED 0x3C]
        DAC_CV[GP8413 15-bit DAC 0x58]
        I2S_OUT[PCM9211 24-bit 48kHz I2S]
    end

    subgraph Core 0 Application & Controls
        ENC_DRV[EncoderInput Driver] --> SYS[System & Navigation State Machine]
        CV_MGR[CVManager] --> SYS
        MIDI_MGR[MidiManager Hook] --> SYS
        SYS --> DISP[Display Manager / U8g2]
        DISP --> OLED
        MIDI_MGR --> DAC_CV
    end

    subgraph Core 1 AMY Real-Time Audio Engine
        AUDIO_IN -->|I2S ADC (GPIO 9)| AMY_IN[AUDIO_IN0 / AUDIO_IN1 Oscillator]
        SYS -->|amy_event queue| SYNTH_ENG[Melodic Synth: synth = 1, Ch 1]
        SYS -->|amy_event queue| DRUM_ENG[Drum Machine: synth = 10, Ch 10]
        SYNTH_ENG --> MASTER_BUS[Master FX Bus: Chorus, Reverb, Delay]
        DRUM_ENG --> MASTER_BUS
        AMY_IN --> MASTER_BUS
        MASTER_BUS --> I2S_OUT
    end
```

---

## 2. Multi-Timbral Engine Architecture

AMY Rack runs **two dedicated synthesizer engines concurrently**:

1. **Melodic Synth Engine (`synth = 1`)**:
   - Listens to configurable **`Synth MIDI`** (Channel 1 by default, or 1..16/Omni).
   - Dynamically loads any of the 5 melodic instruments:
     - **DX7**: Pure 6-operator Yamaha FM synthesizer (ROM patches 128..255).
     - **Juno-106**: 5-voice Virtual Analog with PWM, Sub-oscillator, Noise, and 24dB resonant VCF.
     - **Analog**: Dual-Oscillator subtractive synth with Sine/Saw/Pulse/Tri waves, detune, noise, and balance.
     - **Sampler**: Polyphonic PCM playback (11 built-in 808 ROM sets + Live Audio RAM recording slot).
     - **Piano**: 4-voice Additive Spectral physical modeled piano (Patch 256).

2. **Drum Machine Engine (`synth = 10`)**:
   - Runs continuously in the background on **`Drum MIDI`** (Channel 10 by default).
   - Generates 6-voice polyphonic GM percussion mapped to 9 distinct Gamma9001 / 808 drum kits.
   - Suppresses note-off clipping so drum hits ring out naturally.
   - Offers real-time Kick, Snare, and Tom pitch tuning ($\pm 12\text{ st}$) and analog drive saturation.

3. **Live Audio In (`osc = 60`, `AUDIO_IN0`)**:
   - Streams incoming stereo audio from the PCM9211 ADC directly into AMY's DSP pipeline.
   - Routes through the master Chorus, Reverb, and Delay effects in real time.

---

## 3. Multi-Core Threading & Memory Architecture

AMY Rack runs on the ESP32-S3 (Dual Xtensa LX7 cores @ 240MHz):

1. **Core 0 (Control & Event Core)**:
   - Rotary Encoder scanning with acceleration and debouncing.
   - Display frame rendering via U8g2 with value-change dirty caching (~30 FPS).
   - Background tasks (Sampler RAM audio capture task).
   - Dual MIDI Channel routing and CV output DAC translation.
2. **Core 1 (Real-Time Audio Core)**:
   - Dedicated exclusively to AMY's synthesis pipeline (`amy_update()`).
   - Generates 32-sample audio blocks at 48,000Hz (or 22,050Hz internal engine clock).
   - Direct DMA transfer to PCM9211 I2S audio codec.
3. **PSRAM (8MB Octal SPI)**:
   - Stores master delay lines, stereo reverb allpass networks, and the live Sampler audio recording buffer.
