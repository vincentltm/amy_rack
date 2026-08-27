#include "InstrumentAnalog.h"
#include "analog_patches.h"
#include <cmath>

static const char* waveNames[5] = { "Sine", "Pulse", "Saw", "Tri", "Noise" };

InstrumentAnalog::InstrumentAnalog() {
    _instrumentName = "Analog";
    _instrumentShortName = "ANLG";
}

void InstrumentAnalog::init() {
    buildBaseParams();

    // Default params
    _osc1_wave_f = 2.0f; // SAW
    _osc2_wave_f = 2.0f; // SAW
    _osc2_detune_pct = 25.0f;
    _osc_balance_pct = 50.0f;
    params.noise_pct = 0.0f;
    params.cutoff = 1800.0f;
    params.resonance = 1.2f;
    params.attack_ms = 80.0f;
    params.decay_ms = 350.0f;
    params.sustain_pct = 60.0f;
    params.release_ms = 300.0f;

    // Tab: SYNTH
    _analogParams[0] = PARAM_ENUM("Osc 1 Wave", 4, &_osc1_wave_f, waveNames, TAB_SYNTH);
    _analogParams[1] = PARAM_ENUM("Osc 2 Wave", 4, &_osc2_wave_f, waveNames, TAB_SYNTH);
    _analogParams[2] = PARAM_PCT("Detune", 0.0f, 100.0f, 1.0f, &_osc2_detune_pct, TAB_SYNTH);
    _analogParams[3] = PARAM_PCT("Osc Mix", 0.0f, 100.0f, 1.0f, &_osc_balance_pct, TAB_SYNTH);
    _analogParams[4] = PARAM_PCT("Noise Level", 0.0f, 100.0f, 1.0f, &params.noise_pct, TAB_SYNTH);

    _analogParamCount = 5;

    // Tab: ENV & FX from baseParams
    for (int i = 0; i < _baseParamCount; i++) {
        _analogParams[_analogParamCount++] = _baseParams[i];
    }
}

void InstrumentAnalog::start() {
    isActive = true;
    setupSynthVoices();
    needsUIRedraw = true;
}

void InstrumentAnalog::stop() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.velocity = 0.0f;
    amy_add_event(&e);
    isActive = false;
}

void InstrumentAnalog::setPatch(int patchIndex) {
    if (patchIndex < 0) patchIndex = 11;
    if (patchIndex > 11) patchIndex = 0;
    _currentPatch = patchIndex;

    const AnalogPreset& p = analog_presets[_currentPatch];
    _osc1_wave_f = (float)p.osc1_wave;
    _osc2_wave_f = (float)p.osc2_wave;
    _osc2_detune_pct = p.osc2_detune_pct;
    _osc_balance_pct = p.osc_balance_pct;
    params.noise_pct = p.noise_pct;
    params.cutoff = p.cutoff_hz;
    params.resonance = p.resonance;
    params.attack_ms = p.attack_ms;
    params.decay_ms = p.decay_ms;
    params.sustain_pct = p.sustain_pct;
    params.release_ms = p.release_ms;
    params.voice_mode = (float)p.voice_mode;
    params.glide_ms = p.glide_ms;
    params.chorus_pct = p.chorus_pct;
    params.reverb_pct = p.reverb_pct;
    params.delay_mix_pct = p.delay_mix_pct;

    setupSynthVoices();
    needsUIRedraw = true;
}

const char* InstrumentAnalog::getPatchName(int idx) const {
    if (idx >= 0 && idx < 12) {
        return analog_presets[idx].name;
    }
    return "";
}

