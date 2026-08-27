#include "InstrumentDrum.h"
#include <cmath>
#include <algorithm>

static const char* kitNames[4] = {
    "808 Classic",
    "808 Electro",
    "808 Percussion",
    "808 Sub Boom"
};

static const char* padLabels[DRUM_NUM_PADS] = {
    "BD", "SD", "CH", "OH",
    "CP", "TM", "CB", "MA"
};

InstrumentDrum::InstrumentDrum() {
    _instrumentName = "Drum Machine";
    _instrumentShortName = "DRUM";
}

void InstrumentDrum::init() {
    buildBaseParams();

    // Tab: SYNTH
    _drumParams[0] = PARAM_FLOAT("Kick Tune", "st", -12.0f, 12.0f, 1.0f, &_param_kick_tune, TAB_SYNTH);
    _drumParams[1] = PARAM_FLOAT("Snare Tune", "st", -12.0f, 12.0f, 1.0f, &_param_snare_tune, TAB_SYNTH);
    _drumParams[2] = PARAM_FLOAT("Tom Tune", "st", -12.0f, 12.0f, 1.0f, &_param_tom_tune, TAB_SYNTH);
    _drumParams[3] = PARAM_FLOAT("Drive", "x", 0.5f, 5.0f, 0.1f, &_param_drive, TAB_SYNTH);

    _drumParamCount = 4;
    for (int i = 0; i < _baseParamCount; i++) {
        _drumParams[_drumParamCount++] = _baseParams[i];
    }
}

void InstrumentDrum::start() {
    isActive = true;
    setupSynthVoices();
    needsUIRedraw = true;
}

void InstrumentDrum::stop() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.velocity = 0.0f;
    amy_add_event(&e);
    isActive = false;
}

void InstrumentDrum::setPatch(int index) {
    if (index < 0) index = 3;
    if (index > 3) index = 0;
    _currentKit = index;

    if (_currentKit == 0) { // Classic 808
        _param_kick_tune = 0.0f;
        _param_snare_tune = 0.0f;
        _param_tom_tune = 0.0f;
        _param_drive = 2.2f;
    } else if (_currentKit == 1) { // Electro
        _param_kick_tune = 3.0f;
        _param_snare_tune = 4.0f;
        _param_tom_tune = 2.0f;
        _param_drive = 3.0f;
    } else if (_currentKit == 2) { // Percussion
        _param_kick_tune = -2.0f;
        _param_snare_tune = 0.0f;
        _param_tom_tune = 5.0f;
        _param_drive = 2.0f;
    } else if (_currentKit == 3) { // Sub Boom
        _param_kick_tune = -7.0f;
        _param_snare_tune = -2.0f;
        _param_tom_tune = -4.0f;
        _param_drive = 3.5f;
    }

    setupSynthVoices();
    needsUIRedraw = true;
}

const char* InstrumentDrum::getPatchName(int idx) const {
    if (idx >= 0 && idx < 4) return kitNames[idx];
    return "";
}

void InstrumentDrum::onParamChanged(uint8_t paramIndex) {
    if (paramIndex >= 12) {
        configChorus();
        configReverb();
        configDelay();
    }
    needsUIRedraw = true;
}

void InstrumentDrum::setupSynthVoices() {
    amy_event e = amy_default_event();
    e.reset_osc = RESET_PATCH;
    e.patch_number = 1025;
    amy_add_event(&e);

    e = amy_default_event();
    e.patch_number = 1025;
    e.wave = PCM;
    e.preset = 1;
    amy_add_event(&e);

    e = amy_default_event();
    e.synth = getSynthChannel();
    e.patch_number = 1025;
    e.num_voices = 8;
    amy_add_event(&e);

    configChorus();
    configReverb();
    configDelay();
}

void InstrumentDrum::triggerDrum(uint8_t preset, uint8_t pitch, float velocity, uint8_t padIdx) {
    if (padIdx < DRUM_NUM_PADS) {
        _padFlashTime[padIdx] = millis();
    }

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.wave = PCM;
    e.preset = preset;
    e.midi_note = pitch;
    e.velocity = velocity * _param_drive;
    amy_add_event(&e);

    needsUIRedraw = true;
}

