#pragma once
// =============================================================================
// System.h — Navigation state machine and instrument management
// =============================================================================
// Replaces Spark's SystemClass.  Manages the encoder-driven UI navigation,
// instrument lifecycle, and ties together Display, Encoder, CV, and MIDI.
// =============================================================================

#include <Arduino.h>
#include "Config.h"
#include "Instrument.h"

// Forward declarations — these are included in the .cpp
class Display;
class EncoderInput;
class CVManager;
class MidiManager;

// Navigation states (encoder-driven state machine)
enum class NavState : uint8_t {
    MAIN_SCREEN,        // Default: shows instrument UI + params (read-only)
    PARAM_SELECT,       // Encoder scrolls parameter list
    PARAM_EDIT,         // Encoder changes selected parameter value
    INSTRUMENT_MENU,    // Full-screen instrument picker
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

    // Parameter list state (for Display to render)
    uint8_t         getSelectedParamIndex() const { return _selectedParam; }
    bool            isEditingParam()        const { return _navState == NavState::PARAM_EDIT; }

    // Instrument menu state
    uint8_t         getMenuSelection()      const { return _menuSelection; }

    // --- External triggers ---
    void switchInstrument(uint8_t index);

private:
    // References to subsystems (set in begin())
    Display      *_display  = nullptr;
    EncoderInput *_encoder  = nullptr;
    CVManager    *_cv       = nullptr;
    MidiManager  *_midi     = nullptr;

    // Navigation state
    NavState _navState = NavState::MAIN_SCREEN;

    // Instrument management
    uint8_t     _currentInstrument = DEFAULT_INSTRUMENT;
    Instrument *_instruments[NUM_INSTRUMENTS];
    void initInstruments();

    // Parameter selection
    uint8_t _selectedParam  = 0;
    uint8_t _paramScrollTop = 0;    // First visible param in the list

    // Instrument menu
    uint8_t _menuSelection = 0;

    // State machine handlers
    void handleMainScreen();
    void handleParamSelect();
    void handleParamEdit();
    void handleInstrumentMenu();

    // Transitions
    void enterState(NavState newState);
};

// Global instance
extern System Sys;