void InstrumentAnalog::drawWaveShape(U8G2 &u8g2, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t waveType) {
    u8g2.drawFrame(x, y, w, h);
    int midY = y + h / 2;
    int topY = y + 3;
    int botY = y + h - 4;
    
    switch (waveType) {
        case 0: { // SINE
            int prevY = midY;
            for (int i = 0; i <= w - 8; i++) {
                float angle = ((float)i / (float)(w - 8)) * 2.0f * 3.14159265f;
                int curY = midY - (int)(sinf(angle) * (h / 2 - 4));
                if (i > 0) {
                    u8g2.drawLine(x + 4 + i - 1, prevY, x + 4 + i, curY);
                }
                prevY = curY;
            }
            break;
        }
        case 1: { // PULSE
            int midX = x + w / 2;
            u8g2.drawVLine(x + 4, topY, botY - topY + 1);
            u8g2.drawHLine(x + 4, topY, midX - (x + 4));
            u8g2.drawVLine(midX, topY, botY - topY + 1);
            u8g2.drawHLine(midX, botY, (x + w - 4) - midX);
            u8g2.drawVLine(x + w - 4, topY, botY - topY + 1);
            break;
        }
        case 2: { // SAW
            int midX = x + w / 2;
            u8g2.drawLine(x + 4, botY, midX, topY);
            u8g2.drawVLine(midX, topY, botY - topY + 1);
            u8g2.drawLine(midX, botY, x + w - 4, topY);
            u8g2.drawVLine(x + w - 4, topY, botY - topY + 1);
            break;
        }
        case 3: { // TRIANGLE
            int q1X = x + 4 + (w - 8) / 4;
            int q3X = x + 4 + 3 * (w - 8) / 4;
            u8g2.drawLine(x + 4, midY, q1X, topY);
            u8g2.drawLine(q1X, topY, q3X, botY);
            u8g2.drawLine(q3X, botY, x + w - 4, midY);
            break;
        }
        default: { // NOISE
            for (int i = x + 4; i < x + w - 4; i += 2) {
                int r = ((i * 37 + 11) % (h - 8)) - (h / 2 - 4);
                u8g2.drawPixel(i, midY + r);
            }
            break;
        }
    }
}

void InstrumentAnalog::drawUI(U8G2 &u8g2) {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(1);

    uint8_t w1 = (uint8_t)_osc1_wave_f;
    if (w1 > 4) w1 = 0;
    u8g2.setCursor(8, 23);
    u8g2.printf("OSC1: %s", waveNames[w1]);
    drawWaveShape(u8g2, 8, 25, 52, 23, w1);

    uint8_t w2 = (uint8_t)_osc2_wave_f;
    if (w2 > 4) w2 = 0;
    u8g2.setCursor(68, 23);
    u8g2.printf("OSC2: %s", waveNames[w2]);
    drawWaveShape(u8g2, 68, 25, 52, 23, w2);

    u8g2.setCursor(8, 57);
    u8g2.print("BAL");
    u8g2.drawFrame(30, 51, 30, 7);
    int balFill = (int)((_osc_balance_pct / 100.0f) * 28.0f);
    u8g2.drawBox(31, 52, balFill, 5);

    u8g2.setCursor(70, 57);
    u8g2.printf("DET: %.0f%%", _osc2_detune_pct);
}

void InstrumentAnalog::onParamChanged(uint8_t paramIndex) {
    if (paramIndex == 0) updateOsc1Wave();
    else if (paramIndex == 1) updateOsc2Wave();
    else if (paramIndex == 2) updateOscDetune();
    else if (paramIndex == 3) updateOscBalance();
    else if (paramIndex == 4) configNoise();
    else if (paramIndex == 5 || paramIndex == 6) sendFilter();
    else if (paramIndex >= 7 && paramIndex <= 10) sendAdsr();
    else if (paramIndex == 11 || paramIndex == 12) configLfo();
    else if (paramIndex >= 13) {
        configChorus();
        configReverb();
        configDelay();
    }

    needsUIRedraw = true;
}

void InstrumentAnalog::setupSynthVoices() {
    amy_event e = amy_default_event();
    e.reset_osc = RESET_PATCH;
    e.patch_number = 1024;
    amy_add_event(&e);

    // 1. Osc 1 - with ADSR via EG0
    e = amy_default_event();
    e.osc = OSC_1;
    e.patch_number = 1024;
    e.wave = (uint8_t)_osc1_wave_f;
    e.amp_coefs[COEF_CONST] = 0.5f;
    e.amp_coefs[COEF_VEL] = 1.0f;
    e.amp_coefs[COEF_EG0] = 1.0f;
    e.chained_osc = OSC_2;
    e.mod_source = OSC_LFO_FILTER;
    amy_add_event(&e);

    // 2. Osc 2 - with ADSR via EG0
    e = amy_default_event();
    e.osc = OSC_2;
    e.patch_number = 1024;
    e.wave = (uint8_t)_osc2_wave_f;
    e.amp_coefs[COEF_CONST] = 0.5f;
    e.amp_coefs[COEF_VEL] = 1.0f;
    e.amp_coefs[COEF_EG0] = 1.0f;
    e.chained_osc = OSC_NOISE;
    e.mod_source = OSC_LFO_FILTER;
    amy_add_event(&e);

    // 3. Noise
    e = amy_default_event();
    e.osc = OSC_NOISE;
    e.patch_number = 1024;
    e.wave = NOISE;
    float noiseAmp = (params.noise_pct / 100.0f) * 0.5f;
    if (noiseAmp < 0.001f) {
        e.amp_coefs[COEF_CONST] = 0.0001f;
        e.amp_coefs[COEF_VEL] = 0.0f;
        e.amp_coefs[COEF_EG0] = 0.0f;
    } else {
        e.amp_coefs[COEF_CONST] = noiseAmp;
        e.amp_coefs[COEF_VEL] = 1.0f;
        e.amp_coefs[COEF_EG0] = 1.0f;
    }
    e.chained_osc = OSC_LFO_FILTER;
    amy_add_event(&e);

    // 4. LFO Filter modulator
    e = amy_default_event();
    e.osc = OSC_LFO_FILTER;
    e.patch_number = 1024;
    e.wave = TRIANGLE;
    e.freq_coefs[COEF_CONST] = params.lfo_freq_hz;
    e.amp_coefs[COEF_CONST] = (params.lfo_depth_pct / 100.0f) * 1.5f;
    e.freq_coefs[COEF_NOTE] = 0;
    e.amp_coefs[COEF_NOTE] = 0;
    amy_add_event(&e);

    // 5. Instantiate patch 1024 across synth voices
    e = amy_default_event();
    e.patch_number = 1024;
    e.oscs_per_voice = 4;
    e.synth = getSynthChannel();
    e.num_voices = (params.voice_mode > 0.5f) ? 1 : 6;
    e.portamento_ms = (uint16_t)params.glide_ms;
    amy_add_event(&e);

    updateOscDetune();
    updateOscBalance();
    sendAdsr();
    sendFilter();
    configChorus();
    configReverb();
    configDelay();
}

