#pragma once
// =============================================================================
// ParamDefs.h — Tabbed Parameter Descriptors with 6 Modular Tabs
// =============================================================================

#include <Arduino.h>

enum TabId : uint8_t {
    TAB_MAIN  = 0, // Synth, Patch, Drums, Synth Vol, Drum Vol
    TAB_SYNTH = 1, // Engine specific controls (DCO, Waves, Trim, Gain)
    TAB_DRUM  = 2, // Drum Machine (Kits, Tuning, Drive, Volume)
    TAB_ENV   = 3, // Filter & Envelope (Cutoff, Res, ADSR, LFO)
    TAB_FX    = 4, // Effects (Chorus, Reverb, Delay)
    TAB_MIDI  = 5, // MIDI Channels (Synth Ch, Drum Ch) & CV Routing
    TAB_COUNT = 6
};

struct ParamDescriptor {
    const char *name;       // Display name, e.g. "Cutoff", "Engine"
    const char *unit;       // Unit string, e.g. "Hz", "ms", "%", "x", ""
    float minVal;           // Minimum allowed value
    float maxVal;           // Maximum allowed value
    float step;             // Encoder increment per click (linear fine)
    float coarseStep;       // Encoder increment when accelerating
    float *valuePtr;        // Pointer to the live value
    bool isInteger;         // If true, display and snap to integers
    TabId tab;              // Associated Tab ID
    const char * const *enumNames = nullptr; // Optional enum string array
    uint8_t enumCount = 0;

    float getValue() const { return *valuePtr; }

    void setValue(float v) const {
        if (v < minVal) v = minVal;
        if (v > maxVal) v = maxVal;
        if (isInteger) v = roundf(v);
        *valuePtr = v;
    }

    void adjust(int delta, bool accelerated = false) const {
        setValue(*valuePtr + delta * step);
    }

    void formatValue(char *buf, size_t bufLen) const {
        if (enumNames && enumCount > 0) {
            int idx = (int)roundf(*valuePtr);
            if (idx >= 0 && idx < enumCount && enumNames[idx]) {
                snprintf(buf, bufLen, "%s", enumNames[idx]);
                return;
            }
        }
        if (isInteger) {
            if (unit && unit[0]) {
                snprintf(buf, bufLen, "%d %s", (int)roundf(*valuePtr), unit);
            } else {
                snprintf(buf, bufLen, "%d", (int)roundf(*valuePtr));
            }
        } else if (step >= 0.1f) {
            if (unit && unit[0]) {
                snprintf(buf, bufLen, "%.1f %s", *valuePtr, unit);
            } else {
                snprintf(buf, bufLen, "%.1f", *valuePtr);
            }
        } else {
            if (unit && unit[0]) {
                snprintf(buf, bufLen, "%.2f %s", *valuePtr, unit);
            } else {
                snprintf(buf, bufLen, "%.2f", *valuePtr);
            }
        }
    }
};

#define PARAM_FLOAT(name, unit, minV, maxV, stepVal, ptr, tabId) \
    ParamDescriptor{ name, unit, minV, maxV, stepVal, (stepVal)*5.0f, ptr, false, tabId, nullptr, 0 }

#define PARAM_INT(name, unit, minV, maxV, ptr, tabId) \
    ParamDescriptor{ name, unit, (float)(minV), (float)(maxV), 1.0f, 5.0f, ptr, true, tabId, nullptr, 0 }

#define PARAM_INT_STEP(name, unit, minV, maxV, stepVal, ptr, tabId) \
    ParamDescriptor{ name, unit, (float)(minV), (float)(maxV), (float)(stepVal), (float)((stepVal)*5), ptr, true, tabId, nullptr, 0 }

#define PARAM_HZ(name, minV, maxV, stepVal, ptr, tabId) \
    ParamDescriptor{ name, "Hz", (float)(minV), (float)(maxV), (float)(stepVal), (float)((stepVal)*5), ptr, true, tabId, nullptr, 0 }

#define PARAM_MS(name, minV, maxV, stepVal, ptr, tabId) \
    ParamDescriptor{ name, "ms", (float)(minV), (float)(maxV), (float)(stepVal), (float)((stepVal)*5), ptr, true, tabId, nullptr, 0 }

#define PARAM_PCT(name, minV, maxV, stepVal, ptr, tabId) \
    ParamDescriptor{ name, "%", minV, maxV, stepVal, (stepVal)*5.0f, ptr, true, tabId, nullptr, 0 }

#define PARAM_ENUM(name, count, ptr, namesArray, tabId) \
    ParamDescriptor{ name, "", 0.0f, (float)((count) - 1), 1.0f, 1.0f, ptr, true, tabId, namesArray, (uint8_t)(count) }
