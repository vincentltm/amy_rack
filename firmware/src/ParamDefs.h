#pragma once
// =============================================================================
// ParamDefs.h — Self-describing parameter descriptors for encoder-driven UI
// =============================================================================
// Each instrument exposes an array of ParamDescriptor structs so the Display
// can render a generic scrollable parameter list without knowing instrument
// internals.  The encoder scrolls through the list, and edits values via the
// min/max/step constraints defined here.
// =============================================================================

#include <Arduino.h>

// A single editable parameter exposed to the encoder UI
struct ParamDescriptor {
    const char *name;       // Display name, e.g. "Cutoff"
    const char *unit;       // Unit string, e.g. "Hz", "%", "ms", "" for none
    float minVal;           // Minimum allowed value
    float maxVal;           // Maximum allowed value
    float step;             // Encoder increment per click (fine)
    float coarseStep;       // Encoder increment when accelerating
    float *valuePtr;        // Pointer to the live value in SynthParams
    bool isInteger;         // If true, display and snap to integers

    // Get the current value
    float getValue() const { return *valuePtr; }

    // Set value with clamping
    void setValue(float v) const {
        if (v < minVal) v = minVal;
        if (v > maxVal) v = maxVal;
        if (isInteger) v = roundf(v);
        *valuePtr = v;
    }

    // Increment by delta encoder clicks (positive or negative)
    // Uses coarseStep when |delta| > 1 (acceleration)
    void adjust(int delta, bool accelerated = false) const {
        float s = accelerated ? coarseStep : step;
        setValue(*valuePtr + delta * s);
    }

    // Get value as a formatted string for display
    void formatValue(char *buf, size_t bufLen) const {
        if (isInteger) {
            snprintf(buf, bufLen, "%d%s%s",
                     (int)roundf(*valuePtr),
                     unit[0] ? " " : "", unit);
        } else if (step >= 0.1f) {
            snprintf(buf, bufLen, "%.1f%s%s",
                     *valuePtr,
                     unit[0] ? " " : "", unit);
        } else {
            snprintf(buf, bufLen, "%.2f%s%s",
                     *valuePtr,
                     unit[0] ? " " : "", unit);
        }
    }
};

// Helper macros for common parameter types
#define PARAM_FLOAT(name, unit, min, max, step, ptr) \
    { name, unit, min, max, step, (step) * 10.0f, ptr, false }

#define PARAM_INT(name, unit, min, max, ptr) \
    { name, unit, (float)(min), (float)(max), 1.0f, 10.0f, ptr, true }

#define PARAM_PERCENT(name, ptr) \
    { name, "%", 0.0f, 1.0f, 0.01f, 0.1f, ptr, false }

#define PARAM_FREQ(name, min, max, ptr) \
    { name, "Hz", min, max, 10.0f, 100.0f, ptr, false }

#define PARAM_TIME_MS(name, min, max, ptr) \
    { name, "ms", min, max, 1.0f, 50.0f, ptr, true }
