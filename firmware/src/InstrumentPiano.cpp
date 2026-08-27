#include "InstrumentPiano.h"

InstrumentPiano::InstrumentPiano() {
    _instrumentName = "Piano";
    _instrumentShortName = "PIANO";
}

void InstrumentPiano::init() {
    buildBaseParams();
}

void InstrumentPiano::start() {
    isActive = true;
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.num_voices = 8;
    e.patch_number = 0; // Standard PCM Piano
    amy_add_event(&e);

    sendAllParams();
    needsUIRedraw = true;
}

void InstrumentPiano::stop() {
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.velocity = 0.0f;
    amy_add_event(&e);
    isActive = false;
}

void InstrumentPiano::drawUI(U8G2 &u8g2) {
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setDrawColor(1);
    u8g2.drawStr(12, 32, "Acoustic Grand Piano");
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(12, 48, "AMY Multi-Sampled PCM");
}
