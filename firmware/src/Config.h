#pragma once
// =============================================================================
// Config.h — AMY Rack central configuration
// =============================================================================

#include <Arduino.h>

// -----------------------------------------------------------------------------
// I2C Bus (front-panel expansion port — Grove / STEMMA QT)
// -----------------------------------------------------------------------------
#define I2C_SDA_PIN             17
#define I2C_SCL_PIN             18
#define I2C_FREQ                400000  // 400 kHz

// I2C device addresses
#define DISPLAY_I2C_ADDR        0x3C    // SH1107 OLED (or 0x3D)
#define ENCODER_I2C_ADDR        0x36    // Adafruit seesaw rotary encoder
#define CV_DAC_I2C_ADDR         0x58    // GP8413 15-bit DAC (CV outputs)

// -----------------------------------------------------------------------------
// Display — SH1107 128×128 OLED via U8g2
// -----------------------------------------------------------------------------
#define SCREEN_WIDTH            128
#define SCREEN_HEIGHT           128

// UI layout zones (y-coordinates, top-to-bottom)
#define HEADER_Y                0
#define HEADER_HEIGHT           14      // Instrument name + patch info
#define INSTRUMENT_UI_Y         16      // Start of instrument-specific visualizer
#define INSTRUMENT_UI_H         58      // Height for instrument visuals
#define PARAM_LIST_Y            77      // Start of scrollable parameter list
#define PARAM_LIST_H            41      // Parameter list height
#define PARAM_LIST_ROW_H        10      // Height per parameter row
#define STATUS_BAR_Y            120     // Bottom status bar
#define STATUS_BAR_H            8       // Status text

// Fonts (U8g2 font names)
#define FONT_HEADER             u8g2_font_7x14B_tr
#define FONT_PARAM_NAME         u8g2_font_6x10_tr
#define FONT_PARAM_VALUE        u8g2_font_6x10_tr
#define FONT_STATUS             u8g2_font_5x7_tr
#define FONT_INSTRUMENT_TITLE   u8g2_font_9x15B_tf

// -----------------------------------------------------------------------------
// Encoder — Adafruit I2C QT Rotary Encoder (seesaw)
// -----------------------------------------------------------------------------
#define ENCODER_LONG_PRESS_MS   350     // Snappy 350ms for long-press hierarchy back
#define ENCODER_ACCEL_THRESHOLD 3       // Delta/poll to trigger acceleration
#define ENCODER_ACCEL_FACTOR    4       // Multiplier when accelerating
#define ENCODER_POLL_INTERVAL   20      // ms between encoder reads

// Seesaw encoder configuration
#define SEESAW_ENCODER_PIN      24      // Encoder channel on seesaw
#define SEESAW_BUTTON_PIN       24      // Button pin on seesaw

// -----------------------------------------------------------------------------
// CV — Control Voltage I/O
// -----------------------------------------------------------------------------

// CV Input (read via AMY's hooks / ADC)
#define CV_IN_CHANNELS          2
#define CV_GATE_CHANNEL         0       // CV In 0 = Gate
#define CV_VOCT_CHANNEL         1       // CV In 1 = V/Oct pitch
#define CV_GATE_THRESHOLD       2.5f    // Voltage to trigger gate high
#define CV_GATE_RESET           0.5f    // Voltage to reset gate low
#define CV_VOCT_SCALE           1.0f    // 1V/octave standard

// CV Output (written via GP8413 15-bit DAC)
#define CV_OUT_CHANNELS         2
#define CV_OUT_VOCT             0       // CV Out 0 = V/Oct (pitch)
#define CV_OUT_GATE             1       // CV Out 1 = Gate
#define CV_DAC_RESOLUTION       32767   // 15-bit max value
#define CV_DAC_RANGE_MIN        (-10.0f)
#define CV_DAC_RANGE_MAX        10.0f
#define CV_DAC_RANGE_TOTAL      20.0f   // -10V to +10V
#define CV_GATE_HIGH_V          5.0f    // Gate high output voltage
#define CV_GATE_LOW_V           0.0f    // Gate low output voltage
#define CV_VOCT_REF_NOTE        60      // MIDI note for 0V (C4)

// GP8413 DAC I2C register addresses
#define GP8413_REG_CH0          0x02
#define GP8413_REG_CH1          0x04
#define GP8413_REG_RANGE        0x01    // Output range config register
#define GP8413_RANGE_10V        0x00    // 0–10V mode

// -----------------------------------------------------------------------------
// Instruments
// -----------------------------------------------------------------------------
#define NUM_INSTRUMENTS         5
#define INST_DX7                0
#define INST_JUNO               1
#define INST_ANALOG             2
#define INST_SAMPLER            3
#define INST_PIANO              4
#define DEFAULT_INSTRUMENT      INST_JUNO

// Maximum parameters per instrument (for encoder UI)
#define MAX_PARAMS              20

// AMY synth channel assignments
#define SYNTH_CHANNEL_DEFAULT   1       // Default synth channel for instruments

// -----------------------------------------------------------------------------
// MIDI
// -----------------------------------------------------------------------------
#define MIDI_DEFAULT_CHANNEL    0       // MIDI channel 1 (0-indexed)
#define MIDI_DRUM_CHANNEL       9       // Channel 10 (0-indexed)

// -----------------------------------------------------------------------------
// Timing
// -----------------------------------------------------------------------------
#define DISPLAY_UPDATE_INTERVAL 33      // ~30 fps
#define STATUS_UPDATE_INTERVAL  100     // Status bar refresh (ms)
