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

static const char* engineNames[NUM_INSTRUMENTS] = {
    "DX7",
    "JUNO-106",
    "ANALOG",
    "SAMPLER",
    "PIANO"
};

static const char* voiceModeNames[2] = {
    "Poly 6V",
    "Mono Leg"
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
    Instrument *inst = getActiveInstrument();

    // TAB_MAIN
    _masterParams[0] = PARAM_ENUM("Engine",     NUM_INSTRUMENTS - 1, &_param_engine,  engineNames,      TAB_MAIN);
    _masterParams[1] = PARAM_INT("Patch",       "", 0, 127,           &_param_patch,                    TAB_MAIN);
    _masterParams[2] = PARAM_ENUM("Voice Mode", 1,                   inst ? &(inst->params.voice_mode) : nullptr, voiceModeNames, TAB_MAIN);
    _masterParams[3] = PARAM_MS("Glide Time",   0.0f, 500.0f, 10.0f,  inst ? &(inst->params.glide_ms) : nullptr, TAB_MAIN);
    _masterParams[4] = PARAM_PCT("Volume",      0.0f, 100.0f, 5.0f,   &_param_volume,                   TAB_MAIN);

    // TAB_MIDI
    _masterParams[5] = PARAM_ENUM("MIDI Ch",    16,                  &_param_midi_ch, midiChannelNames, TAB_MIDI);
    _masterParams[6] = PARAM_ENUM("CV1 In",     3,                   &_param_cv1,     cv1Names,         TAB_MIDI);
    _masterParams[7] = PARAM_ENUM("CV2 In",     3,                   &_param_cv2,     cv2Names,         TAB_MIDI);

    _masterParamCount = 8;
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

    if (_activeTab == TabId::TAB_MAIN || _activeTab == TabId::TAB_MIDI) {
        if (inst) {
            _masterParams[1].maxVal = (float)std::max(0, inst->getPatchCount() - 1);
            _param_patch = (float)inst->getCurrentPatch();
            _masterParams[2].valuePtr = &(inst->params.voice_mode);
            _masterParams[3].valuePtr = &(inst->params.glide_ms);
        }
        _param_engine = (float)_currentInstrument;

        for (uint8_t i = 0; i < _masterParamCount; i++) {
            if (_masterParams[i].tab == _activeTab) {
                _tabIndices[_tabParamCount++] = i;
            }
        }
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
    if (idx >= _tabParamCount) return nullptr;
    uint8_t realIdx = _tabIndices[idx];

    if (_activeTab == TabId::TAB_MAIN || _activeTab == TabId::TAB_MIDI) {
        if (realIdx < _masterParamCount) return &_masterParams[realIdx];
        return nullptr;
    }
    Instrument *inst = getActiveInstrument();
    if (inst) {
        return &(inst->getParams()[realIdx]);
    }
    return nullptr;
}

uint8_t System::getTabParamRealIndex(uint8_t idx) const {
    if (idx < _tabParamCount) return _tabIndices[idx];
    return 0;
}

// -----------------------------------------------------------------------------
// Level 0: TAB_SELECT (Horizontal Tab Bar: MAIN | SYNTH | ENV | FX | MIDI)
// -----------------------------------------------------------------------------
void System::handleTabSelect() {
    int delta = _encoder->getDelta();
    if (delta != 0) {
        int nextTab = (int)_activeTab + delta;
        if (nextTab < 0) nextTab = 0;
        if (nextTab >= TAB_COUNT) nextTab = TAB_COUNT - 1;
        
        if ((TabId)nextTab != _activeTab) {
            _activeTab = (TabId)nextTab;
            _selectedParam = 0;
            updateTabParams();
            if (getActiveInstrument()) getActiveInstrument()->needsUIRedraw = true;
        }
    }

    if (_encoder->wasPressed()) {
        if (_tabParamCount > 0) {
            _selectedParam = 0;
            enterState(NavState::PARAM_SELECT);
        }
    }
}

// -----------------------------------------------------------------------------
// Level 1: PARAM_SELECT (Scroll vertical parameter list)
// -----------------------------------------------------------------------------
void System::handleParamSelect() {
    int delta = _encoder->getDelta();
    if (delta != 0 && _tabParamCount > 0) {
        int next = (int)_selectedParam + delta;
        if (next < 0) next = 0;
        if (next >= _tabParamCount) next = _tabParamCount - 1;
        _selectedParam = (uint8_t)next;
    }

    if (_encoder->wasPressed()) {
        if (_tabParamCount > 0) {
            enterState(NavState::PARAM_EDIT);
        }
    }

    if (_encoder->wasLongPressed()) {
        enterState(NavState::TAB_SELECT);
    }
}

// -----------------------------------------------------------------------------
// Level 2: PARAM_EDIT (Turn encoder to live adjust value)
// -----------------------------------------------------------------------------
void System::handleParamEdit() {
    int delta = _encoder->getDelta();
    if (delta != 0) {
        if (_activeTab == TabId::TAB_MAIN || _activeTab == TabId::TAB_MIDI) {
            uint8_t realIdx = _tabIndices[_selectedParam];
            if (realIdx < _masterParamCount) {
                _masterParams[realIdx].adjust(delta, false);
                onMasterParamChanged(realIdx);
            }
        } else {
            Instrument *inst = getActiveInstrument();
            if (inst) {
                uint8_t realIdx = _tabIndices[_selectedParam];
                const ParamDescriptor *desc = &(inst->getParams()[realIdx]);
                if (desc) {
                    desc->adjust(delta, false);
                    inst->onParamChanged(realIdx);
                }
            }
        }
    }

    if (_encoder->wasPressed() || _encoder->wasLongPressed()) {
        enterState(NavState::PARAM_SELECT);
    }
}

void System::onMasterParamChanged(uint8_t idx) {
    if (idx == 0) { // Engine
        uint8_t newInst = (uint8_t)roundf(_param_engine);
        if (newInst < NUM_INSTRUMENTS && newInst != _currentInstrument) {
            switchInstrument(newInst);
        }
    } else if (idx == 1) { // Patch
        Instrument *inst = getActiveInstrument();
        if (inst) {
            inst->setPatch((int)roundf(_param_patch));
        }
    } else if (idx == 2 || idx == 3) { // Voice Mode or Glide Time
        Instrument *inst = getActiveInstrument();
        if (inst) {
            inst->applyVoiceMode();
        }
    } else if (idx == 4) { // Volume
        amy_event e = amy_default_event();
        e.volume = _param_volume / 100.0f;
        amy_add_event(&e);
    } else if (idx == 5) { // MIDI Ch
        if (_midi) {
            _midi->setChannel((uint8_t)roundf(_param_midi_ch));
        }
    }
}

void System::switchInstrument(uint8_t index) {
    if (index >= NUM_INSTRUMENTS) return;

    if (_instruments[_currentInstrument]) {
        _instruments[_currentInstrument]->stop();
    }

    _currentInstrument = index;
    _param_engine = (float)index;

    // Reset AMY oscillators synchronously to guarantee a clean slate
    amy_reset_oscs();

    if (_instruments[_currentInstrument]) {
        _instruments[_currentInstrument]->start();
    }

    updateTabParams();
    _selectedParam = 0;
    if (_instruments[_currentInstrument]) {
        _instruments[_currentInstrument]->needsUIRedraw = true;
    }
}

const char* System::getInstrumentName(uint8_t i) const {
    if (i < NUM_INSTRUMENTS && _instruments[i]) {
        return _instruments[i]->getName();
    }
    return "";
}

void System::enterState(NavState newState) {
    _navState = newState;
    if (_instruments[_currentInstrument]) {
        _instruments[_currentInstrument]->needsUIRedraw = true;
    }
}
