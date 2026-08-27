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

static const char* catNames[] = {
    "1. SYNTH CONTROLS",
    "2. FILTER & ENV",
    "3. EFFECTS (FX)"
};

static const char* catShortHeaders[] = {
    "[ SYNTH SETTINGS ]",
    "[ FILTER & ENV ]",
    "[ FX SETTINGS ]"
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

    if (state == NavState::CATEGORY_SELECT) {
        uint8_t catSel = sys.getSelectedCategory();
        bool dirty = (navStateInt != lastNavState) || (catSel != lastCategory) || needsRedraw;
        lastNavState = navStateInt;
        lastCategory = catSel;

        if (dirty) {
            drawCategoryMenu(catSel);
        }
        return;
    }

    lastNavState = navStateInt;

    Instrument* inst = sys.getActiveInstrument();
    const ParamDescriptor* allParams = inst ? inst->getParams() : nullptr;
    uint8_t selectedIdx = sys.getSelectedParamIndex();
    bool editing = sys.isEditingParam();
    uint8_t catIdx = sys.getSelectedCategory();
    uint8_t filteredCount = sys.getFilteredParamCount();
    uint8_t midiCh = midi.getLastChannel();
    uint8_t lastNote = midi.getLastNote();
    bool gateActive = midi.isNoteActive();
    int currentPatch = inst ? inst->getCurrentPatch() : -1;

    bool dirty = (inst != lastInst) || (selectedIdx != lastSelectedIdx) || 
                 (editing != lastEditing) || (catIdx != lastCategory) ||
                 (midiCh != lastMidiCh) || (lastNote != this->lastNote) || 
                 (gateActive != lastGateActive) || (currentPatch != lastPatch) || 
                 (inst && inst->needsUIRedraw) || needsRedraw;

    if (!dirty) return;

    lastInst = inst;
    lastSelectedIdx = selectedIdx;
    lastEditing = editing;
    lastCategory = catIdx;
    lastMidiCh = midiCh;
    this->lastNote = lastNote;
    lastGateActive = gateActive;
    lastPatch = currentPatch;
    if (inst) inst->needsUIRedraw = false;
    needsRedraw = false;

    u8g2.clearBuffer();

    if (inst) {
        drawHeader(inst->getName(), inst->getPatchName(currentPatch), currentPatch);
        drawInstrumentUI(inst);
    }
    
    if (state == NavState::PARAM_SELECT || state == NavState::PARAM_EDIT) {
        uint8_t filteredIndices[MAX_PARAMS];
        for (uint8_t i = 0; i < filteredCount; i++) {
            filteredIndices[i] = sys.getFilteredParamRealIndex(i);
        }
        drawFilteredParamList(allParams, filteredIndices, filteredCount, selectedIdx, editing, catIdx);
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

    if (state == NavState::CATEGORY_SELECT) {
        drawCategoryMenu(sys.getSelectedCategory());
        return;
    }

    Instrument* inst = sys.getActiveInstrument();
    const ParamDescriptor* allParams = inst ? inst->getParams() : nullptr;
    uint8_t selectedIdx = sys.getSelectedParamIndex();
    bool editing = sys.isEditingParam();
    uint8_t catIdx = sys.getSelectedCategory();
    uint8_t filteredCount = sys.getFilteredParamCount();
    int currentPatch = inst ? inst->getCurrentPatch() : -1;

    u8g2.clearBuffer();

    if (inst) {
        drawHeader(inst->getName(), inst->getPatchName(currentPatch), currentPatch);
        drawInstrumentUI(inst);
    }
    
    if (state == NavState::PARAM_SELECT || state == NavState::PARAM_EDIT) {
        uint8_t filteredIndices[MAX_PARAMS];
        for (uint8_t i = 0; i < filteredCount; i++) {
            filteredIndices[i] = sys.getFilteredParamRealIndex(i);
        }
        drawFilteredParamList(allParams, filteredIndices, filteredCount, selectedIdx, editing, catIdx);
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
    
    u8g2.drawHLine(0, 14, SCREEN_WIDTH);
}

void Display::drawInstrumentUI(Instrument* inst) {
    if (inst) {
        inst->drawUI(u8g2);
    }
}

void Display::drawCategoryMenu(uint8_t selectedCat) {
    u8g2.clearBuffer();
    
    // Header
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setDrawColor(1);
    const char* title = "-- SETTINGS MENU --";
    int tw = u8g2.getStrWidth(title);
    u8g2.drawStr((SCREEN_WIDTH - tw) / 2, 12, title);
    u8g2.drawHLine(0, 15, SCREEN_WIDTH);

    // 3 Category Cards
    u8g2.setFont(u8g2_font_7x14B_tr);
    for (uint8_t i = 0; i < 3; i++) {
        int y = 30 + i * 28;
        if (i == selectedCat) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(4, y, SCREEN_WIDTH - 8, 22);
            u8g2.setDrawColor(0); // Inverted
        } else {
            u8g2.setDrawColor(1);
            u8g2.drawFrame(4, y, SCREEN_WIDTH - 8, 22);
        }
        
        int w = u8g2.getStrWidth(catNames[i]);
        u8g2.drawStr((SCREEN_WIDTH - w) / 2, y + 16, catNames[i]);
    }
    
    // Bottom hint
    u8g2.setDrawColor(1);
    u8g2.drawHLine(0, 116, SCREEN_WIDTH);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(6, 125, "Click: Select | Hold: Back");

    u8g2.sendBuffer();
    needsRedraw = true;
}

void Display::drawFilteredParamList(const ParamDescriptor* allParams, const uint8_t* filteredIndices, uint8_t count, uint8_t selectedIdx, bool editing, uint8_t catIdx) {
    u8g2.drawHLine(0, PARAM_LIST_Y - 2, SCREEN_WIDTH);
    u8g2.setFont(FONT_PARAM_NAME);
    
    uint8_t visibleRows = PARAM_LIST_H / PARAM_LIST_ROW_H;
    uint8_t startIdx = 0;
    
    if (selectedIdx >= visibleRows) {
        startIdx = selectedIdx - visibleRows + 1;
    }
    
    for (uint8_t i = 0; i < visibleRows; i++) {
        uint8_t fIdx = startIdx + i;
        if (fIdx >= count) break;
        
        uint8_t realIdx = filteredIndices[fIdx];
        int y = PARAM_LIST_Y + (i * PARAM_LIST_ROW_H);
        bool isSelected = (fIdx == selectedIdx);
        
        if (isSelected) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(0, y, SCREEN_WIDTH, PARAM_LIST_ROW_H);
            u8g2.setDrawColor(0);
        } else {
            u8g2.setDrawColor(1);
        }
        
        if (isSelected && editing) {
            char nameWithIndicator[32];
            snprintf(nameWithIndicator, sizeof(nameWithIndicator), ">%s", allParams[realIdx].name);
            u8g2.drawStr(2, y + 8, nameWithIndicator);
        } else {
            u8g2.drawStr(2, y + 8, allParams[realIdx].name);
        }
        
        char valBuf[32];
        allParams[realIdx].formatValue(valBuf, sizeof(valBuf));
        
        int w = u8g2.getStrWidth(valBuf);
        u8g2.drawStr(SCREEN_WIDTH - w - 2, y + 8, valBuf);
    }
    u8g2.setDrawColor(1);
}

void Display::drawStatusBar(uint8_t midiCh, uint8_t lastNote, bool gateActive, uint8_t navState) {
    u8g2.drawHLine(0, STATUS_BAR_Y - 2, SCREEN_WIDTH);
    u8g2.setFont(FONT_STATUS);
    u8g2.setDrawColor(1);
    
    const char* modeStr = "[PATCH]";
    if (navState == 2) modeStr = "[MENU]";
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
