// =============================================================================
// amy_rack.ino — AMY Rack: Eurorack Synth Module for AmyBoard
// =============================================================================
// A port of Spark Synth to AmyBoard hardware.
//
// Features:
//   - 4 synth engines: DX7, Juno-106, 2-osc Analog, PCM Piano
//   - TRS MIDI In/Out + USB MIDI (native via AMY)
//   - CV Gate + V/Oct input, 2× CV output (MIDI-to-CV)
//   - 128×128 SH1107 OLED display with instrument visualizations
//   - Single rotary encoder for full parameter control
//
// Hardware: AmyBoard (ESP32-S3, 10HP Eurorack)
// Build:    Arduino IDE → Board: esp32:esp32:amyboard
// Libraries: AMY, U8g2, Adafruit seesaw
// =============================================================================

#include <AMY-Arduino.h>
#include <Wire.h>

#include "src/Config.h"
#include "src/Display.h"
#include "src/EncoderInput.h"
#include "src/CVManager.h"
#include "src/MidiManager.h"
#include "src/System.h"

// ---------------------------------------------------------------------------
// Global subsystem instances
// ---------------------------------------------------------------------------
Display      display;
EncoderInput encoder;
CVManager    cvManager;
MidiManager  midiManager;

// Timing
unsigned long lastDisplayUpdate = 0;

// =============================================================================
// setup()
// =============================================================================
void setup() {
    // --- Serial for debugging ---
    Serial.begin(115200);
    delay(100);
    Serial.println("=== AMY Rack v1.0 ===");

    // --- Initialize I2C bus for front-panel accessories ---
    // SDA=17, SCL=18 (AmyBoard front panel Grove/STEMMA QT port)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQ);
    Serial.println("[I2C] Front panel bus initialized (SDA=17, SCL=18)");

    // --- Configure and start AMY ---
    // When compiled with the AmyBoard board target, AMYBOARD_ARDUINO is defined
    // automatically.  AMY's amy_default_config() then pre-fills:
    //   - I2S pins (BCLK=8, LRC=2, DOUT=6, DIN=9, MCLK=3) in slave mode
    //   - PCM9211 codec initialization
    //   - MIDI UART (IN=21, OUT_A=14, OUT_B=15)
    //   - USB MIDI gadget via TinyUSB
    //   - Dual-core rendering
    amy_config_t amy_config = amy_default_config();
    amy_config.features.startup_bleep = 1;
    amy_config.features.default_synths = 1;   // Juno on ch1, DX7 on ch2, drums on ch10

    // Install MIDI input hook so MidiManager can intercept messages for CV out
    midiManager.installMidiHook(amy_config);

    amy_start(amy_config);
    Serial.println("[AMY] Synthesizer engine started");

    // --- Initialize hardware peripherals ---
    display.begin();
    Serial.println("[Display] SH1107 128x128 OLED initialized");

    encoder.begin();
    Serial.println("[Encoder] Adafruit seesaw encoder initialized");

    cvManager.begin();
    Serial.println("[CV] CV inputs + GP8413 DAC initialized");

    midiManager.begin(cvManager);
    Serial.println("[MIDI] MIDI manager initialized");

    // --- Initialize the system (instruments, navigation) ---
    Sys.begin(display, encoder, cvManager, midiManager);
    Serial.println("[System] Instruments loaded, ready!");
    Serial.println("========================");
}

// =============================================================================
// loop()
// =============================================================================
void loop() {
    // 1. AMY audio rendering + MIDI polling
    //    This is the heartbeat — AMY renders audio blocks, processes MIDI UART
    //    and USB MIDI messages, and runs the sequencer.  Must be called every loop.
    amy_update();

    // 2. Read encoder input
    encoder.update();

    // 3. Process system state (navigation, instrument control)
    Sys.update();

    // 4. Update CV outputs (responds to MIDI-to-CV events set by MidiManager)
    cvManager.update();

    // 5. Update display (rate-limited to ~30fps)
    unsigned long now = millis();
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        lastDisplayUpdate = now;
        display.update(Sys, midiManager);
    }
}
