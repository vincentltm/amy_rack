#pragma once
// =============================================================================
// Config.h — AMY Rack central configuration with Permanent Visualizer & Tab Bar
// =============================================================================

#include <Arduino.h>

// -----------------------------------------------------------------------------
// I2C Bus (front-panel expansion port — Grove / STEMMA QT)
// -----------------------------------------------------------------------------
#define I2C_SDA_PIN             17
#define I2C_SCL_PIN             18
#define I2C_FREQ                400000  // 400 kHz

// I2C device addresses
#define DISPLAY_I2C_ADDR        0x3C    // SH1107 OLED
#define ENCODER_I2C_ADDR        0x36    // Adafruit seesaw rotary encoder
#define CV_DAC_I2C_ADDR         0x58    // GP8413 15-bit DAC (CV outputs)

// -----------------------------------------------------------------------------
// Display — SH1107 128×128 OLED (Always-On Visualizer + Mid Tab Bar)
// -----------------------------------------------------------------------------
#define SCREEN_WIDTH            128
#define SCREEN_HEIGHT           128

// UI layout zones (top-to-bottom)
#define HEADER_Y                0
#define HEADER_HEIGHT           12      // Top Header: Instrument & Patch

#define VISUALIZER_Y            14      // Top Visualizer zone (Always Present!)
#define VISUALIZER_H            47      // Visualizer height (47px)

#define TAB_BAR_Y               63      // Mid Tab Bar (Pills under visualizer)
#define TAB_BAR_H               11      // Tab Bar height

#define PARAM_LIST_Y            76      // Parameter list below Tab Bar
#define PARAM_LIST_H            42      // Parameter list height
#define PARAM_ROW_H             10      // 10px per parameter row (4 visible rows)

#define STATUS_BAR_Y            120     // Bottom status bar
#define STATUS_BAR_H            8

// Fonts (U8g2 font names)
#define FONT_HEADER             u8g2_font_7x14B_tr
#define FONT_TAB                u8g2_font_5x7_tr
#define FONT_PARAM_NAME         u8g2_font_6x10_tr
#define FONT_PARAM_VALUE        u8g2_font_6x10_tr
#define FONT_STATUS             u8g2_font_5x7_tr
#define FONT_INSTRUMENT_TITLE   u8g2_font_9x15B_tf

// -----------------------------------------------------------------------------
// Encoder — Adafruit I2C QT Rotary Encoder (seesaw)
// -----------------------------------------------------------------------------
#define ENCODER_LONG_PRESS_MS   350     // Snappy 350ms
#define ENCODER_ACCEL_THRESHOLD 3
#define ENCODER_ACCEL_FACTOR    4
#define ENCODER_POLL_INTERVAL   20

#define SEESAW_ENCODER_PIN      24
#define SEESAW_BUTTON_PIN       24

// -----------------------------------------------------------------------------
// CV — Control Voltage I/O
// -----------------------------------------------------------------------------
#define CV_IN_CHANNELS          2
#define CV_GATE_CHANNEL         0
#define CV_VOCT_CHANNEL         1
#define CV_GATE_THRESHOLD       2.5f
#define CV_GATE_RESET           0.5f
#define CV_VOCT_SCALE           1.0f

#define CV_OUT_CHANNELS         2
#define CV_OUT_VOCT             0
#define CV_OUT_GATE             1
#define CV_DAC_RESOLUTION       32767
#define CV_DAC_RANGE_MIN        (-10.0f)
#define CV_DAC_RANGE_MAX        10.0f
#define CV_DAC_RANGE_TOTAL      20.0f
#define CV_GATE_HIGH_V          5.0f
#define CV_GATE_LOW_V           0.0f
#define CV_VOCT_REF_NOTE        60

#define GP8413_REG_CH0          0x02
#define GP8413_REG_CH1          0x04
#define GP8413_REG_RANGE        0x01
#define GP8413_RANGE_10V        0x00

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

#define MAX_PARAMS              24
#define SYNTH_CHANNEL_DEFAULT   1

// -----------------------------------------------------------------------------
// MIDI
// -----------------------------------------------------------------------------
#define MIDI_DEFAULT_CHANNEL    0
#define MIDI_DRUM_CHANNEL       9

// -----------------------------------------------------------------------------
// Timing
// -----------------------------------------------------------------------------
#define DISPLAY_UPDATE_INTERVAL 33      // ~30 fps
#define STATUS_UPDATE_INTERVAL  100     // Status refresh
