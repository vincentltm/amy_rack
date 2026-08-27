# Developer Handoff Guide

Welcome to the AMY Rack project! This guide will help you get started with development, build the firmware, and extend the system.

## Prerequisites

1. **Arduino IDE 2.x** or VS Code with Arduino extension.
2. **ESP32 Board Package**: Version >= 3.3.8 required. Add `https://espressif.github.io/arduino-esp32/package_esp32_index.json` to your Additional Boards Manager URLs.
3. **Libraries**:
   - `AMY` (shorepine/amy)
   - `U8g2` (olikraus/u8g2)
   - `Adafruit seesaw Library`

## Building and Flashing

1. Connect the AmyBoard via USB-C to your computer.
2. Open `firmware/firmware.ino` in Arduino IDE.
3. Select Board: **Tools → Board → ESP32 Arduino → AMYboard**
4. Verify compiling succeeds.
5. Click **Upload**.

## Project File Structure

- `README.md`: High-level overview.
- `firmware/firmware.ino`: Main setup/loop, hardware init.
- `firmware/src/Config.h`: Central definition of all pins, I2C addresses, UI dimensions, and constants.
- `firmware/src/ParamDefs.h`: Structures for the UI parameter system (`ParamDescriptor`).
- `firmware/src/Instrument.h`: Base class for synth engines. Wraps AMY API calls.
- `docs/`: Documentation folder.

## How to Add a New Instrument

1. Create a new class that inherits from `Instrument` (e.g., `InstrumentWavetable.h`).
2. Override `drawUI(U8G2 &u8g2)` to render its unique visual.
3. Override `getSynthChannel()` to define which AMY voices it uses.
4. Populate custom parameters by appending to `_baseParams` or overriding `getParams()`.
5. Implement `noteOn()`, `noteOff()`, and `sendAllParams()` to map your `SynthParams` to AMY `amy_event` calls.
6. Add it to the instrument list in `amy_rack.ino`.

## How to Add a New Parameter

1. Add the variable to the `SynthParams` struct in `Instrument.h`.
2. In your instrument's setup, define a new `ParamDescriptor` using the macros in `ParamDefs.h` (e.g., `PARAM_FLOAT("MyParam", "%", 0.0, 1.0, 0.01, &params.myParam)`).
3. Ensure `getParamCount()` reflects the updated total.
4. In `onParamChanged()`, map this parameter to the corresponding `amy_event` parameter (e.g., `e.filter_freq`).

## Modifying the UI Layout

All UI coordinates and dimensions are strictly defined in `firmware/src/Config.h`. To change the height of the instrument area or the parameter list, modify `INSTRUMENT_UI_Y`, `INSTRUMENT_UI_H`, `PARAM_LIST_Y`, etc.

## Debugging Tips

- **Serial Monitor**: Runs at 115200 baud. Use `Serial.println()` for basic tracing.
- **USB vs Eurorack Power**: You can develop over USB without connecting it to a Eurorack case.
- **I2C Scanner**: If the screen or encoder isn't responding, double-check that they are on pins 17/18 and write a quick I2C scanner sketch to verify addresses.

## Key Reference Links

- [AMY API Documentation](https://github.com/shorepine/amy)
- [AmyBoard Hardware](https://amyboard.com)
- [U8g2 Reference](https://github.com/olikraus/u8g2/wiki)
- [Adafruit seesaw](https://learn.adafruit.com/adafruit-seesaw-pb92)

## Phase 2/3 Roadmap

- Sequencer interface and clock syncing.
- Full MIDI-to-CV routing matrix.
- Look into `amy_external_coef_hook` for tight CV input integration.
