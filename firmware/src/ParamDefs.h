#pragma once
// =============================================================================
// ParamDefs.h — Self-describing parameter descriptors with category support
// =============================================================================

#include <Arduino.h>

// Parameter categories for top-level menu navigation
enum ParamCategory : uint8_t {
    CAT_SYNTH      = 0, // Engine-specific controls (Waves, DCO, HPF, Trim, Gain)
    CAT_FILTER_ENV = 1, // Cutoff, Resonance, Attack, Decay, Sustain, Release, LFO
    CAT_FX         = 2, // Reverb, Delay Mix, Delay Time, Feedback
    CAT_COUNT      = 3
};

struct ParamDescriptor {
    const char *name;       // Display name, e.g. "Cutoff", "Attack"
    const char *unit;       // Unit string, e.g. "Hz", "ms", "%", "x"
    float minVal;           // Minimum allowed value
    float maxVal;           // Maximum allowed value
    float step;             // Encoder increment per click (fine)
    float coarseStep;       // Encoder increment when accelerating
    float *valuePtr;        // Pointer to the live value
    bool isInteger;         // If true, display and snap to integers
    ParamCategory category; // Category for tabbed navigation

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
        if (isInteger) {
            snprintf(buf, bufLen, "%d %s", (int)roundf(*valuePtr), unit);
        } else if (step >= 0.1f) {
            snprintf(buf, bufLen, "%.1f %s", *valuePtr, unit);
        } else {
            snprintf(buf, bufLen, "%.2f %s", *valuePtr, unit);
        }
    }
};

// Helper macros for standardized parameter definitions
#define PARAM_MS(name, min, max, step, ptr, cat) \
    { name, "ms", (float)(min), (float)(max), (float)(step), (float)(step) * 5.0f, ptr, true, cat }

#define PARAM_HZ(name, min, max, step, ptr, cat) \
    { name, "Hz", (float)(min), (float)(max), (float)(step), (float)(step) * 10.0f, ptr, ((step) >= 1.0f), cat }

#define PARAM_PCT(name, min, max, step, ptr, cat) \
    { name, "%", (float)(min), (float)(max), (float)(step), (float)(step) * 5.0f, ptr, true, cat }

#define PARAM_FLOAT(name, unit, min, max, step, ptr, cat) \
    { name, unit, min, max, step, (step) * 10.0f, ptr, false, cat }

#define PARAM_INT(name, unit, min, max, ptr, cat) \
    { name, unit, (float)(min), (float)(max), 1.0f, 10.0f, ptr, true, cat }
