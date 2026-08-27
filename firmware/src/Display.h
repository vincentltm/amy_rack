#pragma once
#include <U8g2lib.h>
#include <Wire.h>
#include "Config.h"
#include "Instrument.h"
#include "ParamDefs.h"

// Forward declarations
class System;
class MidiManager;

class Display {
public:
    Display();
    void begin();
    
    // System-level update
    void update(System& sys, MidiManager& midi);
    void update(System& sys);

    // Direct parameter update
    void update(Instrument* inst, const ParamDescriptor* params, uint8_t paramCount, uint8_t selectedIdx, bool editing, uint8_t midiCh, uint8_t lastNote, bool gateActive);
    
    void drawHeader(const char* instName, const char* patchName);
    void drawInstrumentUI(Instrument* inst);
    void drawParamList(const ParamDescriptor* params, uint8_t count, uint8_t selectedIdx, bool editing);
    void drawStatusBar(uint8_t midiCh, uint8_t lastNote, bool gateActive);
    void drawInstrumentMenu(const char* names[], uint8_t count, uint8_t selected);
    void showSplash();

    U8G2& getU8G2() { return u8g2; }

private:
    U8G2_SH1107_128X128_F_HW_I2C u8g2;
    
    Instrument* lastInst = nullptr;
    uint8_t lastSelectedIdx = 255;
    bool lastEditing = false;
    uint8_t lastMidiCh = 255;
    uint8_t lastNote = 255;
    bool lastGateActive = false;
    uint8_t lastNavState = 255;
    uint8_t lastMenuSel = 255;
    bool needsRedraw = true;
};
