#include "Display.h"
#include "System.h"
#include "MidiManager.h"

Display::Display() : u8g2(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN) {
}

void Display::begin() {
    u8g2.begin();
    u8g2.clearBuffer();
    showSplash();
    u8g2.sendBuffer();
    delay(1000);
    u8g2.clearBuffer();
}

void Display::update(System& sys, MidiManager& midi) {
    NavState state = sys.getNavState();
    uint8_t navStateInt = static_cast<uint8_t>(state);

    if (state == NavState::INSTRUMENT_MENU) {
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
    const ParamDescriptor* params = inst ? inst->getParams() : nullptr;
    uint8_t paramCount = inst ? inst->getParamCount() : 0;
    uint8_t selectedIdx = sys.getSelectedParamIndex();
    bool editing = sys.isEditingParam();
    uint8_t midiCh = midi.getLastChannel();
    uint8_t lastNote = midi.getLastNote();
    bool gateActive = midi.isNoteActive();

    update(inst, params, paramCount, selectedIdx, editing, midiCh, lastNote, gateActive);
}

void Display::update(System& sys) {
    NavState state = sys.getNavState();
    uint8_t navStateInt = static_cast<uint8_t>(state);

    if (state == NavState::INSTRUMENT_MENU) {
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
    const ParamDescriptor* params = inst ? inst->getParams() : nullptr;
    uint8_t paramCount = inst ? inst->getParamCount() : 0;
    uint8_t selectedIdx = sys.getSelectedParamIndex();
    bool editing = sys.isEditingParam();

    update(inst, params, paramCount, selectedIdx, editing, 0, 255, false);
}

void Display::update(Instrument* inst, const ParamDescriptor* params, uint8_t paramCount, uint8_t selectedIdx, bool editing, uint8_t midiCh, uint8_t lastNote, bool gateActive) {
    bool dirty = false;
    
    if (inst != lastInst) { dirty = true; lastInst = inst; }
    if (selectedIdx != lastSelectedIdx) { dirty = true; lastSelectedIdx = selectedIdx; }
    if (editing != lastEditing) { dirty = true; lastEditing = editing; }
    if (midiCh != lastMidiCh) { dirty = true; lastMidiCh = midiCh; }
    if (lastNote != this->lastNote) { dirty = true; this->lastNote = lastNote; }
    if (gateActive != lastGateActive) { dirty = true; lastGateActive = gateActive; }
    if (inst && inst->needsUIRedraw) { dirty = true; inst->needsUIRedraw = false; }
    if (needsRedraw) { dirty = true; needsRedraw = false; }

    if (!dirty) return;

    u8g2.clearBuffer();

    if (inst) {
        drawHeader(inst->getName(), inst->getPatchName(inst->getCurrentPatch()));
        drawInstrumentUI(inst);
    }
    
    if (params && paramCount > 0) {
        drawParamList(params, paramCount, selectedIdx, editing);
    }
    
    drawStatusBar(midiCh, lastNote, gateActive);

    u8g2.sendBuffer();
}

void Display::drawHeader(const char* instName, const char* patchName) {
    u8g2.setFont(FONT_HEADER);
    u8g2.setDrawColor(1);
    
    // Instrument Name on left
    if (instName) {
        u8g2.drawStr(0, HEADER_Y + 12, instName);
    }
    
    // Patch name on right
    if (patchName && patchName[0] != '\0') {
        int w = u8g2.getStrWidth(patchName);
        u8g2.drawStr(SCREEN_WIDTH - w, HEADER_Y + 12, patchName);
    }
    
    u8g2.drawHLine(0, HEADER_Y + HEADER_HEIGHT, SCREEN_WIDTH);
}

void Display::drawInstrumentUI(Instrument* inst) {
    if (inst) {
        inst->drawUI(u8g2);
    }
}

void Display::drawParamList(const ParamDescriptor* params, uint8_t count, uint8_t selectedIdx, bool editing) {
    u8g2.setFont(FONT_PARAM_NAME);
    
    uint8_t visibleRows = PARAM_LIST_H / PARAM_LIST_ROW_H;
    uint8_t startIdx = 0;
    
    if (selectedIdx >= visibleRows) {
        startIdx = selectedIdx - visibleRows + 1;
    }
    
    for (uint8_t i = 0; i < visibleRows; i++) {
        uint8_t idx = startIdx + i;
        if (idx >= count) break;
        
        int y = PARAM_LIST_Y + (i * PARAM_LIST_ROW_H);
        
        if (idx == selectedIdx) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(0, y, SCREEN_WIDTH, PARAM_LIST_ROW_H);
            u8g2.setDrawColor(0);
        } else {
            u8g2.setDrawColor(1);
        }
        
        u8g2.drawStr(2, y + 8, params[idx].name);
        
        char valBuf[32];
        params[idx].formatValue(valBuf, sizeof(valBuf));
        
        int w = u8g2.getStrWidth(valBuf);
        u8g2.drawStr(SCREEN_WIDTH - w - 2, y + 8, valBuf);
        
        if (idx == selectedIdx && editing) {
            // Draw a tiny indicator that we are editing
            u8g2.drawStr(SCREEN_WIDTH - w - 10, y + 8, ">");
        }
    }
    u8g2.setDrawColor(1);
}

void Display::drawStatusBar(uint8_t midiCh, uint8_t lastNote, bool gateActive) {
    u8g2.drawHLine(0, STATUS_BAR_Y - 2, SCREEN_WIDTH);
    u8g2.setFont(FONT_STATUS);
    u8g2.setDrawColor(1);
    
    char buf[32];
    if (lastNote != 255) {
        snprintf(buf, sizeof(buf), "CH:%d NOTE:%d GATE:%c", 
                 (midiCh < 16 ? midiCh + 1 : 1), lastNote, gateActive ? '*' : '-');
    } else {
        snprintf(buf, sizeof(buf), "CH:%d NOTE:--- GATE:%c", 
                 (midiCh < 16 ? midiCh + 1 : 1), gateActive ? '*' : '-');
    }
             
    u8g2.drawStr(0, STATUS_BAR_Y + 6, buf);
}

void Display::drawInstrumentMenu(const char* names[], uint8_t count, uint8_t selected) {
    u8g2.clearBuffer();
    u8g2.setFont(FONT_INSTRUMENT_TITLE);
    
    for (uint8_t i = 0; i < count; i++) {
        int y = 24 + i * 22;
        if (i == selected) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(0, y - 16, SCREEN_WIDTH, 20);
            u8g2.setDrawColor(0);
        } else {
            u8g2.setDrawColor(1);
        }
        
        int w = u8g2.getStrWidth(names[i]);
        u8g2.drawStr((SCREEN_WIDTH - w) / 2, y - 2, names[i]);
    }
    
    u8g2.setDrawColor(1);
    u8g2.sendBuffer();
    needsRedraw = true; // force next update to draw full screen
}

void Display::showSplash() {
    u8g2.setFont(FONT_INSTRUMENT_TITLE);
    u8g2.setDrawColor(1);
    const char* text = "AMY Rack v1.0";
    int w = u8g2.getStrWidth(text);
    u8g2.drawStr((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT / 2, text);
}
