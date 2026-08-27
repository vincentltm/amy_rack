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

static const char* tabLabels[TAB_COUNT] = {
    "MAIN",
    "SYNTH",
    "ENV",
    "FX"
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
        drawVisualizerArea(inst, activeTab);

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
        drawVisualizerArea(inst, activeTab);
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

void Display::drawVisualizerArea(Instrument* inst, TabId activeTab) {
    if (activeTab == TabId::TAB_MAIN || activeTab == TabId::TAB_SYNTH) {
        if (inst) inst->drawUI(u8g2);
    } else if (activeTab == TabId::TAB_ENV) {
        if (inst) drawFilterEnvPlot(inst->params);
    } else if (activeTab == TabId::TAB_FX) {
        if (inst) drawFXPlot(inst->params);
    }
    u8g2.setDrawColor(1);
    u8g2.drawHLine(0, 61, SCREEN_WIDTH);
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

    // Left: Reverb Room Space
    const int R_X = 2;
    const int R_Y = 15;
    const int R_W = 58;
    const int R_H = 43;

    u8g2.drawFrame(R_X, R_Y, R_W, R_H);
    u8g2.drawStr(R_X + 3, R_Y + 7, "REVERB");

    u8g2.drawFrame(R_X + 8, R_Y + 12, R_W - 16, R_H - 18);
    u8g2.drawLine(R_X, R_Y, R_X + 8, R_Y + 12);
    u8g2.drawLine(R_X + R_W, R_Y, R_X + R_W - 8, R_Y + 12);
    u8g2.drawLine(R_X, R_Y + R_H, R_X + 8, R_Y + R_H - 6);
    u8g2.drawLine(R_X + R_W, R_Y + R_H, R_X + R_W - 8, R_Y + R_H - 6);

    int numParticles = (int)((p.reverb_pct / 100.0f) * 20.0f);
    for (int i = 0; i < numParticles; i++) {
        int px = R_X + 12 + ((i * 17) % (R_W - 24));
        int py = R_Y + 16 + ((i * 23) % (R_H - 24));
        u8g2.drawPixel(px, py);
    }

    // Right: Delay Pulse Train
    const int D_X = 64;
    const int D_Y = 15;
    const int D_W = 62;
    const int D_H = 43;
    const int D_BASE = D_Y + D_H - 4;

    u8g2.drawFrame(D_X, D_Y, D_W, D_H);
    u8g2.drawStr(D_X + 3, D_Y + 7, "DELAY");
    u8g2.drawHLine(D_X + 4, D_BASE, D_W - 8);

    float mix = p.delay_mix_pct / 100.0f;
    float fb  = p.delay_feedback / 100.0f;
    int spacing = (int)((p.delay_time_ms / 1500.0f) * 16.0f) + 6;

    float amp = 24.0f * (mix > 0.05f ? mix : 0.2f);
    for (int i = 0; i < 4; i++) {
        int tx = D_X + 6 + i * spacing;
        if (tx >= D_X + D_W - 4) break;
        int pulseH = (int)amp;
        if (pulseH > 0) {
            u8g2.drawVLine(tx, D_BASE - pulseH, pulseH);
            u8g2.drawDisc(tx, D_BASE - pulseH, 1);
        }
        amp *= (0.3f + fb * 0.65f);
    }
}

void Display::drawTabBar(TabId activeTab, bool tabFocus) {
    const uint8_t tabW = 28;
    const uint8_t tabH = 10;
    const uint8_t gap = 2;
    const uint8_t startX = (SCREEN_WIDTH - (tabW * TAB_COUNT + gap * (TAB_COUNT - 1))) / 2; // 5px
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
