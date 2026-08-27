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

    // AMY Additive Partials Grand Piano (Patch 256: 24 harmonic partials per voice)
    amy_event e = amy_default_event();
    e.synth = getSynthChannel();
    e.num_voices = 4;
    e.patch_number = 256;
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
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(1);
    u8g2.drawStr(8, 23, "ADDITIVE SPECTRAL PIANO");

    // Draw 14 white piano keys across x=8..120, y=26..58 (height 32px)
    const int startX = 8;
    const int startY = 26;
    const int keyW = 8;
    const int whiteKeyH = 31;
    const int blackKeyH = 18;
    const int blackKeyW = 5;

    for (int i = 0; i < 14; i++) {
        u8g2.drawFrame(startX + i * keyW, startY, keyW + 1, whiteKeyH);
    }

    // Draw black keys (pattern: 2, gap, 3, gap, 2, gap, 3)
    const int blackKeyOffsets[] = {
        1, 2,      // C#, D#
        4, 5, 6,   // F#, G#, A#
        8, 9,      // C#, D#
        11, 12, 13 // F#, G#, A#
    };

    for (int i = 0; i < 10; i++) {
        int k = blackKeyOffsets[i];
        int bx = startX + k * keyW - (blackKeyW / 2);
        u8g2.drawBox(bx, startY, blackKeyW, blackKeyH);
    }
}
