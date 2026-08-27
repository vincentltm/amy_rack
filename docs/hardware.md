# Hardware Reference

Comprehensive hardware specification for AMY Rack.

## AmyBoard Pinout

| Function | ESP32-S3 Pin | Notes |
|----------|--------------|-------|
| I2S BCLK | GPIO 8 | To PCM9211 (slave mode) |
| I2S LRC (WS) | GPIO 2 | To PCM9211 |
| I2S DOUT | GPIO 6 | To PCM9211 |
| I2S DIN | GPIO 9 | From PCM9211 |
| I2S MCLK | GPIO 3 | External master clock from PCM9211 |
| MIDI RX | GPIO 21 | TRS MIDI In |
| MIDI TX Type A | GPIO 14 | TRS MIDI Out (Type A) |
| MIDI TX Type B | GPIO 15 | TRS MIDI Out (Type B) |
| Front I2C SDA | GPIO 17 | Front panel STEMMA QT / Grove (Display, Encoder, DAC) |
| Front I2C SCL | GPIO 18 | Front panel STEMMA QT / Grove |

## I2C Bus & Address Map

**IMPORTANT:** The front panel I2C bus (SDA=17, SCL=18) is distinct from the back panel I2C bus (used for Tulip expansions). Ensure all front-panel modules are connected to pins 17/18.

- **Display (SH1107 OLED):** `0x3C`
- **Encoder (Adafruit seesaw):** `0x36`
- **CV DAC (GP8413):** `0x58`
- **Audio Codec (PCM9211):** `0x40` (Internal back-panel bus usually)

## DAC & Control Voltage

### GP8413 CV DAC
- 15-bit resolution (0 to 32767).
- Bipolar output range configured via register `0x01` (`GP8413_RANGE_10V` mode + external op-amp bias gives -10V to +10V).
- Register `0x02` controls CH0 (V/Oct).
- Register `0x04` controls CH1 (Gate).

**Voltage Calculation:**
`DAC_Value = ((V_out + 10.0) / 20.0) * 32767`

### CV Inputs
- **Gate:** Trigger threshold > 2.5V, Reset < 0.5V.
- **V/Oct:** 1V/octave standard scaling.

## Audio & Power

- **DIP Switches:** Internal switches toggle between Line Level (approx. 2Vpp) and Eurorack Level (approx. 10Vpp).
- **Power Options:**
  - Standard Eurorack 10-pin header (+12V, -12V, 5V).
  - USB-C 5V for standalone desktop use and development.
  - I2C STEMMA QT supplies 3.3V to the display and encoder.

## Module Specs

- **SH1107 Display:** 1.5" 128x128 pixel OLED. Uses `U8g2` library. Supports 180-degree rotation.
- **Adafruit seesaw Encoder:** I2C-based rotary encoder with built-in push button (Pin 24 on the seesaw chip) and optional NeoPixel support.
