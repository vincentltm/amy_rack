#pragma once
// =============================================================================
// Instrument.h — Base instrument class for AMY Rack
// =============================================================================

#include <Arduino.h>
#include <U8g2lib.h>
#include "Config.h"
#include "ParamDefs.h"

extern "C" {
#include <amy.h>
}

struct SynthParams {
    float cutoff         = 2000.0f; // 20 - 10000 Hz
    float resonance      = 1.0f;    // 0.5 - 5.0
    float attack_ms      = 20.0f;   // 1 - 5000 ms
    float decay_ms       = 250.0f;  // 5 - 5000 ms
    float sustain_pct    = 70.0f;   // 0 - 100 %
    float release_ms     = 350.0f;  // 5 - 5000 ms
    float lfo_freq_hz    = 2.0f;    // 0.1 - 20.0 Hz
    float lfo_depth_pct  = 0.0f;    // 0 - 100 %
    float reverb_pct     = 0.0f;    // 0 - 100 %
    float reverb_damping = 50.0f;   // 0 - 100 %
    float delay_mix_pct  = 0.0f;    // 0 - 100 %
    float delay_time_ms  = 350.0f;  // 10 - 1500 ms
    float delay_feedback = 40.0f;   // 0 - 95 %
    float noise_pct      = 0.0f;    // 0 - 100 %
};

class Instrument {
public:
    SynthParams params;
    bool isActive = false;
    bool needsUIRedraw = false;

    virtual ~Instrument() {}

    virtual void init() {}

    virtual void start() {
        isActive = true;
        sendAllParams();
    }

    virtual void stop() {
        isActive = false;
    }

    virtual void update() {}

    virtual void drawUI(U8G2 &u8g2) {}

    virtual void noteOn(uint8_t note, float velocity) {
        if (!isActive) return;
        amy_event e = amy_default_event();
        e.synth = getSynthChannel();
        e.midi_note = note;
        e.velocity = velocity;
        amy_add_event(&e);
    }

    virtual void noteOff(uint8_t note) {
        if (!isActive) return;
        amy_event e = amy_default_event();
        e.synth = getSynthChannel();
        e.midi_note = note;
        e.velocity = 0.0f;
        amy_add_event(&e);
    }

    const char *getName()      const { return _instrumentName; }
    const char *getShortName() const { return _instrumentShortName; }

    virtual const ParamDescriptor *getParams() const { return _baseParams; }
    virtual uint8_t getParamCount()            const { return _baseParamCount; }

    virtual void onParamChanged(uint8_t paramIndex) {
        sendAllParams();
    }

    virtual int  getPatchCount()              const { return 0; }
    virtual int  getCurrentPatch()            const { return 0; }
    virtual void setPatch(int index)                { }
    virtual const char *getPatchName(int idx)  const { return ""; }

    virtual void nextPatch()     { setPatch(getCurrentPatch() + 1); }
    virtual void prevPatch()     { setPatch(getCurrentPatch() - 1); }

    virtual void configReverb() {
        float level = (params.reverb_pct / 100.0f) * 1.5f;
        float damping = params.reverb_damping / 100.0f;
        config_reverb(level, 0.85f, damping, 3000.0f);
    }

    virtual void configDelay() {
        float level = params.delay_mix_pct / 100.0f;
        float fb = (params.delay_feedback / 100.0f) * 0.85f;
        config_echo(level, params.delay_time_ms, 2000.0f, fb, 0.0f);
    }

protected:
    const char *_instrumentName      = "Base";
    const char *_instrumentShortName = "Base";

    virtual uint8_t getSynthChannel() { return SYNTH_CHANNEL_DEFAULT; }

    virtual void sendAllParams() {
        sendAdsr();
        sendFilter();
        configReverb();
        configDelay();
    }

    virtual void sendAdsr() {
        amy_event e = amy_default_event();
        e.synth = getSynthChannel();

        uint16_t a_ms = (uint16_t)fmax(params.attack_ms, 1.0f);
        uint16_t d_ms = (uint16_t)fmax(params.decay_ms, 1.0f);
        uint16_t r_ms = (uint16_t)fmax(params.release_ms, 1.0f);
        float s_val   = constrain(params.sustain_pct / 100.0f, 0.0f, 1.0f);

        e.eg0_times[0]  = a_ms;
        e.eg0_values[0] = 1.0f;
        e.eg0_times[1]  = d_ms;
        e.eg0_values[1] = s_val;
        e.eg0_times[2]  = r_ms;
        e.eg0_values[2] = 0.0f;

        amy_add_event(&e);
    }

    virtual void sendFilter() {
        amy_event e = amy_default_event();
        e.synth = getSynthChannel();
        e.filter_freq_coefs[0] = params.cutoff;
        e.resonance = params.resonance;
        e.filter_type = 1;
        amy_add_event(&e);
    }

    void buildBaseParams() {
        // Tab: ENV (Filter & Envelope)
        _baseParams[0]  = PARAM_HZ("Cutoff",      20.0f, 10000.0f, 50.0f, &params.cutoff,       TAB_ENV);
        _baseParams[1]  = PARAM_FLOAT("Resonance","", 0.5f, 5.0f, 0.1f,   &params.resonance,    TAB_ENV);
        _baseParams[2]  = PARAM_MS("Attack",      1.0f, 4000.0f, 10.0f,   &params.attack_ms,    TAB_ENV);
        _baseParams[3]  = PARAM_MS("Decay",       5.0f, 4000.0f, 20.0f,   &params.decay_ms,     TAB_ENV);
        _baseParams[4]  = PARAM_PCT("Sustain",    0.0f, 100.0f, 5.0f,     &params.sustain_pct,  TAB_ENV);
        _baseParams[5]  = PARAM_MS("Release",     5.0f, 4000.0f, 20.0f,   &params.release_ms,   TAB_ENV);
        _baseParams[6]  = PARAM_HZ("LFO Rate",    0.1f, 20.0f, 0.2f,      &params.lfo_freq_hz,  TAB_ENV);
        _baseParams[7]  = PARAM_PCT("LFO Depth",  0.0f, 100.0f, 5.0f,     &params.lfo_depth_pct,TAB_ENV);

        // Tab: FX (Effects)
        _baseParams[8]  = PARAM_PCT("Reverb",     0.0f, 100.0f, 5.0f,     &params.reverb_pct,    TAB_FX);
        _baseParams[9]  = PARAM_PCT("Rev Damp",   0.0f, 100.0f, 5.0f,     &params.reverb_damping,TAB_FX);
        _baseParams[10] = PARAM_PCT("Delay Mix",  0.0f, 100.0f, 5.0f,     &params.delay_mix_pct, TAB_FX);
        _baseParams[11] = PARAM_MS("Delay Time",  10.0f, 1500.0f, 25.0f,  &params.delay_time_ms, TAB_FX);
        _baseParams[12] = PARAM_PCT("Delay FB",   0.0f, 95.0f, 5.0f,      &params.delay_feedback,TAB_FX);

        _baseParamCount = 13;
    }

    ParamDescriptor _baseParams[MAX_PARAMS];
    uint8_t _baseParamCount = 0;
};
