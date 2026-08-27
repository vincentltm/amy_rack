#include "System.h"
#include "Display.h"
#include "EncoderInput.h"
#include "CVManager.h"
#include "MidiManager.h"

// Include all 5 instrument types
#include "InstrumentDX7.h"
#include "InstrumentJuno.h"
#include "InstrumentAnalog.h"
#include "InstrumentSampler.h"
#include "InstrumentPiano.h"

// Global instance
System Sys;

void System::begin(Display &disp, EncoderInput &enc, CVManager &cv, MidiManager &midi) {
    _display = &disp;
    _encoder = &enc;
    _cv      = &cv;
    _midi    = &midi;

    initInstruments();
    switchInstrument(DEFAULT_INSTRUMENT);
    enterState(NavState::PATCH_SELECT);
}

void System::initInstruments() {
    _instruments[INST_DX7]     = new InstrumentDX7();
    _instruments[INST_JUNO]    = new InstrumentJuno();
    _instruments[INST_ANALOG]  = new InstrumentAnalog();
    _instruments[INST_SAMPLER] = new InstrumentSampler();
    _instruments[INST_PIANO]   = new InstrumentPiano();

    for (uint8_t i = 0; i < NUM_INSTRUMENTS; i++) {
        _instruments[i]->init();
    }
}

void System::update() {
    switch (_navState) {
        case NavState::ENGINE_MENU:  handleEngineMenu();  break;
        case NavState::PATCH_SELECT: handlePatchSelect(); break;
        case NavState::PARAM_SELECT: handleParamSelect(); break;
        case NavState::PARAM_EDIT:   handleParamEdit();   break;
    }

    Instrument *inst = getActiveInstrument();
    if (inst) inst->update();
}

// -----------------------------------------------------------------------------
// Level 0: ENGINE_MENU (Select synth engine)
//   - Turn: scroll engines
//   - Click: select engine -> go DOWN to Level 1 (PATCH_SELECT)
//   - Long press: cancel -> go DOWN to Level 1 (PATCH_SELECT)
// -----------------------------------------------------------------------------
void System::handleEngineMenu() {
    // Click -> Select engine and go DOWN to Level 1
    if (_encoder->wasPressed()) {
        switchInstrument(_menuSelection);
        enterState(NavState::PATCH_SELECT);
        return;
    }

    // Long press -> Cancel and return to Level 1
    if (_encoder->wasLongPressed()) {
        enterState(NavState::PATCH_SELECT);
        return;
    }

    // Turn -> Scroll through engine choices
    int delta = _encoder->getDelta();
    if (delta != 0) {
        int newSel = (int)_menuSelection + delta;
        newSel = constrain(newSel, 0, NUM_INSTRUMENTS - 1);
        _menuSelection = (uint8_t)newSel;
    }
}

