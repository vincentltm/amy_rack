# UI & Navigation

## Screen Layout

The 128x128 OLED display is divided into specific horizontal zones (`Config.h`):

- **Header (`Y: 0-14`):** Current Instrument Name and Patch Name/Number.
- **Instrument UI (`Y: 15-78`):** 63px high area dedicated to bespoke instrument visualizations (e.g., DX7 algorithm graph, Juno slider panel, ADSR envelopes).
- **Parameter List (`Y: 80-118`):** A scrollable list showing 4-5 editable parameters at a time.
- **Status Bar (`Y: 120-127`):** Small 8px bar at the bottom displaying MIDI channel, last received note, and gate status.

## Navigation State Machine

The rotary encoder drives the primary interaction loop.

1. **Scroll Mode (Default):**
   - Rotating the encoder scrolls up/down the parameter list.
   - Pushing the button enters Edit Mode for the highlighted parameter.
   - Long-pressing the button (800ms) enters the Instrument Selection Menu.
2. **Edit Mode:**
   - Rotating the encoder changes the parameter value.
   - Values are clamped to `minVal` and `maxVal`.
   - Fast rotation triggers an acceleration multiplier (`ENCODER_ACCEL_FACTOR`).
   - Pushing the button confirms the edit and returns to Scroll Mode.
3. **Instrument Menu:**
   - Scroll through available synth engines (DX7, Juno, Analog, Piano).
   - Click to select and load the instrument.

## Instrument UIs

Each instrument implements `drawUI(U8G2 &u8g2)` to render its visualizer:
- **Juno:** Draws horizontal sliders representing DCO, VCF, and VCA states.
- **DX7:** Visualizes the currently selected FM algorithm with 6 operator boxes and routing lines.
- **Analog:** Shows basic waveform shapes (saw, square, tri) based on parameter states.

## Future Additions (Phase 2/3)

- **Sequencer Grid:** A separate UI state for step sequencing.
- **Oscilloscope:** A real-time rendering of the master audio output buffer.
- **CV Matrix:** A visual patching grid connecting MIDI CCs/LFOs to CV Outputs.
