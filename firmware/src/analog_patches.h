#pragma once
// =============================================================================
// analog_patches.h — Curated Factory Presets for Analog Subtractive Engine
// =============================================================================

#include <Arduino.h>

struct AnalogPreset {
    const char* name;
    uint8_t osc1_wave;       // 0=Sine, 1=Pulse, 2=Saw, 3=Tri, 4=Noise
    uint8_t osc2_wave;       // 0=Sine, 1=Pulse, 2=Saw, 3=Tri, 4=Noise
    float   osc2_detune_pct; // 0..100%
    float   osc_balance_pct; // 0..100%
    float   noise_pct;       // 0..100%
    float   cutoff_hz;       // 20..10000 Hz
    float   resonance;       // 0.5..5.0
    float   attack_ms;       // 1..4000 ms
    float   decay_ms;        // 5..4000 ms
    float   sustain_pct;     // 0..100%
    float   release_ms;      // 5..4000 ms
    uint8_t voice_mode;      // 0=Poly, 1=Mono
    float   glide_ms;        // 0..500 ms
    float   chorus_pct;      // 0..100%
    float   reverb_pct;      // 0..100%
    float   delay_mix_pct;   // 0..100%
};

static const AnalogPreset analog_presets[12] = {
    // 0: Warm Brass
    { "Warm Brass",     2, 2, 25.0f, 50.0f,  0.0f, 1800.0f, 1.2f,  80.0f,  350.0f, 60.0f,  350.0f, 0,   0.0f, 25.0f, 30.0f,  0.0f },
    // 1: Fat Poly Saw
    { "Fat Poly Saw",   2, 2, 45.0f, 50.0f,  0.0f, 4500.0f, 1.0f,  15.0f,  200.0f, 75.0f,  400.0f, 0,   0.0f, 40.0f, 25.0f,  0.0f },
    // 2: Mono Acid Bass
    { "Mono Acid Bass", 2, 1,  0.0f, 75.0f,  0.0f,  950.0f, 3.8f,   5.0f,  180.0f, 15.0f,  180.0f, 1,  45.0f,  0.0f, 15.0f, 35.0f },
    // 3: Sync Lead
    { "Sync Lead",      1, 2, 60.0f, 40.0f,  0.0f, 3200.0f, 2.5f,  10.0f,  250.0f, 80.0f,  250.0f, 1,  30.0f, 30.0f, 20.0f, 40.0f },
    // 4: Analog Pad
    { "Analog Pad",     2, 2, 35.0f, 50.0f,  5.0f, 2200.0f, 1.1f, 400.0f,  800.0f, 90.0f, 1200.0f, 0,   0.0f, 60.0f, 50.0f, 25.0f },
    // 5: Sub Bass
    { "Sub Bass",       0, 1,  0.0f, 20.0f,  0.0f,  600.0f, 1.4f,   5.0f,  200.0f, 40.0f,  180.0f, 1,  20.0f,  0.0f,  0.0f,  0.0f },
    // 6: PWM Strings
    { "PWM Strings",    1, 2, 20.0f, 45.0f,  0.0f, 3800.0f, 0.9f, 150.0f,  600.0f, 85.0f,  800.0f, 0,   0.0f, 70.0f, 40.0f, 20.0f },
    // 7: 5th Lead
    { "5th Lead",       2, 2, 85.0f, 50.0f,  0.0f, 2800.0f, 1.8f,  20.0f,  300.0f, 70.0f,  350.0f, 1,  35.0f, 20.0f, 30.0f, 45.0f },
    // 8: Soft Flute
    { "Soft Flute",     0, 3, 10.0f, 50.0f, 12.0f, 2600.0f, 0.8f,  80.0f,  250.0f, 90.0f,  400.0f, 1,  25.0f, 25.0f, 40.0f, 20.0f },
    // 9: Hard Techno
    { "Hard Techno",    1, 1, 15.0f, 50.0f,  0.0f, 1200.0f, 3.2f,   5.0f,  160.0f, 20.0f,  140.0f, 1,   0.0f,  0.0f, 20.0f, 50.0f },
    // 10: Noise Sweep
    { "Noise Sweep",    4, 2, 50.0f, 30.0f, 80.0f, 1500.0f, 4.2f, 500.0f,  600.0f, 50.0f, 1200.0f, 0,   0.0f, 40.0f, 65.0f, 60.0f },
    // 11: 80s Polysynth
    { "80s Polysynth",  1, 2, 30.0f, 50.0f,  0.0f, 3600.0f, 1.3f,  25.0f,  350.0f, 65.0f,  450.0f, 0,   0.0f, 50.0f, 35.0f, 30.0f }
};
