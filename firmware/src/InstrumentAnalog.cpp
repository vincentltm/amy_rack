#include "InstrumentAnalog.h"
#include <cmath>

InstrumentAnalog::InstrumentAnalog() {
    _instrumentName = "Analog";
    _instrumentShortName = "ANLG";
}

void InstrumentAnalog::init() {
    buildBaseParams();

    _analogParams[0] = PARAM_INT("Osc1 Wave", "", 0, 4, &_osc1_wave_f);
    _analogParams[1] = PARAM_INT("Osc2 Wave", "", 0, 4, &_osc2_wave_f);
    _analogParams[2] = PARAM_PERCENT("Detune", &_osc2_detune);
    _analogParams[3] = PARAM_PERCENT("Balance", &_osc_balance);

    _analogParamCount = 4;

    for (int i = 0; i < _baseParamCount; i++) {
        _analogParams[_analogParamCount++] = _baseParams[i];
    }
}

void InstrumentAnalog::start() {
    isActive = true;
    setupSynthVoices();
}

void InstrumentAnalog::stop() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.velocity = 0.0f;
    amy_add_event(&e);
    isActive = false;
}

void InstrumentAnalog::onParamChanged(uint8_t paramIndex) {
    if (paramIndex == 0) updateOsc1Wave();
    else if (paramIndex == 1) updateOsc2Wave();
    else if (paramIndex == 2) updateOscDetune();
    else if (paramIndex == 3) updateOscBalance();
    else if (paramIndex == 4 || paramIndex == 5) {
        amy_event e = amy_default_event();
        e.synth = getSynthChannel();
        e.filter_freq_coefs[0] = params.cutoff;
        e.resonance = params.resonance;
        e.filter_type = 1;
        amy_add_event(&e);
    }
    else if (paramIndex >= 6 && paramIndex <= 9) sendAdsr();
    else if (paramIndex == 10 || paramIndex == 11) configLfo();
    else if (paramIndex == 12 || paramIndex == 13 || paramIndex == 14) {
        configReverb();
        configDelay();
    }
    else if (paramIndex == 15) configNoise();

    needsUIRedraw = true;
}

void InstrumentAnalog::setupSynthVoices() {
    amy_event e = amy_default_event();
    e.reset_osc = RESET_PATCH;
    e.patch_number = 1024;
    amy_add_event(&e);

    e = amy_default_event();
    e.patch_number = 1024;
    e.oscs_per_voice = 4;
    e.synth = getSynthChannel();
    e.num_voices = 8;
    amy_add_event(&e);

    e = amy_default_event();
    e.osc = OSC_1;
    e.synth = getSynthChannel();
    e.wave = (uint8_t)_osc1_wave_f;
    e.amp_coefs[0] = 0.5f; 
    e.amp_coefs[1] = 1.0f; 
    e.amp_coefs[2] = 1.0f; 
    e.chained_osc = OSC_2;
    e.mod_source = OSC_LFO_FILTER;
    amy_add_event(&e);

    e = amy_default_event();
    e.osc = OSC_2;
    e.synth = getSynthChannel();
    e.wave = (uint8_t)_osc2_wave_f;
    e.amp_coefs[0] = 0.5f;
    e.amp_coefs[1] = 1.0f;
    e.amp_coefs[2] = 1.0f;
    e.chained_osc = OSC_NOISE;
    e.mod_source = OSC_LFO_FILTER;
    amy_add_event(&e);

    e = amy_default_event();
    e.osc = OSC_NOISE;
    e.synth = getSynthChannel();
    e.wave = NOISE;
    e.amp_coefs[0] = params.noise * 0.5f;
    e.amp_coefs[1] = 1.0f;
    e.amp_coefs[2] = 1.0f;
    e.chained_osc = OSC_LFO_FILTER;
    amy_add_event(&e);

    e = amy_default_event();
    e.osc = OSC_LFO_FILTER;
    e.synth = getSynthChannel();
    e.wave = SINE;
    e.freq_coefs[0] = params.lfoFreq * 10.0f;
    e.amp_coefs[0] = params.lfoAmp;
    e.freq_coefs[3] = 0;
    e.amp_coefs[3] = 0;
    amy_add_event(&e);

    e = amy_default_event();
    e.synth = getSynthChannel();
    e.filter_freq_coefs[4] = 1.0f;
    amy_add_event(&e);

    updateOscDetune();
    updateOscBalance();
    configNoise();
    configLfo();
}

void InstrumentAnalog::sendAdsr() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();

    uint16_t a_ms = (uint16_t)fmax(6.0f + 8.0f * (params.attack * 127.0f), 1.0f);
    uint16_t d_ms = (uint16_t)fmax(80.0f * powf(2.0f, 0.085f * (params.decay * 127.0f)) - 80.0f, 1.0f);
    uint16_t r_ms = (uint16_t)fmax(70.0f * powf(2.0f, 0.066f * (params.release * 127.0f)) - 70.0f, 1.0f);

    e.eg0_times[0] = a_ms;
    e.eg0_values[0] = 1.0f;
    e.eg0_times[1] = d_ms;
    e.eg0_values[1] = params.sustain;
    e.eg0_times[2] = r_ms;
    e.eg0_values[2] = 0.0f;

    for (int i = 0; i < 4; i++) {
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
    const float DETUNE_DEADZONE = 0.03f;
    const float HALF_RANGE = 0.5f - DETUNE_DEADZONE;
    float octaves;

    if (_osc2_detune < 0.5f - DETUNE_DEADZONE) {
        float t = _osc2_detune / HALF_RANGE;
        octaves = t - 1.0f;
    } else if (_osc2_detune > 0.5f + DETUNE_DEADZONE) {
        float t = (_osc2_detune - (0.5f + DETUNE_DEADZONE)) / HALF_RANGE;
        octaves = t;
    } else {
        octaves = 0.0f;
    }

    float freq_mult = powf(2.0f, octaves);

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_2;
    e.freq_coefs[0] = freq_mult * 440.0f;
    amy_add_event(&e);
}

void InstrumentAnalog::updateOscBalance() {
    float amp1, amp2;
    if (_osc_balance < 0.5f) {
        amp1 = 1.0f;
        amp2 = _osc_balance * 2.0f;
    } else {
        amp1 = (1.0f - _osc_balance) * 2.0f;
        amp2 = 1.0f;
    }

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_1;
    e.amp_coefs[0] = amp1 * 0.5f;
    amy_add_event(&e);

    e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_2;
    e.amp_coefs[0] = amp2 * 0.5f;
    amy_add_event(&e);
}

void InstrumentAnalog::configNoise() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_NOISE;
    e.amp_coefs[0] = params.noise * 0.5f;
    amy_add_event(&e);
}

void InstrumentAnalog::configLfo() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.osc = OSC_LFO_FILTER;
    e.freq_coefs[0] = params.lfoFreq * 20.0f;
    e.amp_coefs[0] = params.lfoAmp * 2.0f + 0.001f;
    amy_add_event(&e);
}
