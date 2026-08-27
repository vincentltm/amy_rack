#include "InstrumentJuno.h"
#include "juno_patches.h"
#include <cmath>

InstrumentJuno::InstrumentJuno() {
    _instrumentName = "Juno-106";
    _instrumentShortName = "JUNO";
}

void InstrumentJuno::init() {
    buildBaseParams();

    // Tab: SYNTH (DCO, HPF, VCF, Levels)
    _junoParams[0]  = PARAM_PCT("DCO PWM",     0.0f, 100.0f, 1.0f, &_param_pwm_pct,    TAB_SYNTH);
    _junoParams[1]  = PARAM_PCT("Saw Level",   0.0f, 100.0f, 1.0f, &_param_saw_pct,    TAB_SYNTH);
    _junoParams[2]  = PARAM_PCT("Sub Level",   0.0f, 100.0f, 1.0f, &_param_sub_pct,    TAB_SYNTH);
    _junoParams[3]  = PARAM_PCT("Noise Level", 0.0f, 100.0f, 1.0f, &_param_noise_pct,  TAB_SYNTH);
    _junoParams[4]  = PARAM_INT("HPF Cutoff",  "",   0,      3,    &_param_hpf,        TAB_SYNTH);
    _junoParams[5]  = PARAM_PCT("VCF Cutoff",  0.0f, 100.0f, 1.0f, &_param_cutoff_pct, TAB_SYNTH);
    _junoParams[6]  = PARAM_PCT("VCF Res",     0.0f, 100.0f, 1.0f, &_param_res_pct,    TAB_SYNTH);
    _junoParams[7]  = PARAM_PCT("VCF LFO Mod", 0.0f, 100.0f, 1.0f, &_param_lfo_pct,    TAB_SYNTH);

    _junoParamCount = 8;

    // Add Tab: ENV & FX from baseParams
    for (int i = 0; i < _baseParamCount; i++) {
        _junoParams[_junoParamCount++] = _baseParams[i];
    }
}

void InstrumentJuno::syncStateToParams() {
    _param_pwm_pct = state.dco_pwm * 100.0f;
    _param_saw_pct = state.saw_level * 100.0f;
    _param_sub_pct = state.dco_sub * 100.0f;
    _param_noise_pct = state.dco_noise * 100.0f;
    _param_hpf = (float)state.hpf;
    _param_cutoff_pct = state.vcf_freq * 100.0f;
    _param_res_pct = state.vcf_res * 100.0f;
    _param_lfo_pct = state.vcf_lfo * 100.0f;

    params.attack_ms = 6.0f + 8.0f * (state.env_a * 127.0f);
    params.decay_ms = 80.0f * powf(2.0f, 0.085f * (state.env_d * 127.0f)) - 80.0f;
    params.sustain_pct = state.env_s * 100.0f;
    params.release_ms = 70.0f * powf(2.0f, 0.066f * (state.env_r * 127.0f)) - 70.0f;
    params.cutoff = 13.0f * powf(2.0f, 0.0938f * (state.vcf_freq * 127.0f));
    params.resonance = 0.7f * powf(2.0f, 4.0f * state.vcf_res);
}

