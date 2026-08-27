#pragma once
// =============================================================================
// System.h — Tabbed Navigation Architecture for AMY Rack
// =============================================================================

#include <Arduino.h>
#include "Config.h"
#include "Instrument.h"

class Display;
class EncoderInput;
class CVManager;
class MidiManager;

// Hierarchical Navigation States:
// Level 0: ENGINE_MENU   (Switch engine: DX7, Juno, Analog, Sampler, Piano)
// Level 1: TAB_SELECT    (Browse horizontal tabs: MAIN | SYNTH | ENV | FX)
// Level 2: PATCH_BROWSE  (Browse presets when on MAIN tab)
// Level 3: PARAM_SELECT  (Scroll settings inside active tab)
// Level 4: PARAM_EDIT    (Adjust setting value in real-time)
enum class NavState : uint8_t {
    ENGINE_MENU  = 0,
    TAB_SELECT   = 1,
    PATCH_BROWSE = 2,
    PARAM_SELECT = 3,
    PARAM_EDIT   = 4
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

    // Engine menu state
    uint8_t         getMenuSelection()          const { return _menuSelection; }

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

    // Parameter filtering per tab
    uint8_t _selectedParam  = 0;
    uint8_t _paramScrollTop = 0;
    uint8_t _tabIndices[MAX_PARAMS];
    uint8_t _tabParamCount  = 0;

    void updateTabParams();

    // Engine menu
    uint8_t _menuSelection = 0;

    // State machine handlers
    void handleEngineMenu();
    void handleTabSelect();
    void handlePatchBrowse();
    void handleParamSelect();
    void handleParamEdit();

    void enterState(NavState newState);
};

extern System Sys;