void InstrumentDrum::noteOn(uint8_t note, float velocity) {
    if (!isActive) return;

    // General MIDI Drum mapping
    switch (note) {
        case 35: case 36: // Bass Drum
            triggerDrum(1, (uint8_t)std::max(1, (int)(60 + _param_kick_tune)), velocity, 0);
            break;
        case 38: case 40: // Snare
            triggerDrum((_currentKit == 1) ? 3 : 2, (uint8_t)std::max(1, (int)(60 + _param_snare_tune)), velocity, 1);
            break;
        case 37: // Side Stick
            triggerDrum(5, 60, velocity, 1);
            break;
        case 39: // Hand Clap
            triggerDrum(9, 60, velocity, 4);
            break;
        case 42: case 44: // Closed Hi-Hat
            triggerDrum(6, 60, velocity, 2);
            break;
        case 46: // Open Hi-Hat
            triggerDrum(7, 60, velocity, 3);
            break;
        case 41: case 43: // Floor Toms
            triggerDrum(8, (uint8_t)std::max(1, (int)(55 + _param_tom_tune)), velocity, 5);
            break;
        case 45: case 47: case 48: case 50: // Mid/Hi Toms
            triggerDrum(8, (uint8_t)std::max(1, (int)(62 + _param_tom_tune)), velocity, 5);
            break;
        case 56: // Cowbell
            triggerDrum(10, 60, velocity, 6);
            break;
        case 69: case 70: // Maracas / Shaker
            triggerDrum(0, 60, velocity, 7);
            break;
        default: {
            // Chromatic keyboard mapping (modulo 8 pads)
            uint8_t pad = (note - 36) % 8;
            if (pad == 0) triggerDrum(1, (uint8_t)(60 + _param_kick_tune), velocity, 0);
            else if (pad == 1) triggerDrum(2, (uint8_t)(60 + _param_snare_tune), velocity, 1);
            else if (pad == 2) triggerDrum(6, 60, velocity, 2);
            else if (pad == 3) triggerDrum(7, 60, velocity, 3);
            else if (pad == 4) triggerDrum(9, 60, velocity, 4);
            else if (pad == 5) triggerDrum(8, (uint8_t)(60 + _param_tom_tune), velocity, 5);
            else if (pad == 6) triggerDrum(10, 60, velocity, 6);
            else if (pad == 7) triggerDrum(0, 60, velocity, 7);
            break;
        }
    }
}

void InstrumentDrum::noteOff(uint8_t note) {
    // Drum one-shot samples play out naturally
}

void InstrumentDrum::drawUI(U8G2 &u8g2) {
    const int GRID_X = 6;
    const int GRID_Y = 16;
    const int PAD_W = 27;
    const int PAD_H = 19;
    const int GAP_X = 2;
    const int GAP_Y = 3;

    unsigned long now = millis();

    for (int i = 0; i < DRUM_NUM_PADS; i++) {
        int col = i % 4;
        int row = i / 4;
        int px = GRID_X + col * (PAD_W + GAP_X);
        int py = GRID_Y + row * (PAD_H + GAP_Y);

        bool isHit = (now - _padFlashTime[i]) < 120;

        if (isHit) {
            // Inverted active flash
            u8g2.setDrawColor(1);
            u8g2.drawRBox(px, py, PAD_W, PAD_H, 2);
            u8g2.setDrawColor(0);
            u8g2.setFont(u8g2_font_7x14B_tr);
            int tw = u8g2.getStrWidth(padLabels[i]);
            u8g2.drawStr(px + (PAD_W - tw) / 2, py + 14, padLabels[i]);
        } else {
            // Idle outline
            u8g2.setDrawColor(1);
            u8g2.drawRFrame(px, py, PAD_W, PAD_H, 2);
            u8g2.setFont(u8g2_font_6x10_tr);
            int tw = u8g2.getStrWidth(padLabels[i]);
            u8g2.drawStr(px + (PAD_W - tw) / 2, py + 13, padLabels[i]);
        }
    }
    u8g2.setDrawColor(1);
}
