#include "Display.h"
#include "System.h"
#include "MidiManager.h"
#include <cstring>
#include <cmath>
#include <algorithm>

static void cleanString(char* dest, const char* src, size_t maxLen) {
    if (!src) { dest[0] = '\0'; return; }
    size_t len = 0;
    while (*src && len < maxLen - 1) {
        if (*src == '\n' || *src == '\r') break;
        dest[len++] = *src++;
    }
    dest[len] = '\0';
    while (len > 0 && dest[len - 1] == ' ') {
        dest[--len] = '\0';
    }
}

static const char* noteNames[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

static const char* tabLabels[TAB_COUNT] = {
    "MAIN",
    "SYNTH",
    "ENV",
    "FX",
    "MIDI"
};

Display::Display() : u8g2(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN) {
}

void Display::begin() {
    u8g2.begin();
    u8g2.clearBuffer();
    showSplash();
    u8g2.sendBuffer();
    delay(800);
    u8g2.clearBuffer();
}

void Display::update(System& sys, MidiManager& midi) {
    NavState state = sys.getNavState();
    uint8_t navStateInt = static_cast<uint8_t>(state);

    lastNavState = navStateInt;

    Instrument* inst = sys.getActiveInstrument();
    TabId activeTab = sys.getActiveTab();
    uint8_t selectedIdx = sys.getSelectedParamIndex();
    bool editing = sys.isEditingParam();
    uint8_t tabParamCount = sys.getTabParamCount();
    uint8_t midiCh = midi.getChannel();
    uint8_t lastNote = midi.getLastNote();
    bool gateActive = midi.isNoteActive();
    int currentPatch = inst ? inst->getCurrentPatch() : -1;

    bool dirty = (inst != lastInst) || (activeTab != lastTab) ||
                 (selectedIdx != lastSelectedIdx) || (editing != lastEditing) ||
                 (midiCh != lastMidiCh) || (lastNote != this->lastNote) || 
                 (gateActive != lastGateActive) || (currentPatch != lastPatch) || 
                 (inst && inst->needsUIRedraw) || needsRedraw;

    if (!dirty) return;

    lastInst = inst;
    lastTab = activeTab;
    lastSelectedIdx = selectedIdx;
    lastEditing = editing;
    lastMidiCh = midiCh;
    this->lastNote = lastNote;
    lastGateActive = gateActive;
    lastPatch = currentPatch;
    if (inst) inst->needsUIRedraw = false;
    needsRedraw = false;

    u8g2.clearBuffer();

    if (inst) {
        // 1. Top Header with MIDI activity
        drawHeader(inst->getName(), inst->getPatchName(currentPatch), currentPatch, midiCh, lastNote, gateActive);

        // 2. Always-On Top Visualizer (y=14..61)
        drawVisualizerArea(inst, activeTab, lastNote, gateActive);

        // 3. Middle Tab Bar (y=63..74)
        drawTabBar(activeTab, (state == NavState::TAB_SELECT));

        // 4. Parameter List (y=76..127 - 5 full rows)
        bool hasParamFocus = (state == NavState::PARAM_SELECT || state == NavState::PARAM_EDIT);
        drawParamList(sys, tabParamCount, selectedIdx, editing, hasParamFocus);
    }

    u8g2.sendBuffer();
}

void Display::update(System& sys) {
    NavState state = sys.getNavState();
    uint8_t navStateInt = static_cast<uint8_t>(state);

    Instrument* inst = sys.getActiveInstrument();
    TabId activeTab = sys.getActiveTab();
    uint8_t selectedIdx = sys.getSelectedParamIndex();
    bool editing = sys.isEditingParam();
    uint8_t tabParamCount = sys.getTabParamCount();
    int currentPatch = inst ? inst->getCurrentPatch() : -1;

    u8g2.clearBuffer();

    if (inst) {
        drawHeader(inst->getName(), inst->getPatchName(currentPatch), currentPatch, 0, 255, false);
        drawVisualizerArea(inst, activeTab, 255, false);
        drawTabBar(activeTab, (state == NavState::TAB_SELECT));

        bool hasParamFocus = (state == NavState::PARAM_SELECT || state == NavState::PARAM_EDIT);
        drawParamList(sys, tabParamCount, selectedIdx, editing, hasParamFocus);
    }

    u8g2.sendBuffer();
}

void Display::drawHeader(const char* instName, const char* patchName, int patchIndex, uint8_t midiCh, uint8_t lastNote, bool gateActive) {
    u8g2.setFont(u8g2_font_7x14B_tr);
    u8g2.setDrawColor(1);
    
    // Left: Engine Name
    if (instName) {
        u8g2.drawStr(0, 11, instName);
    }
    
    // Middle: Compact MIDI channel & Gate activity dot
    u8g2.setFont(u8g2_font_5x7_tr);
    char midiBuf[8];
    if (midiCh < 16) {
        snprintf(midiBuf, sizeof(midiBuf), "C%d", midiCh + 1);
    } else {
        snprintf(midiBuf, sizeof(midiBuf), "ALL");
    }
    u8g2.drawStr(44, 10, midiBuf);
    if (gateActive) {
        u8g2.drawDisc(60, 7, 2); // Animated MIDI activity dot
    }

    // Right: Patch Name
    char cleanPatch[24];
    cleanString(cleanPatch, patchName, sizeof(cleanPatch));
    
    u8g2.setFont(u8g2_font_6x10_tr);
    if (cleanPatch[0] != '\0') {
        int w = u8g2.getStrWidth(cleanPatch);
        if (w > 62) {
            cleanPatch[10] = '\0';
            w = u8g2.getStrWidth(cleanPatch);
        }
        u8g2.drawStr(SCREEN_WIDTH - w, 10, cleanPatch);
    } else if (patchIndex >= 0) {
        char buf[12];
        snprintf(buf, sizeof(buf), "P:%03d", patchIndex);
        int w = u8g2.getStrWidth(buf);
        u8g2.drawStr(SCREEN_WIDTH - w, 10, buf);
    }
    
    u8g2.drawHLine(0, 12, SCREEN_WIDTH);
}

void Display::drawVisualizerArea(Instrument* inst, TabId activeTab, uint8_t lastNote, bool gateActive) {
    if (activeTab == TabId::TAB_MIDI) {
        drawMasterKeyboard(lastNote, gateActive);
    } else if (activeTab == TabId::TAB_MAIN || activeTab == TabId::TAB_SYNTH) {
        if (inst) inst->drawUI(u8g2);
    } else if (activeTab == TabId::TAB_ENV) {
        if (inst) drawFilterEnvPlot(inst->params);
    } else if (activeTab == TabId::TAB_FX) {
        if (inst) drawFXPlot(inst->params);
    }
    u8g2.setDrawColor(1);
    u8g2.drawHLine(0, 61, SCREEN_WIDTH);
}

// -----------------------------------------------------------------------------
// Interactive Master Keyboard & MIDI Monitor on MAIN Tab
// -----------------------------------------------------------------------------
void Display::drawMasterKeyboard(uint8_t lastNote, bool gateActive) {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(1);

    if (lastNote != 255) {
        int noteIdx = lastNote % 12;
        int oct = (lastNote / 12) - 1;
        float freq = 440.0f * powf(2.0f, ((float)lastNote - 69.0f) / 12.0f);
        char buf[32];
        snprintf(buf, sizeof(buf), "NOTE: %s%d (%.1fHz)%s", noteNames[noteIdx], oct, freq, gateActive ? " *" : "");
        u8g2.drawStr(8, 23, buf);
    } else {
        u8g2.drawStr(8, 23, "KEYBOARD & MIDI MONITOR");
    }

    // 14 White keys across x=8..120 (width 8px per key)
    const int startX = 8;
    const int startY = 26;
    const int keyW = 8;
    const int whiteKeyH = 32;
    const int blackKeyH = 19;
    const int blackKeyW = 5;

    // Determine active key in 2-octave range
    int activeWhiteKey = -1;
    int activeBlackKey = -1;

    if (gateActive && lastNote != 255) {
        int noteInOct = lastNote % 12;
        int octave = (lastNote / 12);
        int octOffset = (octave % 2) * 7;
        int blackOctOffset = (octave % 2) * 5;

        static const int whiteMap[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
        static const int blackMap[12] = {-1, 0, -1, 1, -1, -1, 2, -1, 3, -1, 4, -1};

        if (whiteMap[noteInOct] >= 0) activeWhiteKey = whiteMap[noteInOct] + octOffset;
        if (blackMap[noteInOct] >= 0) activeBlackKey = blackMap[noteInOct] + blackOctOffset;
    }

    for (int i = 0; i < 14; i++) {
        if (i == activeWhiteKey) {
            u8g2.drawBox(startX + i * keyW, startY, keyW + 1, whiteKeyH);
        } else {
            u8g2.drawFrame(startX + i * keyW, startY, keyW + 1, whiteKeyH);
        }
    }

    // Black keys offsets (pattern: 2, gap, 3, gap, 2, gap, 3)
    const int blackKeyOffsets[] = {
        1, 2,      // C#3, D#3
        4, 5, 6,   // F#3, G#3, A#3
        8, 9,      // C#4, D#4
        11, 12, 13 // F#4, G#4, A#4
    };

    for (int i = 0; i < 10; i++) {
        int k = blackKeyOffsets[i];
        int bx = startX + k * keyW - (blackKeyW / 2);

        if (i == activeBlackKey) {
            // Active black key: draw highlighted with white dot
            u8g2.setDrawColor(1);
            u8g2.drawBox(bx, startY, blackKeyW, blackKeyH);
            u8g2.setDrawColor(0);
            u8g2.drawDisc(bx + blackKeyW / 2, startY + blackKeyH - 4, 1);
            u8g2.setDrawColor(1);
        } else {
            u8g2.setDrawColor(1);
            u8g2.drawBox(bx, startY, blackKeyW, blackKeyH);
        }
    }
}

void Display::drawFilterEnvPlot(const SynthParams& p) {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(1);

    // Left: VCF Bode Plot
    const int F_X = 2;
    const int F_Y = 15;
    const int F_W = 58;
    const int F_H = 43;
    const int F_BASE = F_Y + F_H - 2;

    u8g2.drawFrame(F_X, F_Y, F_W, F_H);
    u8g2.drawStr(F_X + 3, F_Y + 7, "VCF");

    float logCutoff = (log10f(p.cutoff) - 1.3f) / (4.0f - 1.3f);
    logCutoff = constrain(logCutoff, 0.05f, 0.95f);
    int cutoffPixel = F_X + 2 + (int)(logCutoff * (F_W - 6));

    int peakHeight = (int)((p.resonance - 0.5f) / 4.5f * 14.0f);
    int passbandY = F_BASE - 18;

    u8g2.drawLine(F_X + 2, passbandY, cutoffPixel - 4, passbandY);
    u8g2.drawLine(cutoffPixel - 4, passbandY, cutoffPixel, passbandY - peakHeight);
    u8g2.drawLine(cutoffPixel, passbandY - peakHeight, F_X + F_W - 3, F_BASE);
    u8g2.drawDisc(cutoffPixel, passbandY - peakHeight, 2);

    // Right: ADSR Envelope Plot
    const int E_X = 64;
    const int E_Y = 15;
    const int E_W = 62;
    const int E_H = 43;
    const int E_BASE = E_Y + E_H - 2;

    u8g2.drawFrame(E_X, E_Y, E_W, E_H);
    u8g2.drawStr(E_X + 3, E_Y + 7, "ENV");

    float totalTime = p.attack_ms + p.decay_ms + p.release_ms + 400.0f;
    int graphW = E_W - 8;
    int aW = std::max(2, (int)((p.attack_ms / totalTime) * graphW));
    int dW = std::max(2, (int)((p.decay_ms / totalTime) * graphW));
    int sW = 12;
    int rW = std::max(2, (int)((p.release_ms / totalTime) * graphW));

    int x0 = E_X + 4;
    int y0 = E_BASE;
    int x1 = x0 + aW;
    int y1 = E_Y + 8;
    int x2 = x1 + dW;
    int y2 = y1 + (int)((1.0f - (p.sustain_pct / 100.0f)) * (E_BASE - y1));
    int x3 = x2 + sW;
    int y3 = y2;
    int x4 = std::min(x3 + rW, E_X + E_W - 4);
    int y4 = E_BASE;

    u8g2.drawLine(x0, y0, x1, y1);
    u8g2.drawLine(x1, y1, x2, y2);
    u8g2.drawLine(x2, y2, x3, y3);
    u8g2.drawLine(x3, y3, x4, y4);

    u8g2.drawDisc(x1, y1, 2);
    u8g2.drawDisc(x2, y2, 2);
    u8g2.drawDisc(x3, y3, 2);
}

void Display::drawFXPlot(const SynthParams& p) {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(1);

    // 1. Left: Chorus Visualizer (x=2..40, w=39)
    const int C_X = 2;
    const int C_Y = 15;
    const int C_W = 39;
    const int C_H = 43;
    const int C_MID = C_Y + C_H / 2 + 3;

    u8g2.drawFrame(C_X, C_Y, C_W, C_H);
    u8g2.drawStr(C_X + 2, C_Y + 7, "CHO");

    float choLvl = p.chorus_pct / 100.0f;
    int choAmp = (int)(choLvl * 10.0f) + 2;

    int prevL = C_MID;
    int prevR = C_MID;
    for (int i = 0; i <= C_W - 6; i++) {
        float angle = ((float)i / (float)(C_W - 6)) * 2.0f * 3.14159265f;
        int curL = C_MID - (int)(sinf(angle) * choAmp);
        int curR = C_MID - (int)(sinf(angle + 1.57f) * (choAmp * 0.75f));
        int px = C_X + 3 + i;
        if (i > 0) {
            u8g2.drawLine(px - 1, prevL, px, curL);
            u8g2.drawPixel(px, curR);
        }
        prevL = curL;
        prevR = curR;
    }

    // 2. Middle: Reverb Visualizer (x=43..83, w=41)
    const int R_X = 43;
    const int R_Y = 15;
    const int R_W = 41;
    const int R_H = 43;
    const int R_BASE = R_Y + R_H - 3;

    u8g2.drawFrame(R_X, R_Y, R_W, R_H);
    u8g2.drawStr(R_X + 2, R_Y + 7, "REV");
    u8g2.drawHLine(R_X + 3, R_BASE, R_W - 6);

    float revLevel = p.reverb_pct / 100.0f;
    float damp = 1.0f - (p.reverb_damping / 100.0f * 0.5f);
    int maxH = (int)(revLevel * 18.0f) + 2;

    int impulses[] = { (int)(maxH * 0.95f), (int)(maxH * 0.75f), (int)(maxH * 0.6f) };
    for (int i = 0; i < 3; i++) {
        int ix = R_X + 5 + i * 3;
        u8g2.drawVLine(ix, R_BASE - impulses[i], impulses[i]);
    }

    int prevTailY = R_BASE - (int)(maxH * 0.5f);
    for (int i = 0; i < R_W - 16; i++) {
        float t = (float)i / (float)(R_W - 16);
        float decay = expf(-t * (2.8f / damp));
        int tailH = (int)(maxH * 0.5f * decay);
        int curTailY = R_BASE - tailH;
        int px = R_X + 14 + i;
        u8g2.drawLine(px - 1, prevTailY, px, curTailY);
        if (i % 2 == 0 && tailH > 2) {
            u8g2.drawVLine(px, curTailY, tailH);
        }
        prevTailY = curTailY;
    }

    // 3. Right: Delay Pulse Train (x=86..126, w=41)
    const int D_X = 86;
    const int D_Y = 15;
    const int D_W = 40;
    const int D_H = 43;
    const int D_BASE = D_Y + D_H - 3;

    u8g2.drawFrame(D_X, D_Y, D_W, D_H);
    u8g2.drawStr(D_X + 2, D_Y + 7, "DLY");
    u8g2.drawHLine(D_X + 3, D_BASE, D_W - 6);

    float mix = p.delay_mix_pct / 100.0f;
    float fb  = p.delay_feedback / 100.0f;
    int spacing = (int)((p.delay_time_ms / 1500.0f) * 10.0f) + 5;

    float amp = 20.0f * (mix > 0.05f ? mix : 0.2f);
    for (int i = 0; i < 4; i++) {
        int tx = D_X + 5 + i * spacing;
        if (tx >= D_X + D_W - 3) break;
        int pulseH = (int)amp;
        if (pulseH > 0) {
            u8g2.drawVLine(tx, D_BASE - pulseH, pulseH);
            u8g2.drawDisc(tx, D_BASE - pulseH, 1);
        }
        amp *= (0.25f + fb * 0.7f);
    }
}

void Display::drawTabBar(TabId activeTab, bool tabFocus) {
    const uint8_t tabW = 22;
    const uint8_t tabH = 10;
    const uint8_t gap = 2;
    const uint8_t startX = (SCREEN_WIDTH - (tabW * TAB_COUNT + gap * (TAB_COUNT - 1))) / 2; // (128 - (110 + 8)) / 2 = 5px
    const uint8_t y = TAB_BAR_Y;

    u8g2.setFont(FONT_TAB);

    for (uint8_t i = 0; i < TAB_COUNT; i++) {
        uint8_t x = startX + i * (tabW + gap);
        bool isActive = (i == (uint8_t)activeTab);

        if (isActive) {
            if (tabFocus) {
                u8g2.setDrawColor(1);
                u8g2.drawBox(x, y, tabW, tabH);
                u8g2.setDrawColor(0);
            } else {
                u8g2.setDrawColor(1);
                u8g2.drawFrame(x, y, tabW, tabH);
                u8g2.drawBox(x + 2, y + tabH - 2, tabW - 4, 2);
                u8g2.setDrawColor(1);
            }
        } else {
            u8g2.setDrawColor(1);
        }

        int tw = u8g2.getStrWidth(tabLabels[i]);
        u8g2.drawStr(x + (tabW - tw) / 2, y + 8, tabLabels[i]);
        u8g2.setDrawColor(1);
    }

    u8g2.drawHLine(0, TAB_BAR_Y + TAB_BAR_H + 1, SCREEN_WIDTH);
}

void Display::drawParamList(System& sys, uint8_t count, uint8_t selectedIdx, bool editing, bool hasFocus) {
    if (count == 0) {
        u8g2.setFont(FONT_PARAM_NAME);
        u8g2.setDrawColor(1);
        u8g2.drawStr(12, PARAM_LIST_Y + 18, "No Parameters");
        return;
    }

    u8g2.setFont(FONT_PARAM_NAME);
    
    uint8_t visibleRows = 5; // 5 full rows
    uint8_t startIdx = 0;
    
    if (selectedIdx >= visibleRows) {
        startIdx = selectedIdx - visibleRows + 1;
    }
    
    for (uint8_t i = 0; i < visibleRows; i++) {
        uint8_t tIdx = startIdx + i;
        if (tIdx >= count) break;
        
        const ParamDescriptor* param = sys.getTabParamDescriptor(tIdx);
        if (!param) continue;

        int y = PARAM_LIST_Y + (i * PARAM_ROW_H);
        bool isSelected = (tIdx == selectedIdx && hasFocus);
        
        if (isSelected) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(0, y, SCREEN_WIDTH, PARAM_ROW_H);
            u8g2.setDrawColor(0);
        } else {
            u8g2.setDrawColor(1);
        }
        
        if (isSelected && editing) {
            char nameWithIndicator[32];
            snprintf(nameWithIndicator, sizeof(nameWithIndicator), ">%s", param->name);
            u8g2.drawStr(2, y + 8, nameWithIndicator);
        } else {
            u8g2.drawStr(2, y + 8, param->name);
        }
        
        char valBuf[32];
        param->formatValue(valBuf, sizeof(valBuf));
        
        int w = u8g2.getStrWidth(valBuf);
        u8g2.drawStr(SCREEN_WIDTH - w - 2, y + 8, valBuf);
    }
    u8g2.setDrawColor(1);
}

void Display::drawInstrumentMenu(const char* names[], uint8_t count, uint8_t selected) {
    u8g2.clearBuffer();
    
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setDrawColor(1);
    const char* title = "-- SELECT ENGINE --";
    int tw = u8g2.getStrWidth(title);
    u8g2.drawStr((SCREEN_WIDTH - tw) / 2, 10, title);
    u8g2.drawHLine(0, 13, SCREEN_WIDTH);

    u8g2.setFont(u8g2_font_7x14B_tr);
    for (uint8_t i = 0; i < count; i++) {
        int y = 28 + i * 19;
        if (i == selected) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(4, y - 12, SCREEN_WIDTH - 8, 16);
            u8g2.setDrawColor(0);
        } else {
            u8g2.setDrawColor(1);
            u8g2.drawFrame(4, y - 12, SCREEN_WIDTH - 8, 16);
        }
        
        int w = u8g2.getStrWidth(names[i]);
        u8g2.drawStr((SCREEN_WIDTH - w) / 2, y, names[i]);
    }
    
    u8g2.setDrawColor(1);
    u8g2.sendBuffer();
    needsRedraw = true;
}

void Display::showSplash() {
    u8g2.setFont(u8g2_font_helvB14_tr);
    u8g2.setDrawColor(1);
    const char* title = "AMY RACK";
    int w = u8g2.getStrWidth(title);
    u8g2.drawStr((SCREEN_WIDTH - w) / 2, 50, title);

    u8g2.setFont(u8g2_font_6x10_tr);
    const char* sub = "Eurorack Synthesizer";
    w = u8g2.getStrWidth(sub);
    u8g2.drawStr((SCREEN_WIDTH - w) / 2, 70, sub);

    const char* ver = "v1.0";
    w = u8g2.getStrWidth(ver);
    u8g2.drawStr((SCREEN_WIDTH - w) / 2, 90, ver);
}
