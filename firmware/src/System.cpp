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
    _activeTab = TabId::TAB_MAIN;
    enterState(NavState::TAB_SELECT);
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
        case NavState::TAB_SELECT:   handleTabSelect();   break;
        case NavState::PATCH_BROWSE: handlePatchBrowse(); break;
        case NavState::PARAM_SELECT: handleParamSelect(); break;
        case NavState::PARAM_EDIT:   handleParamEdit();   break;
    }

    Instrument *inst = getActiveInstrument();
    if (inst) inst->update();
}

void System::updateTabParams() {
    Instrument *inst = getActiveInstrument();
    _tabParamCount = 0;
    if (!inst) return;

    const ParamDescriptor *params = inst->getParams();
    uint8_t totalParams = inst->getParamCount();

    for (uint8_t i = 0; i < totalParams; i++) {
        if (params[i].tab == _activeTab) {
            _tabIndices[_tabParamCount++] = i;
        }
    }
}

uint8_t System::getTabParamRealIndex(uint8_t idx) const {
    if (idx < _tabParamCount) {
        return _tabIndices[idx];
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Level 0: ENGINE_MENU (Select synth engine)
// -----------------------------------------------------------------------------
void System::handleEngineMenu() {
    if (_encoder->wasPressed()) {
        switchInstrument(_menuSelection);
        enterState(NavState::TAB_SELECT);
        return;
    }

    if (_encoder->wasLongPressed()) {
        enterState(NavState::TAB_SELECT);
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
// Level 1: TAB_SELECT (Horizontal Tab Bar: MAIN | SYNTH | ENV | FX)
// -----------------------------------------------------------------------------
void System::handleTabSelect() {
    // Long press -> Open Engine Menu
    if (_encoder->wasLongPressed()) {
        _menuSelection = _currentInstrument;
        enterState(NavState::ENGINE_MENU);
        return;
    }

    // Click -> Dive into active tab
    if (_encoder->wasPressed()) {
        if (_activeTab == TabId::TAB_MAIN) {
            Instrument *inst = getActiveInstrument();
            if (inst && inst->getPatchCount() > 0) {
                enterState(NavState::PATCH_BROWSE);
            } else {
                // If no patches, jump directly to SYNTH tab
                _activeTab = TabId::TAB_SYNTH;
                updateTabParams();
                _selectedParam = 0;
                enterState(NavState::PARAM_SELECT);
            }
        } else {
            // Dive into parameter list of current tab
            updateTabParams();
            _selectedParam = 0;
            _paramScrollTop = 0;
            if (_tabParamCount > 0) {
                enterState(NavState::PARAM_SELECT);
            }
        }
        return;
    }

    // Turn -> Switch Tab horizontally
    int delta = _encoder->getDelta();
    if (delta != 0) {
        int newTab = (int)_activeTab + delta;
        newTab = constrain(newTab, 0, (int)TabId::TAB_COUNT - 1);
        _activeTab = (TabId)newTab;
        updateTabParams();
        _selectedParam = 0;
        Instrument *inst = getActiveInstrument();
        if (inst) inst->needsUIRedraw = true;
    }
}

// -----------------------------------------------------------------------------
// Level 2: PATCH_BROWSE (When on MAIN tab and clicked)
// -----------------------------------------------------------------------------
void System::handlePatchBrowse() {
    // Click or Long press -> Confirm patch & go back to TAB_SELECT
    if (_encoder->wasPressed() || _encoder->wasLongPressed()) {
        enterState(NavState::TAB_SELECT);
        return;
    }

    // Turn -> Scroll preset patches
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

// -----------------------------------------------------------------------------
// Level 3: PARAM_SELECT (Scroll parameters inside active tab)
// -----------------------------------------------------------------------------
void System::handleParamSelect() {
    Instrument *inst = getActiveInstrument();
    if (!inst || _tabParamCount == 0) {
        enterState(NavState::TAB_SELECT);
        return;
    }

    // Long press -> Go back UP to Tab Bar
    if (_encoder->wasLongPressed()) {
        enterState(NavState::TAB_SELECT);
        return;
    }

    // Click -> Enter PARAM_EDIT mode for highlighted setting
    if (_encoder->wasPressed()) {
        enterState(NavState::PARAM_EDIT);
        return;
    }

    // Turn -> Scroll parameters in this tab
    int delta = _encoder->getDelta();
    if (delta != 0) {
        int newSel = (int)_selectedParam + delta;
        newSel = constrain(newSel, 0, (int)_tabParamCount - 1);
        _selectedParam = (uint8_t)newSel;

        uint8_t visibleRows = 6;
        if (_selectedParam < _paramScrollTop) {
            _paramScrollTop = _selectedParam;
        } else if (_selectedParam >= _paramScrollTop + visibleRows) {
            _paramScrollTop = _selectedParam - visibleRows + 1;
        }
    }
}

// -----------------------------------------------------------------------------
// Level 4: PARAM_EDIT (Adjust setting value in real-time)
// -----------------------------------------------------------------------------
void System::handleParamEdit() {
    Instrument *inst = getActiveInstrument();
    if (!inst) return;

    if (_selectedParam >= _tabParamCount) {
        enterState(NavState::PARAM_SELECT);
        return;
    }

    uint8_t realIdx = _tabIndices[_selectedParam];
    const ParamDescriptor *params = inst->getParams();

    // Click or Long press -> Confirm & return to PARAM_SELECT
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
    updateTabParams();
}

const char* System::getInstrumentName(uint8_t i) const {
    if (i < NUM_INSTRUMENTS && _instruments[i]) {
        return _instruments[i]->getShortName();
    }
    return "---";
}