// -----------------------------------------------------------------------------
// Level 1: PATCH_SELECT (Browse presets on main engine screen)
//   - Turn: scroll presets (if available)
//   - Click: go DOWN to Level 2 (PARAM_SELECT / settings)
//   - Long press: go UP to Level 0 (ENGINE_MENU)
// -----------------------------------------------------------------------------
void System::handlePatchSelect() {
    // Long press -> Go UP to Level 0 (ENGINE_MENU)
    if (_encoder->wasLongPressed()) {
        _menuSelection = _currentInstrument;
        enterState(NavState::ENGINE_MENU);
        return;
    }

    // Click -> Go DOWN to Level 2 (PARAM_SELECT)
    if (_encoder->wasPressed()) {
        _selectedParam = 0;
        _paramScrollTop = 0;
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Turn -> Cycle through preset patches
    int delta = _encoder->getDelta();
    if (delta != 0) {
        Instrument *inst = getActiveInstrument();
        if (inst && inst->getPatchCount() > 0) {
            int newPatch = inst->getCurrentPatch() + delta;
            newPatch = constrain(newPatch, 0, inst->getPatchCount() - 1);
            inst->setPatch(newPatch);
            inst->needsUIRedraw = true;
        } else if (inst && inst->getParamCount() > 0) {
            // If no presets, turning directly scrolls to settings
            _selectedParam = 0;
            enterState(NavState::PARAM_SELECT);
        }
    }
}

// -----------------------------------------------------------------------------
// Level 2: PARAM_SELECT (Browse settings list)
//   - Turn: scroll settings
//   - Click: go DOWN to Level 3 (PARAM_EDIT highlighted setting)
//   - Long press: go UP to Level 1 (PATCH_SELECT)
// -----------------------------------------------------------------------------
void System::handleParamSelect() {
    Instrument *inst = getActiveInstrument();
    if (!inst) return;

    uint8_t paramCount = inst->getParamCount();
    if (paramCount == 0) {
        enterState(NavState::PATCH_SELECT);
        return;
    }

    // Long press -> Go UP to Level 1 (PATCH_SELECT)
    if (_encoder->wasLongPressed()) {
        enterState(NavState::PATCH_SELECT);
        return;
    }

    // Click -> Go DOWN to Level 3 (PARAM_EDIT)
    if (_encoder->wasPressed()) {
        enterState(NavState::PARAM_EDIT);
        return;
    }

    // Turn -> Scroll through parameters
    int delta = _encoder->getDelta();
    if (delta != 0) {
        int newSel = (int)_selectedParam + delta;
        newSel = constrain(newSel, 0, (int)paramCount - 1);
        _selectedParam = (uint8_t)newSel;

        uint8_t visibleRows = PARAM_LIST_H / PARAM_LIST_ROW_H;
        if (_selectedParam < _paramScrollTop) {
            _paramScrollTop = _selectedParam;
        } else if (_selectedParam >= _paramScrollTop + visibleRows) {
            _paramScrollTop = _selectedParam - visibleRows + 1;
        }
    }
}

// -----------------------------------------------------------------------------
// Level 3: PARAM_EDIT (Adjust setting value)
//   - Turn: tweak value in real time
//   - Click: confirm and go UP to Level 2 (PARAM_SELECT)
//   - Long press: confirm and go UP to Level 2 (PARAM_SELECT)
// -----------------------------------------------------------------------------
void System::handleParamEdit() {
    Instrument *inst = getActiveInstrument();
    if (!inst) return;

    const ParamDescriptor *params = inst->getParams();
    uint8_t paramCount = inst->getParamCount();

    if (_selectedParam >= paramCount) {
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Click or Long press -> Confirm & go UP to Level 2
    if (_encoder->wasPressed() || _encoder->wasLongPressed()) {
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Turn -> Adjust parameter value
    int delta = _encoder->getDelta();
    if (delta != 0) {
        bool accel = (abs(delta) >= ENCODER_ACCEL_THRESHOLD);
        params[_selectedParam].adjust(delta, accel);
        inst->onParamChanged(_selectedParam);
        inst->needsUIRedraw = true;
    }
}

void System::enterState(NavState newState) {
    _navState = newState;
    Instrument *inst = getActiveInstrument();
    if (inst) inst->needsUIRedraw = true;
}

void System::switchInstrument(uint8_t index) {
    if (index >= NUM_INSTRUMENTS) return;

    if (_instruments[_currentInstrument]) {
        _instruments[_currentInstrument]->stop();
    }

    _currentInstrument = index;

    if (_instruments[_currentInstrument]) {
        _instruments[_currentInstrument]->start();
        _instruments[_currentInstrument]->needsUIRedraw = true;
    }

    _selectedParam = 0;
    _paramScrollTop = 0;
}

const char* System::getInstrumentName(uint8_t i) const {
    if (i < NUM_INSTRUMENTS && _instruments[i]) {
        return _instruments[i]->getShortName();
    }
    return "---";
}
