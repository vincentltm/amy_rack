#include "Display.h"
#include "System.h"
#include "MidiManager.h"
#include <cstring>

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

    if (state == NavState::ENGINE_MENU) {
        uint8_t menuSel = sys.getMenuSelection();
        bool dirty = (navStateInt != lastNavState) || (menuSel != lastMenuSel) || needsRedraw;
        lastNavState = navStateInt;
        lastMenuSel = menuSel;

        if (dirty) {
            const char* names[NUM_INSTRUMENTS];
            for (uint8_t i = 0; i < NUM_INSTRUMENTS; i++) {
                names[i] = sys.getInstrumentName(i);
            }
            drawInstrumentMenu(names, NUM_INSTRUMENTS, menuSel);
        }
        return;
    }

    lastNavState = navStateInt;

    Instrument* inst = sys.getActiveInstrument();
    TabId activeTab = sys.getActiveTab();
    const ParamDescriptor* allParams = inst ? inst->getParams() : nullptr;
    uint8_t selectedIdx = sys.getSelectedParamIndex();
    bool editing = sys.isEditingParam();
    uint8_t tabParamCount = sys.getTabParamCount();
    uint8_t midiCh = midi.getLastChannel();
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
        drawHeader(inst->getName(), inst->getPatchName(currentPatch), currentPatch);
        drawTabBar(activeTab, (state == NavState::TAB_SELECT));

        if (activeTab == TabId::TAB_MAIN) {
            drawInstrumentUI(inst);
        } else {
            uint8_t tabIndices[MAX_PARAMS];
            for (uint8_t i = 0; i < tabParamCount; i++) {
                tabIndices[i] = sys.getTabParamRealIndex(i);
            }
            bool hasParamFocus = (state == NavState::PARAM_SELECT || state == NavState::PARAM_EDIT);
            drawTabParams(allParams, tabIndices, tabParamCount, selectedIdx, editing, hasParamFocus);
        }
    }
    
    drawStatusBar(midiCh, lastNote, gateActive, navStateInt);

    u8g2.sendBuffer();
}

void Display::update(System& sys) {
    NavState state = sys.getNavState();
    uint8_t navStateInt = static_cast<uint8_t>(state);

    if (state == NavState::ENGINE_MENU) {
        uint8_t menuSel = sys.getMenuSelection();
        const char* names[NUM_INSTRUMENTS];
        for (uint8_t i = 0; i < NUM_INSTRUMENTS; i++) {
            names[i] = sys.getInstrumentName(i);
        }
        drawInstrumentMenu(names, NUM_INSTRUMENTS, menuSel);
        return;
    }

    Instrument* inst = sys.getActiveInstrument();
    TabId activeTab = sys.getActiveTab();
    const ParamDescriptor* allParams = inst ? inst->getParams() : nullptr;
    uint8_t selectedIdx = sys.getSelectedParamIndex();
    bool editing = sys.isEditingParam();
    uint8_t tabParamCount = sys.getTabParamCount();
    int currentPatch = inst ? inst->getCurrentPatch() : -1;

    u8g2.clearBuffer();

    if (inst) {
        drawHeader(inst->getName(), inst->getPatchName(currentPatch), currentPatch);
        drawTabBar(activeTab, (state == NavState::TAB_SELECT));

        if (activeTab == TabId::TAB_MAIN) {
            drawInstrumentUI(inst);
        } else {
            uint8_t tabIndices[MAX_PARAMS];
            for (uint8_t i = 0; i < tabParamCount; i++) {
                tabIndices[i] = sys.getTabParamRealIndex(i);
            }
            bool hasParamFocus = (state == NavState::PARAM_SELECT || state == NavState::PARAM_EDIT);
            drawTabParams(allParams, tabIndices, tabParamCount, selectedIdx, editing, hasParamFocus);
        }
    }
    
    drawStatusBar(0, 255, false, navStateInt);

    u8g2.sendBuffer();
}

