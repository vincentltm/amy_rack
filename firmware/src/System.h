#pragma once
// =============================================================================
// System.h — Tabbed Architecture with Master Overview on MAIN Tab
// =============================================================================

#include <Arduino.h>
#include "Config.h"
#include "Instrument.h"

class Display;
class EncoderInput;
class CVManager;
class MidiManager;

enum class NavState : uint8_t {
    TAB_SELECT   = 0, // Turning switches MAIN | SYNTH | ENV | FX
    PARAM_SELECT = 1, // Turning scrolls settings in active tab
    PARAM_EDIT   = 2  // Turning edits highlighted setting
};

class System {
public:
    void begin(Display &disp, EncoderInput &enc, CVManager &cv, MidiManager &midi);
    void update();

    // --- State accessors ---
    NavState        getNavState()               const { return _navState; }
    TabId           getActiveTab()              const { return _activeTab; }
    Instrument*     getActiveInstrument()       const { return _instruments[_currentInstrument]; }
    uint8_t         getCurrentInstrumentIndex() const { return _currentInstrument; }
    uint8_t         getNumInstruments()         const { return NUM_INSTRUMENTS; }
    const char*     getInstrumentName(uint8_t i) const;

    // Parameter & Tab state
    uint8_t         getSelectedParamIndex()     const { return _selectedParam; }
    bool            isEditingParam()            const { return _navState == NavState::PARAM_EDIT; }
    uint8_t         getTabParamCount()          const { return _tabParamCount; }
    uint8_t         getTabParamRealIndex(uint8_t idx) const;
    const ParamDescriptor* getTabParamDescriptor(uint8_t idx) const;

    void switchInstrument(uint8_t index);

private:
    Display      *_display  = nullptr;
    EncoderInput *_encoder  = nullptr;
    CVManager    *_cv       = nullptr;
    MidiManager  *_midi     = nullptr;

    NavState _navState   = NavState::TAB_SELECT;
    TabId    _activeTab  = TabId::TAB_MAIN;

    uint8_t     _currentInstrument = DEFAULT_INSTRUMENT;
    Instrument *_instruments[NUM_INSTRUMENTS];
    void initInstruments();

    // Master params for TAB_MAIN
    float _param_engine  = (float)DEFAULT_INSTRUMENT;
    float _param_patch   = 0.0f;
    float _param_midi_ch = 0.0f; // 0 = Ch 1, 15 = Ch 16, 16 = Omni
    float _param_cv1     = 0.0f; // 0 = V/Oct, 1 = Cutoff, 2 = Vol, 3 = Off
    float _param_cv2     = 0.0f; // 0 = Gate, 1 = ModWhl, 2 = Res, 3 = Off

    ParamDescriptor _masterParams[6];
    uint8_t _masterParamCount = 0;
    void initMasterParams();

    // Parameter filtering per tab
    uint8_t _selectedParam  = 0;
    uint8_t _paramScrollTop = 0;
    uint8_t _tabIndices[MAX_PARAMS];
    uint8_t _tabParamCount  = 0;

    void updateTabParams();
    void onMasterParamChanged(uint8_t idx);

    // State machine handlers
    void handleTabSelect();
    void handleParamSelect();
    void handleParamEdit();

    void enterState(NavState newState);
};

extern System Sys;
