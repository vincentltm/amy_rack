#include "InstrumentPiano.h"

void InstrumentPiano::init() {
    buildBaseParams();
}

void InstrumentPiano::start() {
    isActive = true;

    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.num_voices = 5;
    e.patch_number = 256;
    amy_add_event(&e);

    needsUIRedraw = true;
}

void InstrumentPiano::stop() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.velocity = 0.0f;
    amy_add_event(&e);
    isActive = false;
}
