#pragma once
// =============================================================================
// ParamDefs.h — Tabbed Parameter Descriptors with Enum support
// =============================================================================

#include <Arduino.h>

enum TabId : uint8_t {
    TAB_MAIN  = 0, // Master / Engine / Patch / MIDI / CV Dashboard
    TAB_SYNTH = 1, // Engine specific controls (DCO, Waves, Trim, Gain)
    TAB_ENV   = 2, // Filter & Envelope (Cutoff, Res, ADSR, LFO)
    TAB_FX    = 3, // Effects (Reverb, Delay)
    TAB_COUNT = 4
};

struct ParamDescriptor {
    const char *name;       // Display name, e.g. "Cutoff", "Engine"
    const char *unit;       // Unit string, e.g. "Hz", "ms", "%", "x", ""
    float minVal;           // Minimum allowed value
    float maxVal;           // Maximum allowed value
    float step;             // Encoder increment per click (fine)
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
        float s = accelerated ? coarseStep : step;
        setValue(*valuePtr + delta * s);
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

// Helper macros
#define PARAM_MS(name, min, max, step, ptr, tab) \
    { name, "ms", (float)(min), (float)(max), (float)(step), (float)(step) * 5.0f, ptr, true, tab, nullptr, 0 }

#define PARAM_HZ(name, min, max, step, ptr, tab) \
    { name, "Hz", (float)(min), (float)(max), (float)(step), (float)(step) * 10.0f, ptr, ((step) >= 1.0f), tab, nullptr, 0 }

#define PARAM_PCT(name, min, max, step, ptr, tab) \
    { name, "%", (float)(min), (float)(max), (float)(step), (float)(step) * 5.0f, ptr, true, tab, nullptr, 0 }

#define PARAM_FLOAT(name, unit, min, max, step, ptr, tab) \
    { name, unit, min, max, step, (step) * 10.0f, ptr, false, tab, nullptr, 0 }

#define PARAM_INT(name, unit, min, max, ptr, tab) \
    { name, unit, (float)(min), (float)(max), 1.0f, 10.0f, ptr, true, tab, nullptr, 0 }

#define PARAM_ENUM(name, maxIdx, ptr, enumArray, tab) \
    { name, "", 0.0f, (float)(maxIdx), 1.0f, 1.0f, ptr, true, tab, enumArray, (uint8_t)((maxIdx) + 1) }
