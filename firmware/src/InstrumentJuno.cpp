#include "InstrumentJuno.h"
#include <algorithm>

InstrumentJuno::InstrumentJuno() {
    _instrumentName = "JUNO";
    _instrumentShortName = "JUNO";
}

void InstrumentJuno::init() {
    buildBaseParams();

    // Tab: SYNTH
    _junoParams[0] = PARAM_PCT("DCO PWM",   0.0f, 100.0f, 2.0f, &state.dco_pwm,   TAB_SYNTH);
    _junoParams[1] = PARAM_PCT("Saw Lvl",   0.0f, 100.0f, 5.0f, &state.saw_level, TAB_SYNTH);
    _junoParams[2] = PARAM_PCT("Sub Lvl",   0.0f, 100.0f, 5.0f, &state.dco_sub,   TAB_SYNTH);
    _junoParams[3] = PARAM_PCT("Noise Lvl", 0.0f, 100.0f, 5.0f, &state.dco_noise, TAB_SYNTH);
    _junoParams[4] = PARAM_INT("HPF", "",   0, 3,               &state.hpf,       TAB_SYNTH);

    // Tab: ENV
    _junoParams[5] = PARAM_PCT("VCF Freq",  0.0f, 100.0f, 2.0f, &state.vcf_freq,  TAB_ENV);
    _junoParams[6] = PARAM_PCT("VCF Res",   0.0f, 100.0f, 2.0f, &state.vcf_res,   TAB_ENV);
    _junoParams[7] = PARAM_PCT("VCF LFO",   0.0f, 100.0f, 5.0f, &state.vcf_lfo,   TAB_ENV);
    _junoParams[8] = PARAM_HZ("LFO Rate",   0.1f, 20.0f,  0.2f, &state.lfo_rate,  TAB_ENV);
    _junoParams[9] = PARAM_MS("Env A",      1.0f, 4000.0f,10.0f,&state.env_a_ms,  TAB_ENV);
    _junoParams[10]= PARAM_MS("Env D",      5.0f, 4000.0f,20.0f,&state.env_d_ms,  TAB_ENV);
    _junoParams[11]= PARAM_PCT("Env S",     0.0f, 100.0f, 5.0f, &state.env_s_pct, TAB_ENV);
    _junoParams[12]= PARAM_MS("Env R",      5.0f, 4000.0f,20.0f,&state.env_r_ms,  TAB_ENV);

    _junoParamCount = 13;

    // Add Category: EFFECTS (FX) from baseParams (Reverb, Rev Damp, Delay Mix, Delay Time, Delay FB)
    for (int i = 8; i < _baseParamCount; i++) {
        _junoParams[_junoParamCount++] = _baseParams[i];
    }
}

void InstrumentJuno::start() {
    isActive = true;

    amy_event e = amy_default_event();
    e.synth = 1;
    e.num_voices = 5;
    e.patch_number = patch;
    amy_add_event(&e);

    state.loadFromSysex(patch);

    // Sync base params for ENV dynamic visualizer
    params.cutoff = 50.0f + powf(state.vcf_freq / 100.0f, 2.0f) * 8000.0f;
    params.resonance = 0.7f + (state.vcf_res / 100.0f) * 4.3f;
    params.attack_ms = state.env_a_ms;
    params.decay_ms = state.env_d_ms;
    params.sustain_pct = state.env_s_pct;
    params.release_ms = state.env_r_ms;
    params.lfo_freq_hz = state.lfo_rate;

    updateOscDuty(JUNO_OSC_PWM);
    updateOscAmps(JUNO_OSC_PWM);
    updateOscAmps(JUNO_OSC_SAW);
    updateOscAmps(JUNO_OSC_SUB);
    updateOscAmps(JUNO_OSC_NOISE);
    updateVcf();
    updateAdsr();
    updateLfo();
    updateHpf();

    configReverb();
    configDelay();

    needsUIRedraw = true;
}

void InstrumentJuno::stop() {
    amy_event e = amy_default_event();
    e.synth = 1;
    e.velocity = 0.0f;
    amy_add_event(&e);
    isActive = false;
}

void InstrumentJuno::setPatch(int index) {
    if (index < 0) index = 127;
    if (index > 127) index = 0;
    patch = index;
    start();
}

const char *InstrumentJuno::getPatchName(int idx) const {
    if (idx >= 0 && idx < 128) return juno_patch_names[idx];
    return "";
}

