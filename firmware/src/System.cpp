// =============================================================================
// System.cpp — Navigation state machine and instrument management
// =============================================================================

#include "System.h"
#include "Display.h"
#include "EncoderInput.h"
#include "CVManager.h"
#include "MidiManager.h"

// Include all instrument types
#include "InstrumentDX7.h"
#include "InstrumentJuno.h"
#include "InstrumentAnalog.h"
#include "InstrumentPiano.h"

// Global instance
System Sys;

// =============================================================================
// Initialization
// =============================================================================

void System::begin(Display &disp, EncoderInput &enc, CVManager &cv, MidiManager &midi) {
    _display = &disp;
    _encoder = &enc;
    _cv      = &cv;
    _midi    = &midi;

    initInstruments();
    switchInstrument(DEFAULT_INSTRUMENT);
    enterState(NavState::MAIN_SCREEN);
}

void System::initInstruments() {
    _instruments[INST_DX7]    = new InstrumentDX7();
    _instruments[INST_JUNO]   = new InstrumentJuno();
    _instruments[INST_ANALOG] = new InstrumentAnalog();
    _instruments[INST_PIANO]  = new InstrumentPiano();

    for (uint8_t i = 0; i < NUM_INSTRUMENTS; i++) {
        _instruments[i]->init();
    }
}

// =============================================================================
// Main update loop — called every frame from loop()
// =============================================================================

void System::update() {
    switch (_navState) {
        case NavState::MAIN_SCREEN:     handleMainScreen();     break;
        case NavState::PARAM_SELECT:    handleParamSelect();    break;
        case NavState::PARAM_EDIT:      handleParamEdit();      break;
        case NavState::INSTRUMENT_MENU: handleInstrumentMenu(); break;
    }

    // Let the active instrument do any per-frame work
    Instrument *inst = getActiveInstrument();
    if (inst) inst->update();
}

// =============================================================================
// State machine handlers
// =============================================================================

void System::handleMainScreen() {
    // Long press → open instrument menu
    if (_encoder->wasLongPressed()) {
        _menuSelection = _currentInstrument;
        enterState(NavState::INSTRUMENT_MENU);
        return;
    }

    // Short press → enter parameter selection
    if (_encoder->wasPressed()) {
        _selectedParam = 0;
        _paramScrollTop = 0;
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Encoder turn on main screen → browse patches (if instrument has patches)
    int delta = _encoder->getDelta();
    if (delta != 0) {
        Instrument *inst = getActiveInstrument();
        if (inst && inst->getPatchCount() > 0) {
            int newPatch = inst->getCurrentPatch() + delta;
            newPatch = constrain(newPatch, 0, inst->getPatchCount() - 1);
            inst->setPatch(newPatch);
            inst->needsUIRedraw = true;
        }
    }
}

void System::handleParamSelect() {
    Instrument *inst = getActiveInstrument();
    if (!inst) return;

    uint8_t paramCount = inst->getParamCount();
    if (paramCount == 0) {
        enterState(NavState::MAIN_SCREEN);
        return;
    }

    // Long press → back to main screen
    if (_encoder->wasLongPressed()) {
        enterState(NavState::MAIN_SCREEN);
        return;
    }

    // Short press → enter edit mode for selected param
    if (_encoder->wasPressed()) {
        enterState(NavState::PARAM_EDIT);
        return;
    }

    // Encoder turn → scroll through parameter list
    int delta = _encoder->getDelta();
    if (delta != 0) {
        int newSel = (int)_selectedParam + delta;
        newSel = constrain(newSel, 0, (int)paramCount - 1);
        _selectedParam = (uint8_t)newSel;

        // Keep selected param visible in the scroll window
        uint8_t visibleRows = PARAM_LIST_H / PARAM_LIST_ROW_H;
        if (_selectedParam < _paramScrollTop) {
            _paramScrollTop = _selectedParam;
        } else if (_selectedParam >= _paramScrollTop + visibleRows) {
            _paramScrollTop = _selectedParam - visibleRows + 1;
        }
    }
}

void System::handleParamEdit() {
    Instrument *inst = getActiveInstrument();
    if (!inst) return;

    const ParamDescriptor *params = inst->getParams();
    uint8_t paramCount = inst->getParamCount();

    if (_selectedParam >= paramCount) {
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Long press → cancel edit, back to param select
    if (_encoder->wasLongPressed()) {
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Short press → confirm edit, back to param select
    if (_encoder->wasPressed()) {
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Encoder turn → adjust parameter value
    int delta = _encoder->getDelta();
    if (delta != 0) {
        bool accel = (abs(delta) >= ENCODER_ACCEL_THRESHOLD);
        params[_selectedParam].adjust(delta, accel);
        inst->onParamChanged(_selectedParam);
        inst->needsUIRedraw = true;
    }
}

void System::handleInstrumentMenu() {
    // Short press → select and switch instrument
    if (_encoder->wasPressed()) {
        switchInstrument(_menuSelection);
        enterState(NavState::MAIN_SCREEN);
        return;
    }

    // Long press → cancel, back to main screen
    if (_encoder->wasLongPressed()) {
        enterState(NavState::MAIN_SCREEN);
        return;
    }

    // Encoder turn → scroll through instruments
    int delta = _encoder->getDelta();
    if (delta != 0) {
        int newSel = (int)_menuSelection + delta;
        newSel = constrain(newSel, 0, NUM_INSTRUMENTS - 1);
        _menuSelection = (uint8_t)newSel;
    }
}

// =============================================================================
// State transitions
// =============================================================================

void System::enterState(NavState newState) {
    _navState = newState;
    // Force a display redraw on any state change
    Instrument *inst = getActiveInstrument();
    if (inst) inst->needsUIRedraw = true;
}

// =============================================================================
// Instrument switching
// =============================================================================

void System::switchInstrument(uint8_t index) {
    if (index >= NUM_INSTRUMENTS) return;

    // Stop current instrument
    if (_instruments[_currentInstrument]) {
        _instruments[_currentInstrument]->stop();
    }

    _currentInstrument = index;

    // Start new instrument
    if (_instruments[_currentInstrument]) {
        _instruments[_currentInstrument]->start();
        _instruments[_currentInstrument]->needsUIRedraw = true;
    }

    // Reset parameter selection
    _selectedParam = 0;
    _paramScrollTop = 0;
}

const char* System::getInstrumentName(uint8_t i) const {
    if (i < NUM_INSTRUMENTS && _instruments[i]) {
        return _instruments[i]->getShortName();
    }
    return "---";
}
