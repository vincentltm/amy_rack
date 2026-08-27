#pragma once
// =============================================================================
// Instrument.h — Base instrument class for AMY Rack
// =============================================================================
// Adapted from Spark Synth's Instrument.h.  Key changes:
//   - Removed U8g2 dependency from drawUI() — uses forward-declared Display
//   - Removed joystick, button handlers (replaced by encoder)
//   - Added ParamDescriptor array for encoder-driven parameter editing
//   - Kept AMY event-based noteOn/noteOff/ADSR/filter logic intact
// =============================================================================

#include <Arduino.h>
#include <U8g2lib.h>
#include "Config.h"
#include "ParamDefs.h"

extern "C" {
#include <amy.h>
}

// ---------------------------------------------------------------------------
// SynthParams — holds all synth parameters for save/load (matches Spark)
// ---------------------------------------------------------------------------
struct SynthParams {
    float cutoff    = 2000.0f;
    float resonance = 1.0f;
    float attack    = 0.05f;    // Seconds (mapped to ms for AMY)
    float decay     = 0.2f;
    float sustain   = 0.5f;    // Level 0.0–1.0
    float release   = 0.5f;
    float reverb    = 0.0f;
    float delayAmp  = 0.0f;
    float delayFreq = 0.0f;
    float noise     = 0.0f;
    float lfoAmp    = 0.0f;
    float lfoFreq   = 0.0f;
    float custom[4] = {0.5f, 0.5f, 0.5f, 0.5f};
};

// ---------------------------------------------------------------------------
// Instrument — abstract base class
// ---------------------------------------------------------------------------
class Instrument {
public:
    SynthParams params;
    bool isActive = false;
    bool needsUIRedraw = false;

    virtual ~Instrument() {}

    // --- Lifecycle ---
    virtual void init() {}

    virtual void start() {
        isActive = true;
        sendAllParams();
    }

    virtual void stop() {
        isActive = false;
    }

    virtual void update() {}

    // --- Display ---
    // Draw instrument-specific UI into the instrument area (y = INSTRUMENT_UI_Y,
    // height = INSTRUMENT_UI_H).  The U8g2 reference is passed directly so
    // instrument UIs ported from Spark need minimal changes.
    virtual void drawUI(U8G2 &u8g2) {}

    // --- Notes (AMY synth allocator) ---
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

    // --- Identity ---
    const char *getName()      const { return _instrumentName; }
    const char *getShortName() const { return _instrumentShortName; }

    // --- Parameters (for encoder UI) ---
    // Subclasses override to provide their parameter list
    virtual const ParamDescriptor *getParams() const { return _baseParams; }
    virtual uint8_t getParamCount()            const { return _baseParamCount; }

    // Called after a parameter value changes (via encoder or MIDI CC)
    virtual void onParamChanged(uint8_t paramIndex) {
        // Rebuild whichever AMY config the changed param affects
        sendAllParams();
    }

    // --- Patch browsing ---
    virtual int  getPatchCount()              const { return 0; }
    virtual int  getCurrentPatch()            const { return 0; }
    virtual void setPatch(int index)                { }
    virtual const char *getPatchName(int idx)  const { return ""; }

    // Navigate patches (encoder shortcut)
    virtual void nextPatch()     { setPatch(getCurrentPatch() + 1); }
    virtual void prevPatch()     { setPatch(getCurrentPatch() - 1); }

protected:
    const char *_instrumentName      = "Base";
    const char *_instrumentShortName = "Base";

    virtual uint8_t getSynthChannel() { return SYNTH_CHANNEL_DEFAULT; }

    // --- AMY parameter senders (from Spark) ---

    void sendAllParams() {
        sendAdsr();
        sendFilter();
    }

    virtual void sendAdsr() {
        amy_event e = amy_default_event();
        e.synth = getSynthChannel();

        // Juno-style ADSR curves (from Spark)
        uint16_t a_ms = (uint16_t)fmax(6.0f + 8.0f * (params.attack * 127.0f), 1.0f);
        uint16_t d_ms = (uint16_t)fmax(80.0f * powf(2.0f, 0.085f * (params.decay * 127.0f)) - 80.0f, 1.0f);
        uint16_t r_ms = (uint16_t)fmax(70.0f * powf(2.0f, 0.066f * (params.release * 127.0f)) - 70.0f, 1.0f);

        e.eg0_times[0]  = a_ms;
        e.eg0_values[0] = 1.0f;
        e.eg0_times[1]  = d_ms;
        e.eg0_values[1] = params.sustain;
        e.eg0_times[2]  = r_ms;
        e.eg0_values[2] = 0.0f;

        amy_add_event(&e);
    }

    virtual void sendFilter() {
        amy_event e = amy_default_event();
        e.synth = getSynthChannel();
        e.filter_freq_coefs[0] = params.cutoff;
        e.resonance = params.resonance;
        e.filter_type = 1;  // Lowpass
        amy_add_event(&e);
    }

    void configReverb() {
        if (params.reverb < 0.05f) return;
        config_reverb(params.reverb * 2.0f, 0.85f, 0.5f, 3000.0f);
    }

    void configDelay() {
        if (params.delayAmp < 0.05f && params.delayFreq < 0.05f) return;
        config_echo(params.delayAmp, params.delayFreq, 3000.0f, params.delayAmp * 0.8f, 0.0f);
    }

    // --- Base parameter descriptors (common to all instruments) ---
    // Subclasses prepend their own custom params before these

    void buildBaseParams() {
        _baseParams[0]  = PARAM_FREQ("Cutoff",    20.0f, 8000.0f, &params.cutoff);
        _baseParams[1]  = PARAM_FLOAT("Resonance", "",  0.5f, 5.0f, 0.1f, &params.resonance);
        _baseParams[2]  = PARAM_PERCENT("Attack",          &params.attack);
        _baseParams[3]  = PARAM_PERCENT("Decay",           &params.decay);
        _baseParams[4]  = PARAM_PERCENT("Sustain",         &params.sustain);
        _baseParams[5]  = PARAM_PERCENT("Release",         &params.release);
        _baseParams[6]  = PARAM_PERCENT("LFO Rate",        &params.lfoFreq);
        _baseParams[7]  = PARAM_PERCENT("LFO Depth",       &params.lfoAmp);
        _baseParams[8]  = PARAM_PERCENT("Reverb",          &params.reverb);
        _baseParams[9]  = PARAM_PERCENT("Delay Mix",       &params.delayAmp);
        _baseParams[10] = PARAM_FREQ("Delay Time",  0.0f, 2000.0f, &params.delayFreq);
        _baseParams[11] = PARAM_PERCENT("Volume",          &params.noise);
        _baseParamCount = 12;
    }

    ParamDescriptor _baseParams[MAX_PARAMS];
    uint8_t _baseParamCount = 0;
};