void InstrumentJuno::onParamChanged(uint8_t paramIndex) {
    if (paramIndex == 0) { updateOscDuty(JUNO_OSC_PWM); }
    else if (paramIndex == 1) { updateOscAmps(JUNO_OSC_SAW); }
    else if (paramIndex == 2) { updateOscAmps(JUNO_OSC_SUB); }
    else if (paramIndex == 3) { updateOscAmps(JUNO_OSC_NOISE); }
    else if (paramIndex == 4) { updateHpf(); }
    else if (paramIndex == 5 || paramIndex == 6 || paramIndex == 7) { updateVcf(); }
    else if (paramIndex == 8) { updateLfo(); }
    else if (paramIndex >= 9 && paramIndex <= 12) { updateAdsr(); }
    else if (paramIndex >= 13) {
        configReverb();
        configDelay();
    }
    needsUIRedraw = true;
}

void InstrumentJuno::updateOscAmps(int osc) {
    amy_event e = amy_default_event();
    e.synth = 1;
    e.osc = osc;

    float target_amp = 0.005f;

    switch (osc) {
    case JUNO_OSC_PWM:
        if (state.dco_pwm <= 1.0f) target_amp = 0.005f;
        else target_amp = fmax(state.vca_level, 0.005f);
        break;
    case JUNO_OSC_SAW:
        target_amp = (state.saw_level > 50.0f) ? fmax(state.vca_level, 0.005f) : 0.005f;
        break;
    case JUNO_OSC_SUB:
        target_amp = fmax((state.dco_sub / 100.0f) * state.vca_level, 0.005f);
        break;
    case JUNO_OSC_NOISE:
        target_amp = fmax((state.dco_noise / 100.0f) * state.vca_level, 0.005f);
        break;
    }

    e.amp_coefs[0] = target_amp;
    amy_add_event(&e);
}

void InstrumentJuno::updateOscDuty(int osc) {
    if (osc != JUNO_OSC_PWM) return;
    amy_event e = amy_default_event();
    e.synth = 1;
    e.osc = osc;
    e.duty_coefs[0] = constrain(state.dco_pwm / 100.0f, 0.01f, 0.99f);
    amy_add_event(&e);
}

void InstrumentJuno::updateVcf() {
    amy_event e = amy_default_event();
    e.synth = 1;
    e.osc = JUNO_OSC_PWM;

    float base_freq = 50.0f + powf(state.vcf_freq / 100.0f, 2.0f) * 8000.0f;
    e.filter_freq_coefs[0] = base_freq;
    e.resonance = 0.7f + (state.vcf_res / 100.0f) * 4.3f;
    e.filter_type = 1;

    e.filter_freq_coefs[2] = (state.vcf_lfo / 100.0f) * 2.0f;

    amy_add_event(&e);
}

void InstrumentJuno::updateAdsr() {
    amy_event e = amy_default_event();
    e.synth = 1;

    uint16_t a_ms = (uint16_t)fmax(state.env_a_ms, 1.0f);
    uint16_t d_ms = (uint16_t)fmax(state.env_d_ms, 1.0f);
    uint16_t r_ms = (uint16_t)fmax(state.env_r_ms, 1.0f);
    float s_val   = constrain(state.env_s_pct / 100.0f, 0.0f, 1.0f);

    e.eg0_times[0]  = a_ms;
    e.eg0_values[0] = 1.0f;
    e.eg0_times[1]  = d_ms;
    e.eg0_values[1] = s_val;
    e.eg0_times[2]  = r_ms;
    e.eg0_values[2] = 0.0f;

    amy_add_event(&e);
}

void InstrumentJuno::updateLfo() {
    amy_event e = amy_default_event();
    e.synth = 1;
    e.osc = JUNO_OSC_LFO;
    e.freq_coefs[0] = state.lfo_rate;
    amy_add_event(&e);
}

void InstrumentJuno::updateHpf() {
    amy_event e = amy_default_event();
    e.synth = 1;
    e.osc = JUNO_OSC_PWM;

    float hpf_freq = 20.0f;
    if (state.hpf == 1) hpf_freq = 150.0f;
    else if (state.hpf == 2) hpf_freq = 300.0f;
    else if (state.hpf == 3) hpf_freq = 600.0f;

    e.filter_freq_coefs[0] = hpf_freq;
    e.filter_type = 2; // Highpass
    amy_add_event(&e);
}
