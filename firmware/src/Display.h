#pragma once
#include <U8g2lib.h>
#include <Wire.h>
#include "Config.h"
#include "Instrument.h"
#include "ParamDefs.h"

class System;
class MidiManager;

class Display {
public:
    Display();
    void begin();
    
    void update(System& sys, MidiManager& midi);
    void update(System& sys);
    void markDirty() { needsRedraw = true; }

    void drawHeader(const char* instName, const char* patchName, int patchIndex, uint8_t midiCh, uint8_t lastNote, bool gateActive);
    void drawVisualizerArea(Instrument* inst, TabId activeTab, uint8_t lastNote, bool gateActive);
    void drawMasterKeyboard(uint8_t lastNote, bool gateActive);
    void drawDrumPads();
    void drawFilterEnvPlot(const SynthParams& p);
    void drawFXPlot(const SynthParams& p);
    void drawTabBar(TabId activeTab, bool tabFocus);
    void drawParamList(System& sys, uint8_t count, uint8_t selectedIdx, bool editing, bool hasFocus);
    void drawInstrumentMenu(const char* names[], uint8_t count, uint8_t selected);
    void showSplash();

    U8G2& getU8G2() { return u8g2; }

private:
    U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2;
    
    Instrument* lastInst = nullptr;
    uint8_t lastSelectedIdx = 255;
    TabId lastTab = (TabId)255;
    bool lastEditing = false;
    float lastParamVal = -99999.0f;
    uint8_t lastMidiCh = 255;
    uint8_t lastNote = 255;
    bool lastGateActive = false;
    uint8_t lastNavState = 255;
    int lastPatch = -1;
    bool needsRedraw = true;
};
