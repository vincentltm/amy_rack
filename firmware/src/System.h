#pragma once
// =============================================================================
// System.h — Navigation state machine with Category & FX Menu support
// =============================================================================

#include <Arduino.h>
#include "Config.h"
#include "Instrument.h"

class Display;
class EncoderInput;
class CVManager;
class MidiManager;

// Hierarchical Navigation States:
// Level 0: ENGINE_MENU     (Choose engine: DX7, Juno, Analog, Sampler, Piano)
// Level 1: PATCH_SELECT    (Main engine preset viewer)
// Level 2: CATEGORY_SELECT (Top Settings Menu: SYNTH | FILTER/ENV | EFFECTS)
// Level 3: PARAM_SELECT    (Settings list within chosen category)
// Level 4: PARAM_EDIT      (Adjust setting value)
enum class NavState : uint8_t {
    ENGINE_MENU     = 0,
    PATCH_SELECT    = 1,
    CATEGORY_SELECT = 2,
    PARAM_SELECT    = 3,
    PARAM_EDIT      = 4
};

class System {
public:
    void begin(Display &disp, EncoderInput &enc, CVManager &cv, MidiManager &midi);
    void update();

    // --- State accessors ---
    NavState        getNavState()               const { return _navState; }
    Instrument*     getActiveInstrument()       const { return _instruments[_currentInstrument]; }
    uint8_t         getCurrentInstrumentIndex() const { return _currentInstrument; }
    uint8_t         getNumInstruments()         const { return NUM_INSTRUMENTS; }
    const char*     getInstrumentName(uint8_t i) const;

    // Parameter & Category state
    uint8_t         getSelectedCategory()       const { return _selectedCategory; }
    uint8_t         getSelectedParamIndex()     const { return _selectedParam; }
    bool            isEditingParam()            const { return _navState == NavState::PARAM_EDIT; }
    uint8_t         getFilteredParamCount()     const { return _filteredParamCount; }
    uint8_t         getFilteredParamRealIndex(uint8_t filteredIdx) const;

    // Engine menu state
    uint8_t         getMenuSelection()          const { return _menuSelection; }

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

    // Category & Parameter selection
    uint8_t _selectedCategory   = 0;
    uint8_t _selectedParam      = 0; // Index within filtered list
    uint8_t _paramScrollTop     = 0;
    uint8_t _filteredIndices[MAX_PARAMS];
    uint8_t _filteredParamCount = 0;

    void updateFilteredParams();

    // Engine menu
    uint8_t _menuSelection = 0;

    // State machine handlers
    void handleEngineMenu();
    void handlePatchSelect();
    void handleCategorySelect();
    void handleParamSelect();
    void handleParamEdit();

    void enterState(NavState newState);
};

extern System Sys;
