#pragma once
// =============================================================================
// System.h — Navigation state machine and instrument management
// =============================================================================

#include <Arduino.h>
#include "Config.h"
#include "Instrument.h"

// Forward declarations
class Display;
class EncoderInput;
class CVManager;
class MidiManager;

// Hierarchical Navigation States:
// Level 0: ENGINE_MENU   (Top level engine picker)
// Level 1: PATCH_SELECT  (Main engine preset viewer)
// Level 2: PARAM_SELECT  (Settings browser)
// Level 3: PARAM_EDIT    (Setting value adjustment)
enum class NavState : uint8_t {
    ENGINE_MENU  = 0,   // Level 0: Select Synth Engine
    PATCH_SELECT = 1,   // Level 1: Browse Presets / Main Screen
    PARAM_SELECT = 2,   // Level 2: Scroll Parameters / Settings
    PARAM_EDIT   = 3    // Level 3: Edit Setting Value
};

class System {
public:
    void begin(Display &disp, EncoderInput &enc, CVManager &cv, MidiManager &midi);
    void update();

    // --- State accessors (for Display) ---
    NavState        getNavState()          const { return _navState; }
    Instrument*     getActiveInstrument()  const { return _instruments[_currentInstrument]; }
    uint8_t         getCurrentInstrumentIndex() const { return _currentInstrument; }
    uint8_t         getNumInstruments()     const { return NUM_INSTRUMENTS; }
    const char*     getInstrumentName(uint8_t i) const;

    // Parameter list state
    uint8_t         getSelectedParamIndex() const { return _selectedParam; }
    bool            isEditingParam()        const { return _navState == NavState::PARAM_EDIT; }

    // Engine menu state
    uint8_t         getMenuSelection()      const { return _menuSelection; }

    // --- External triggers ---
    void switchInstrument(uint8_t index);

private:
    Display      *_display  = nullptr;
    EncoderInput *_encoder  = nullptr;
    CVManager    *_cv       = nullptr;
    MidiManager  *_midi     = nullptr;

    NavState _navState = NavState::PATCH_SELECT;

    uint8_t     _currentInstrument = DEFAULT_INSTRUMENT;
    Instrument *_instruments[NUM_INSTRUMENTS];
    void initInstruments();

    // Parameter selection
    uint8_t _selectedParam  = 0;
    uint8_t _paramScrollTop = 0;

    // Engine menu
    uint8_t _menuSelection = 0;

    // State machine handlers
    void handleEngineMenu();
    void handlePatchSelect();
    void handleParamSelect();
    void handleParamEdit();

    void enterState(NavState newState);
};

extern System Sys;
