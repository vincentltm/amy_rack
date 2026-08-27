# AMY Rack

A Eurorack-compatible synthesizer module built on the [AmyBoard](https://amyboard.com) hardware platform, porting the [Spark Synth](https://github.com/povle/spark-synth) to modular format.

## Features

- **4 Synth Engines**: DX7 (128 FM patches), Juno-106 (64 VA patches), 2-oscillator Analog, and PCM Piano
- **MIDI**: TRS MIDI In/Out + USB MIDI — connect any keyboard or DAW
- **CV**: Gate + V/Oct input for Eurorack sequencers, 2× CV output (MIDI-to-CV)
- **Display**: 128×128 SH1107 OLED with instrument visualizations (algorithm graphs, slider panels, ADSR envelopes)
- **Encoder**: Single rotary encoder for full parameter control and navigation
- **Audio**: Stereo I2S in/out + S/PDIF via PCM9211 codec, switchable line/Eurorack levels

## Hardware

- **Board**: AmyBoard (ESP32-S3, 10HP Eurorack)
- **Display**: Adafruit 1.5" 128×128 OLED (SH1107, STEMMA QT)
- **Encoder**: Adafruit I2C QT Rotary Encoder (seesaw, 0x36)
- **CV DAC**: GP8413 15-bit I2C DAC (0x58) for CV outputs
- **Power**: Eurorack +12V or USB-C 5V

## Building

1. Install [Arduino IDE](https://www.arduino.cc/en/software) 2.x
2. Add ESP32 board support: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Select board: **Tools → Board → ESP32 Arduino → AMYboard**
4. Install libraries via Library Manager:
   - **AMY** (synthesizer engine)
   - **U8g2** (OLED display)
   - **Adafruit seesaw** (rotary encoder)
5. Open `firmware/firmware.ino`
6. Click **Upload**

## Project Structure

```
amy_rack/
├── firmware/              # Arduino project
│   ├── firmware.ino       # Main sketch
│   └── src/               # Source files
├── docs/                  # Documentation
├── amy/                   # AMY library reference
└── spark-synth/           # Spark Synth reference
```

## Documentation

- [Architecture](docs/architecture.md)
- [Hardware Reference](docs/hardware.md)
- [UI Navigation](docs/ui_navigation.md)
- [Handoff Guide](docs/handoff.md)

## Credits

- **AMY Synthesizer**: [shorepine/amy](https://github.com/shorepine/amy)
- **Spark Synth**: [povle/spark-synth](https://github.com/povle/spark-synth)
- **AmyBoard**: [amyboard.com](https://amyboard.com)

## License

MIT