void InstrumentAnalog::sendAdsr() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();

    uint16_t a_ms = (uint16_t)fmax(params.attack_ms, 2.0f);
    uint16_t d_ms = (uint16_t)fmax(params.decay_ms, 5.0f);
    uint16_t r_ms = (uint16_t)fmax(params.release_ms, 5.0f);
    float s_val   = constrain(params.sustain_pct / 100.0f, 0.0f, 1.0f);

    e.eg0_times[0]  = a_ms;
    e.eg0_values[0] = 1.0f;
    e.eg0_times[1]  = d_ms;
    e.eg0_values[1] = s_val;
    e.eg0_times[2]  = r_ms;
    e.eg0_values[2] = 0.0f;

    for (int i = 0; i < 3; i++) {
        e.osc = i;
        amy_add_event(&e);
    }
}

void InstrumentAnalog::sendFilter() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.filter_freq_coefs[COEF_CONST] = params.cutoff;
    e.resonance = params.resonance;
    e.filter_type = 1; // 24dB / octave lowpass
    for (int i = 0; i < 3; i++) {
        e.osc = i;
        amy_add_event(&e);
    }
}

void InstrumentAnalog::updateOsc1Wave() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_1;
    e.wave = (uint8_t)_osc1_wave_f;
    amy_add_event(&e);
}

void InstrumentAnalog::updateOsc2Wave() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_2;
    e.wave = (uint8_t)_osc2_wave_f;
    amy_add_event(&e);
}

void InstrumentAnalog::updateOscDetune() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_2;
    float detuneRatio = 1.0f + ((_osc2_detune_pct / 100.0f) * 0.06f);
    e.ratio = detuneRatio;
    amy_add_event(&e);
}

void InstrumentAnalog::updateOscBalance() {
    float bal = _osc_balance_pct / 100.0f;
    float amp1 = (1.0f - bal);
    float amp2 = bal;

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_1;
    e.amp_coefs[COEF_CONST] = amp1;
    amy_add_event(&e);

    e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_2;
    e.amp_coefs[COEF_CONST] = amp2;
    amy_add_event(&e);
}

void InstrumentAnalog::configNoise() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_NOISE;
    float noiseAmp = (params.noise_pct / 100.0f) * 0.5f;
    if (noiseAmp < 0.001f) {
        e.amp_coefs[COEF_CONST] = 0.0001f;
        e.amp_coefs[COEF_VEL] = 0.0f;
        e.amp_coefs[COEF_EG0] = 0.0f;
    } else {
        e.amp_coefs[COEF_CONST] = noiseAmp;
        e.amp_coefs[COEF_VEL] = 1.0f;
        e.amp_coefs[COEF_EG0] = 1.0f;
    }
    amy_add_event(&e);
}

void InstrumentAnalog::configLfo() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_LFO_FILTER;
    e.freq_coefs[COEF_CONST] = params.lfo_freq_hz;
    e.amp_coefs[COEF_CONST] = (params.lfo_depth_pct / 100.0f) * 1.5f;
    amy_add_event(&e);
}
