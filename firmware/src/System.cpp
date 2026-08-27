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
        case NavState::ENGINE_MENU:     handleEngineMenu();     break;
        case NavState::PATCH_SELECT:    handlePatchSelect();    break;
        case NavState::CATEGORY_SELECT: handleCategorySelect(); break;
        case NavState::PARAM_SELECT:    handleParamSelect();    break;
        case NavState::PARAM_EDIT:      handleParamEdit();      break;
    }

    Instrument *inst = getActiveInstrument();
    if (inst) inst->update();
}

void System::updateFilteredParams() {
    Instrument *inst = getActiveInstrument();
    _filteredParamCount = 0;
    if (!inst) return;

    const ParamDescriptor *params = inst->getParams();
    uint8_t totalParams = inst->getParamCount();

    for (uint8_t i = 0; i < totalParams; i++) {
        if (params[i].category == (ParamCategory)_selectedCategory) {
            _filteredIndices[_filteredParamCount++] = i;
        }
    }
}

uint8_t System::getFilteredParamRealIndex(uint8_t filteredIdx) const {
    if (filteredIdx < _filteredParamCount) {
        return _filteredIndices[filteredIdx];
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Level 0: ENGINE_MENU (Select synth engine)
// -----------------------------------------------------------------------------
void System::handleEngineMenu() {
    if (_encoder->wasPressed()) {
        switchInstrument(_menuSelection);
        enterState(NavState::PATCH_SELECT);
        return;
    }

    if (_encoder->wasLongPressed()) {
        enterState(NavState::PATCH_SELECT);
        return;
    }

    int delta = _encoder->getDelta();
    if (delta != 0) {
        int newSel = (int)_menuSelection + delta;
        newSel = constrain(newSel, 0, NUM_INSTRUMENTS - 1);
        _menuSelection = (uint8_t)newSel;
    }
}

// -----------------------------------------------------------------------------
// Level 1: PATCH_SELECT (Browse presets on main engine screen)
// -----------------------------------------------------------------------------
void System::handlePatchSelect() {
    // Long press -> Go UP to Level 0 (ENGINE_MENU)
    if (_encoder->wasLongPressed()) {
        _menuSelection = _currentInstrument;
        enterState(NavState::ENGINE_MENU);
        return;
    }

    // Click -> Go DOWN to Level 2 (CATEGORY_SELECT)
    if (_encoder->wasPressed()) {
        enterState(NavState::CATEGORY_SELECT);
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
        } else {
            enterState(NavState::CATEGORY_SELECT);
        }
    }
}

// -----------------------------------------------------------------------------
// Level 2: CATEGORY_SELECT (Choose Settings Category: SYNTH, FILTER/ENV, FX)
// -----------------------------------------------------------------------------
void System::handleCategorySelect() {
    // Long press -> Go UP to Level 1 (PATCH_SELECT)
    if (_encoder->wasLongPressed()) {
        enterState(NavState::PATCH_SELECT);
        return;
    }

    // Click -> Select category and go DOWN to Level 3 (PARAM_SELECT)
    if (_encoder->wasPressed()) {
        _selectedParam = 0;
        _paramScrollTop = 0;
        updateFilteredParams();
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Turn -> Scroll through categories
    int delta = _encoder->getDelta();
    if (delta != 0) {
        int newCat = (int)_selectedCategory + delta;
        newCat = constrain(newCat, 0, (int)CAT_COUNT - 1);
        _selectedCategory = (uint8_t)newCat;
    }
}

// -----------------------------------------------------------------------------
// Level 3: PARAM_SELECT (Browse settings list within chosen category)
// -----------------------------------------------------------------------------
void System::handleParamSelect() {
    Instrument *inst = getActiveInstrument();
    if (!inst) return;

    if (_filteredParamCount == 0) {
        enterState(NavState::CATEGORY_SELECT);
        return;
    }

    // Long press -> Go UP to Level 2 (CATEGORY_SELECT)
    if (_encoder->wasLongPressed()) {
        enterState(NavState::CATEGORY_SELECT);
        return;
    }

    // Click -> Go DOWN to Level 4 (PARAM_EDIT)
    if (_encoder->wasPressed()) {
        enterState(NavState::PARAM_EDIT);
        return;
    }

    // Turn -> Scroll through parameter list
    int delta = _encoder->getDelta();
    if (delta != 0) {
        int newSel = (int)_selectedParam + delta;
        newSel = constrain(newSel, 0, (int)_filteredParamCount - 1);
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
// Level 4: PARAM_EDIT (Adjust setting value)
// -----------------------------------------------------------------------------
void System::handleParamEdit() {
    Instrument *inst = getActiveInstrument();
    if (!inst) return;

    if (_selectedParam >= _filteredParamCount) {
        enterState(NavState::PARAM_SELECT);
        return;
    }

    uint8_t realIdx = _filteredIndices[_selectedParam];
    const ParamDescriptor *params = inst->getParams();

    // Click or Long press -> Confirm & go UP to Level 3
    if (_encoder->wasPressed() || _encoder->wasLongPressed()) {
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Turn -> Adjust parameter value
    int delta = _encoder->getDelta();
    if (delta != 0) {
        bool accel = (abs(delta) >= ENCODER_ACCEL_THRESHOLD);
        params[realIdx].adjust(delta, accel);
        inst->onParamChanged(realIdx);
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
    updateFilteredParams();
}

const char* System::getInstrumentName(uint8_t i) const {
    if (i < NUM_INSTRUMENTS && _instruments[i]) {
        return _instruments[i]->getShortName();
    }
    return "---";
}
