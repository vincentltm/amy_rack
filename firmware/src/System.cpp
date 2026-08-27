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

// String arrays for enums
static const char* engineNames[NUM_INSTRUMENTS] = {
    "DX7",
    "JUNO-106",
    "ANALOG",
    "SAMPLER",
    "PIANO"
};

static const char* midiChannelNames[17] = {
    "Ch 1", "Ch 2", "Ch 3", "Ch 4", "Ch 5", "Ch 6", "Ch 7", "Ch 8",
    "Ch 9", "Ch 10", "Ch 11", "Ch 12", "Ch 13", "Ch 14", "Ch 15", "Ch 16", "Omni"
};

static const char* cv1Names[4] = {
    "V/Oct", "Cutoff", "Volume", "Off"
};

static const char* cv2Names[4] = {
    "Gate", "ModWhl", "Reson", "Off"
};

System Sys;

void System::begin(Display &disp, EncoderInput &enc, CVManager &cv, MidiManager &midi) {
    _display = &disp;
    _encoder = &enc;
    _cv      = &cv;
    _midi    = &midi;

    initInstruments();
    initMasterParams();
    switchInstrument(DEFAULT_INSTRUMENT);
    _activeTab = TabId::TAB_MAIN;
    updateTabParams();
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

void System::initMasterParams() {
    _masterParams[0] = PARAM_ENUM("Engine",  NUM_INSTRUMENTS - 1, &_param_engine,  engineNames,      TAB_MAIN);
    _masterParams[1] = PARAM_INT("Patch",   "", 0, 127,           &_param_patch,   TAB_MAIN);
    _masterParams[2] = PARAM_ENUM("MIDI Ch", 16,                  &_param_midi_ch, midiChannelNames, TAB_MAIN);
    _masterParams[3] = PARAM_ENUM("CV1 In",  3,                   &_param_cv1,     cv1Names,         TAB_MAIN);
    _masterParams[4] = PARAM_ENUM("CV2 In",  3,                   &_param_cv2,     cv2Names,         TAB_MAIN);
    _masterParamCount = 5;
}

void System::update() {
    switch (_navState) {
        case NavState::TAB_SELECT:   handleTabSelect();   break;
        case NavState::PARAM_SELECT: handleParamSelect(); break;
        case NavState::PARAM_EDIT:   handleParamEdit();   break;
    }

    Instrument *inst = getActiveInstrument();
    if (inst) inst->update();
}

void System::updateTabParams() {
    Instrument *inst = getActiveInstrument();
    _tabParamCount = 0;

    if (_activeTab == TabId::TAB_MAIN) {
        if (inst) {
            _masterParams[1].maxVal = (float)std::max(0, inst->getPatchCount() - 1);
            _param_patch = (float)inst->getCurrentPatch();
        }
        _param_engine = (float)_currentInstrument;
        _tabParamCount = _masterParamCount;
        return;
    }

    if (!inst) return;

    const ParamDescriptor *params = inst->getParams();
    uint8_t totalParams = inst->getParamCount();

    for (uint8_t i = 0; i < totalParams; i++) {
        if (params[i].tab == _activeTab) {
            _tabIndices[_tabParamCount++] = i;
        }
    }
}

const ParamDescriptor* System::getTabParamDescriptor(uint8_t idx) const {
    if (_activeTab == TabId::TAB_MAIN) {
        if (idx < _masterParamCount) return &_masterParams[idx];
        return nullptr;
    }
    Instrument *inst = getActiveInstrument();
    if (inst && idx < _tabParamCount) {
        uint8_t realIdx = _tabIndices[idx];
        return &(inst->getParams()[realIdx]);
    }
    return nullptr;
}

uint8_t System::getTabParamRealIndex(uint8_t idx) const {
    if (_activeTab == TabId::TAB_MAIN) return idx;
    if (idx < _tabParamCount) return _tabIndices[idx];
    return 0;
}

// -----------------------------------------------------------------------------
// Level 0: TAB_SELECT (Horizontal Tab Bar: MAIN | SYNTH | ENV | FX)
// -----------------------------------------------------------------------------
void System::handleTabSelect() {
    // Click -> Dive into active tab parameter list
    if (_encoder->wasPressed()) {
        updateTabParams();
        _selectedParam = 0;
        _paramScrollTop = 0;
        if (_tabParamCount > 0) {
            enterState(NavState::PARAM_SELECT);
        }
        return;
    }

    // Long press on Tab Bar -> Loop back or jump to MAIN tab
    if (_encoder->wasLongPressed()) {
        _activeTab = TabId::TAB_MAIN;
        updateTabParams();
        _selectedParam = 0;
        enterState(NavState::PARAM_SELECT);
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
// Level 1: PARAM_SELECT (Scroll parameters inside active tab)
// -----------------------------------------------------------------------------
void System::handleParamSelect() {
    if (_tabParamCount == 0) {
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

        uint8_t visibleRows = 5;
        if (_selectedParam < _paramScrollTop) {
            _paramScrollTop = _selectedParam;
        } else if (_selectedParam >= _paramScrollTop + visibleRows) {
            _paramScrollTop = _selectedParam - visibleRows + 1;
        }
    }
}

// -----------------------------------------------------------------------------
// Level 2: PARAM_EDIT (Adjust setting value in real-time)
// -----------------------------------------------------------------------------
void System::handleParamEdit() {
    if (_selectedParam >= _tabParamCount) {
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Click or Long press -> Confirm & return to PARAM_SELECT
    if (_encoder->wasPressed() || _encoder->wasLongPressed()) {
        enterState(NavState::PARAM_SELECT);
        return;
    }

    // Turn -> Adjust parameter value
    int delta = _encoder->getDelta();
    if (delta != 0) {
        bool accel = (abs(delta) >= ENCODER_ACCEL_THRESHOLD);

        if (_activeTab == TabId::TAB_MAIN) {
            _masterParams[_selectedParam].adjust(delta, accel);
            onMasterParamChanged(_selectedParam);
        } else {
            Instrument *inst = getActiveInstrument();
            if (inst) {
                uint8_t realIdx = _tabIndices[_selectedParam];
                inst->getParams()[realIdx].adjust(delta, accel);
                inst->onParamChanged(realIdx);
                inst->needsUIRedraw = true;
            }
        }
    }
}

void System::onMasterParamChanged(uint8_t idx) {
    if (idx == 0) { // Engine switch
        uint8_t newEng = (uint8_t)_param_engine;
        if (newEng != _currentInstrument) {
            switchInstrument(newEng);
        }
    } else if (idx == 1) { // Patch switch
        Instrument *inst = getActiveInstrument();
        if (inst && inst->getPatchCount() > 0) {
            inst->setPatch((int)_param_patch);
            inst->needsUIRedraw = true;
        }
    } else if (idx == 2) { // MIDI channel
        if (_midi) {
            _midi->setChannel((uint8_t)_param_midi_ch);
        }
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

    // Reset AMY synth channels completely so no leftover voice chaining interferes
    amy_event e = amy_default_event();
    e.reset_osc = RESET_ALL_OSCS;
    amy_add_event(&e);

    _currentInstrument = index;
    _param_engine = (float)_currentInstrument;

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