void Display::drawHeader(const char* instName, const char* patchName, int patchIndex) {
    u8g2.setFont(u8g2_font_7x14B_tr);
    u8g2.setDrawColor(1);
    
    if (instName) {
        u8g2.drawStr(0, 11, instName);
    }
    
    char cleanPatch[24];
    cleanString(cleanPatch, patchName, sizeof(cleanPatch));
    
    u8g2.setFont(u8g2_font_6x10_tr);
    if (cleanPatch[0] != '\0') {
        int w = u8g2.getStrWidth(cleanPatch);
        if (w > 80) {
            cleanPatch[12] = '\0';
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
                // Focused on Tab Bar: solid filled active tab
                u8g2.setDrawColor(1);
                u8g2.drawBox(x, y, tabW, tabH);
                u8g2.setDrawColor(0);
            } else {
                // In tab content: framed active tab with bottom indicator
                u8g2.setDrawColor(1);
                u8g2.drawFrame(x, y, tabW, tabH);
                u8g2.drawBox(x + 2, y + tabH - 2, tabW - 4, 2);
                u8g2.setDrawColor(1);
            }
        } else {
            u8g2.setDrawColor(1);
            // Inactive tabs
        }

        int tw = u8g2.getStrWidth(tabLabels[i]);
        u8g2.drawStr(x + (tabW - tw) / 2, y + 8, tabLabels[i]);
        u8g2.setDrawColor(1);
    }

    u8g2.drawHLine(0, TAB_BAR_Y + TAB_BAR_H + 1, SCREEN_WIDTH);
}

void Display::drawInstrumentUI(Instrument* inst) {
    if (inst) {
        inst->drawUI(u8g2);
    }
}

void Display::drawTabParams(const ParamDescriptor* allParams, const uint8_t* tabIndices, uint8_t count, uint8_t selectedIdx, bool editing, bool hasFocus) {
    if (count == 0) {
        u8g2.setFont(FONT_PARAM_NAME);
        u8g2.setDrawColor(1);
        u8g2.drawStr(12, CONTENT_Y + 25, "No Parameters");
        return;
    }

    u8g2.setFont(FONT_PARAM_NAME);
    
    uint8_t visibleRows = 6;
    uint8_t startIdx = 0;
    
    if (selectedIdx >= visibleRows) {
        startIdx = selectedIdx - visibleRows + 1;
    }
    
    for (uint8_t i = 0; i < visibleRows; i++) {
        uint8_t tIdx = startIdx + i;
        if (tIdx >= count) break;
        
        uint8_t realIdx = tabIndices[tIdx];
        int y = CONTENT_Y + (i * PARAM_ROW_H);
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
            snprintf(nameWithIndicator, sizeof(nameWithIndicator), ">%s", allParams[realIdx].name);
            u8g2.drawStr(2, y + 10, nameWithIndicator);
        } else {
            u8g2.drawStr(2, y + 10, allParams[realIdx].name);
        }
        
        char valBuf[32];
        allParams[realIdx].formatValue(valBuf, sizeof(valBuf));
        
        int w = u8g2.getStrWidth(valBuf);
        u8g2.drawStr(SCREEN_WIDTH - w - 2, y + 10, valBuf);
    }
    u8g2.setDrawColor(1);
}

void Display::drawStatusBar(uint8_t midiCh, uint8_t lastNote, bool gateActive, uint8_t navState) {
    u8g2.drawHLine(0, STATUS_BAR_Y - 2, SCREEN_WIDTH);
    u8g2.setFont(FONT_STATUS);
    u8g2.setDrawColor(1);
    
    const char* modeStr = "[TAB]";
    if (navState == 2) modeStr = "[PATCH]";
    else if (navState == 3) modeStr = "[PARAM]";
    else if (navState == 4) modeStr = "[EDIT]";

    char buf[36];
    if (lastNote != 255) {
        snprintf(buf, sizeof(buf), "%s CH:%d N:%d %c", 
                 modeStr, (midiCh < 16 ? midiCh + 1 : 1), lastNote, gateActive ? '*' : ' ');
    } else {
        snprintf(buf, sizeof(buf), "%s CH:%d --", 
                 modeStr, (midiCh < 16 ? midiCh + 1 : 1));
    }
             
    u8g2.drawStr(0, STATUS_BAR_Y + 6, buf);
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