void InstrumentJuno::start() {
    isActive = true;

    amy_event e = amy_default_event();
    e.synth = 1;
    e.num_voices = 5;
    e.patch_number = patch;
    amy_add_event(&e);

    state.loadFromSysex(patch);
    syncStateToParams();

    updateOscDuty(JUNO_OSC_PWM);
    updateOscAmps(JUNO_OSC_PWM);
    updateOscAmps(JUNO_OSC_SAW);
    updateOscAmps(JUNO_OSC_SUB);
    updateOscAmps(JUNO_OSC_NOISE);
    updateVcf();
    updateAdsr();
    updateLfo();
    updateHpf();

    configChorus();
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

void InstrumentJuno::setPatch(int patchIndex) {
    if (patchIndex < 0) patchIndex = 127;
    if (patchIndex > 127) patchIndex = 0;
    patch = patchIndex;

    amy_event e = amy_default_event();
    e.synth = 1;
    e.num_voices = 5;
    e.patch_number = patch;
    amy_add_event(&e);

    state.loadFromSysex(patch);
    syncStateToParams();

    updateOscDuty(JUNO_OSC_PWM);
    updateOscAmps(JUNO_OSC_PWM);
    updateOscAmps(JUNO_OSC_SAW);
    updateOscAmps(JUNO_OSC_SUB);
    updateOscAmps(JUNO_OSC_NOISE);
    updateVcf();
    updateAdsr();
    updateLfo();
    updateHpf();

    needsUIRedraw = true;
}

const char* InstrumentJuno::getPatchName(int idx) const {
    if (idx >= 0 && idx < 128) {
        return juno_patch_names[idx];
    }
    return "";
}

void InstrumentJuno::onParamChanged(uint8_t paramIndex) {
    if (paramIndex == 0) {
        state.dco_pwm = _param_pwm_pct / 100.0f;
        updateOscDuty(JUNO_OSC_PWM);
    } else if (paramIndex == 1) {
        state.saw_level = _param_saw_pct / 100.0f;
        updateOscAmps(JUNO_OSC_SAW);
    } else if (paramIndex == 2) {
        state.dco_sub = _param_sub_pct / 100.0f;
        updateOscAmps(JUNO_OSC_SUB);
    } else if (paramIndex == 3) {
        state.dco_noise = _param_noise_pct / 100.0f;
        updateOscAmps(JUNO_OSC_NOISE);
    } else if (paramIndex == 4) {
        state.hpf = (uint8_t)roundf(_param_hpf);
        updateHpf();
    } else if (paramIndex == 5) {
        state.vcf_freq = _param_cutoff_pct / 100.0f;
        updateVcf();
    } else if (paramIndex == 6) {
        state.vcf_res = _param_res_pct / 100.0f;
        updateVcf();
    } else if (paramIndex == 7) {
        state.vcf_lfo = _param_lfo_pct / 100.0f;
        updateVcf();
    } else if (paramIndex >= 8 && paramIndex <= 11) {
        state.env_a = (params.attack_ms - 6.0f) / (8.0f * 127.0f);
        state.env_s = params.sustain_pct / 100.0f;
        updateAdsr();
    } else if (paramIndex >= 14) {
        configChorus();
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
        if (state.dco_pwm <= 0.01f) target_amp = 0.005f;
        else target_amp = fmax(state.vca_level, 0.005f);
        break;
    case JUNO_OSC_SAW:
        target_amp = fmax(state.saw_level * state.vca_level, 0.005f);
        break;
    case JUNO_OSC_SUB:
        target_amp = fmax(state.dco_sub * state.vca_level, 0.005f);
        break;
    case JUNO_OSC_NOISE:
        target_amp = fmax(state.dco_noise * state.vca_level, 0.005f);
        break;
    }

    e.amp_coefs[0] = 0.0f;
    e.amp_coefs[1] = 0.0f;
    e.amp_coefs[2] = target_amp;
    e.amp_coefs[3] = state.vca_gate ? 0.0f : 1.0f;
    e.amp_coefs[4] = state.vca_gate ? 1.0f : 0.0f;
    e.amp_coefs[5] = 0.0f;
    e.amp_coefs[6] = 0.0f;

    amy_add_event(&e);
}

void InstrumentJuno::updateOscDuty(int osc) {
    amy_event e = amy_default_event();
    e.synth = 1;
    e.osc = osc;

    if (state.dco_pwm <= 0.01f) {
        e.duty_coefs[0] = 0.5f;
        e.duty_coefs[5] = 0.0f;
    } else {
        float active_pwm = (state.dco_pwm - 0.02f) / 0.98f;
        if (active_pwm < 0.0f) active_pwm = 0.0f;

        if (state.pwm_manual) {
            e.duty_coefs[0] = 0.5f - (0.45f * active_pwm);
            e.duty_coefs[5] = 0.0f;
        } else {
            e.duty_coefs[0] = 0.5f;
            e.duty_coefs[5] = 0.5f * active_pwm;
        }
    }

    amy_add_event(&e);
}

void InstrumentJuno::updateVcf() {
    amy_event e = amy_default_event();
    e.synth = 1;
    e.osc = JUNO_OSC_PWM;

    e.filter_freq_coefs[0] = 13.0f * powf(2.0f, 0.0938f * (state.vcf_freq * 127.0f));
    e.resonance = 0.7f * powf(2.0f, 4.0f * state.vcf_res);
    e.filter_freq_coefs[5] = 1.25f * state.vcf_lfo;

    amy_add_event(&e);
}

void InstrumentJuno::updateAdsr() {
    uint16_t a_ms = (uint16_t)fmax(6.0f + 8.0f * (state.env_a * 127.0f), 1.0f);
    uint16_t d_ms = (uint16_t)fmax(80.0f * powf(2.0f, 0.085f * (state.env_d * 127.0f)) - 80.0f, 1.0f);
    uint16_t r_ms = (uint16_t)fmax(70.0f * powf(2.0f, 0.066f * (state.env_r * 127.0f)) - 70.0f, 1.0f);

    amy_event e = amy_default_event();
    e.synth = 1;

    e.eg0_times[0]  = a_ms;
    e.eg0_values[0] = 1.0f;
    e.eg0_times[1]  = d_ms;
    e.eg0_values[1] = state.env_s;
    e.eg0_times[2]  = r_ms;
    e.eg0_values[2] = 0.0f;

    e.osc = JUNO_OSC_PWM;   amy_add_event(&e);
    e.osc = JUNO_OSC_SAW;   amy_add_event(&e);
    e.osc = JUNO_OSC_SUB;   amy_add_event(&e);
    e.osc = JUNO_OSC_NOISE; amy_add_event(&e);
}

void InstrumentJuno::updateLfo() {
    amy_event e = amy_default_event();
    e.synth = 1;
    e.osc = JUNO_OSC_LFO;
    e.freq_coefs[0] = fmax(0.6f * powf(2.0f, 0.04f * (state.lfo_rate * 127.0f)) - 0.1f, 0.001f);

    uint16_t delay_ms = (uint16_t)fmax(18.0f * powf(2.0f, 0.066f * (state.lfo_delay_time * 127.0f)) - 13.0f, 1.0f);
    e.eg0_times[0] = delay_ms;
    e.eg0_values[0] = 1.0f;
    e.eg0_times[1] = 10000;
    e.eg0_values[1] = 0.0f;

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
