# Hardware & Interface Reference

Complete hardware specification, pinout, voltage calibration, and bus architecture for the AMY Rack module on the AmyBoard platform.

---

## 1. AmyBoard ESP32-S3 Pinout

| Pin / Net | GPIO | Direction | Function / Connected Subsystem |
|---|---|---|---|
| **I2S BCLK** | GPIO 8 | Output | Audio bit clock (Slave mode to PCM9211) |
| **I2S LRC (WS)** | GPIO 2 | Output | Left/Right word select clock |
| **I2S DOUT** | GPIO 6 | Output | Serial audio data to PCM9211 DAC |
| **I2S DIN** | GPIO 9 | Input | Serial audio data from PCM9211 ADC |
| **I2S MCLK** | GPIO 3 | Input | Master audio clock (12.288MHz from PCM9211 PLL) |
| **MIDI RX** | GPIO 21 | Input | Hardware TRS MIDI In (Optoisolated) |
| **MIDI TX A** | GPIO 14 | Output | TRS MIDI Out (Type A wiring) |
| **MIDI TX B** | GPIO 15 | Output | TRS MIDI Out (Type B wiring) |
| **Front I2C SDA** | GPIO 17 | I/O | Front-panel STEMMA QT / Grove bus (Display, Encoder, DAC) |
| **Front I2C SCL** | GPIO 18 | Output | Front-panel STEMMA QT / Grove clock (400kHz Fast Mode) |

> [!IMPORTANT]
> **I2C Bus Isolation**: The front panel I2C port (SDA=17, SCL=18) is completely separate from the internal/back-panel I2C port used for Tulip expansions. All UI peripherals (SH1107 display, seesaw encoder, GP8413 DAC) must communicate exclusively over pins 17 and 18.

---

## 2. I2C Bus Address Map

| Address (Hex) | Device | Description |
|---|---|---|
| **`0x3C`** | SH1107 OLED | 1.5" 128×128 pixel display via U8g2 driver |
| **`0x36`** | Adafruit seesaw | Rotary encoder counter + pushbutton (Pin 24) |
| **`0x58`** | GP8413 DAC | 15-bit dual-channel bipolar CV output converter |
| **`0x40`** | PCM9211 Codec | Stereo 24-bit 48kHz audio transceiver / PLL (Internal) |

---

## 3. GP8413 15-Bit CV DAC Specification

The GP8413 provides 2 precision analog CV outputs with a $\pm 10\text{V}$ Eurorack-compatible voltage range.

### Register Map:
- **`0x01` (Range Config)**: Set to `0x0001` on startup for full-scale $0\dots 10\text{V}$ range (which with hardware op-amp bias shifts to $-10\text{V}\dots +10\text{V}$).
- **`0x02` (Channel 0 Output)**: 15-bit DAC register for CV 1 Out (V/Oct pitch output).
- **`0x04` (Channel 1 Output)**: 15-bit DAC register for CV 2 Out (Gate / Mod output).

### Voltage to DAC Formula:
$$\text{DAC\_Value} = \left\lfloor \frac{V_{\text{out}} + 10.0}{20.0} \times 32767 \right\rfloor$$
- $-10\text{V} \implies 0$
- $0\text{V} \implies 16384$
- $+10\text{V} \implies 32767$

---

## 4. CV Input Trigger & Tracking

- **Gate Input**: Threshold $> 2.5\text{V}$ triggers Note-On; $< 0.5\text{V}$ triggers Note-Off.
- **V/Oct Input**: $1\text{V}/\text{octave}$ standard exponential voltage tracking.

---

## 5. Power & Level Configurations

- **Eurorack Power**: Standard 10-pin ribbon connector keyed for $-12\text{V}$ red stripe downwards.
- **Desktop USB-C**: Fully powered via 5V USB-C bus without requiring Eurorack power rails.
- **DIP Switches (Audio Level)**:
  - Position A: Line Level ($\sim 2\text{V}_{\text{pp}}$).
  - Position B: Eurorack Modular Level ($\sim 10\text{V}_{\text{pp}}$).
