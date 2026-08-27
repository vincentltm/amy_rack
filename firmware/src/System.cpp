#include "System.h"
#include "Display.h"
#include "EncoderInput.h"
#include "CVManager.h"
#include "MidiManager.h"

// Include all 5 melodic synth engines
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

static const char* drumKitNames[9] = {
    "TR-808",
    "TR-909",
    "Linn 9000",
    "MR-12",
    "Tokyo Synth",
    "Power Kit",
    "Percussion",
    "808 Electro",
    "808 Sub Boom"
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
    updateDrumEngine();
    updateAudioIn();

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
    // --- TAB_MAIN ---
    _masterParams[0] = PARAM_ENUM("Synth", NUM_INSTRUMENTS, &_param_engine, engineNames, TAB_MAIN);
    _masterParams[1] = PARAM_INT("Patch", "", 0, 127, &_param_patch, TAB_MAIN);
    _masterParams[2] = PARAM_ENUM("Drums", 9, &_param_drum_kit, drumKitNames, TAB_MAIN);
    _masterParams[3] = PARAM_PCT("Synth Vol", 0.0f, 100.0f, 1.0f, &_param_synth_vol, TAB_MAIN);
    _masterParams[4] = PARAM_PCT("Drum Vol", 0.0f, 100.0f, 1.0f, &_param_drum_vol, TAB_MAIN);
    _masterParams[5] = PARAM_PCT("Audio In", 0.0f, 100.0f, 1.0f, &_param_audio_in_vol, TAB_MAIN);
    _masterParams[6] = PARAM_ENUM("Voice Mode", 2, &_param_voice_mode, voiceModeNames, TAB_MAIN);
    _masterParams[7] = PARAM_FLOAT("Glide", "ms", 0.0f, 500.0f, 5.0f, &_param_glide, TAB_MAIN);

    // --- TAB_DRUM ---
    _masterParams[8]  = PARAM_ENUM("Drums", 9, &_param_drum_kit, drumKitNames, TAB_DRUM);
    _masterParams[9]  = PARAM_PCT("Drum Vol", 0.0f, 100.0f, 1.0f, &_param_drum_vol, TAB_DRUM);
    _masterParams[10] = PARAM_FLOAT("Kick Tune", "st", -12.0f, 12.0f, 1.0f, &_param_kick_tune, TAB_DRUM);
    _masterParams[11] = PARAM_FLOAT("Snare Tune", "st", -12.0f, 12.0f, 1.0f, &_param_snare_tune, TAB_DRUM);
    _masterParams[12] = PARAM_FLOAT("Tom Tune", "st", -12.0f, 12.0f, 1.0f, &_param_tom_tune, TAB_DRUM);
    _masterParams[13] = PARAM_FLOAT("Drive", "x", 0.5f, 5.0f, 0.1f, &_param_drum_drive, TAB_DRUM);

    // --- TAB_MIDI ---
    _masterParams[14] = PARAM_ENUM("Synth MIDI", 17, &_param_synth_midi_ch, midiChannelNames, TAB_MIDI);
    _masterParams[15] = PARAM_ENUM("Drum MIDI", 17, &_param_drum_midi_ch, midiChannelNames, TAB_MIDI);
    _masterParams[16] = PARAM_ENUM("CV 1 Out", 4, &_param_cv1, cv1Names, TAB_MIDI);
    _masterParams[17] = PARAM_ENUM("CV 2 Out", 4, &_param_cv2, cv2Names, TAB_MIDI);

    _masterParamCount = 18;
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

    if (inst) {
        _masterParams[1].maxVal = (float)std::max(0, inst->getPatchCount() - 1);
        _param_patch = (float)inst->getCurrentPatch();
    }
    _param_engine = (float)_currentInstrument;

    if (_activeTab == TabId::TAB_MAIN || _activeTab == TabId::TAB_DRUM || _activeTab == TabId::TAB_MIDI) {
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

    if (_activeTab == TabId::TAB_MAIN || _activeTab == TabId::TAB_DRUM || _activeTab == TabId::TAB_MIDI) {
        if (realIdx < _masterParamCount) return &_masterParams[realIdx];
        return nullptr;
    }

    Instrument *inst = getActiveInstrument();
    if (!inst) return nullptr;
    const ParamDescriptor *params = inst->getParams();
    if (realIdx < inst->getParamCount()) return &params[realIdx];
    return nullptr;
}

uint8_t System::getTabParamRealIndex(uint8_t idx) const {
    if (idx < _tabParamCount) return _tabIndices[idx];
    return 0;
}

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

void System::handleParamSelect() {
    int delta = _encoder->getDelta();
    if (delta != 0 && _tabParamCount > 0) {
        int next = (int)_selectedParam + delta;
        if (next < 0) next = 0;
        if (next >= _tabParamCount) next = _tabParamCount - 1;
        _selectedParam = (uint8_t)next;
    }

    if (_encoder->wasLongPressed()) {
        enterState(NavState::TAB_SELECT);
        return;
    }

    if (_encoder->wasPressed()) {
        enterState(NavState::PARAM_EDIT);
    }
}

void System::handleParamEdit() {
    int delta = _encoder->getDelta();
    if (delta != 0) {
        const ParamDescriptor *desc = getTabParamDescriptor(_selectedParam);
        if (desc) {
            desc->adjust(delta);

            if (_activeTab == TabId::TAB_MAIN || _activeTab == TabId::TAB_DRUM || _activeTab == TabId::TAB_MIDI) {
                onMasterParamChanged(_tabIndices[_selectedParam]);
            } else {
                Instrument *inst = getActiveInstrument();
                if (inst) inst->onParamChanged(_tabIndices[_selectedParam]);
            }
            if (getActiveInstrument()) getActiveInstrument()->needsUIRedraw = true;
        }
    }

    if (_encoder->wasPressed()) {
        enterState(NavState::PARAM_SELECT);
    }
}

void System::onMasterParamChanged(uint8_t idx) {
    if (idx == 0) { // Synth Engine
        uint8_t newInst = (uint8_t)roundf(_param_engine);
        if (newInst < NUM_INSTRUMENTS && newInst != _currentInstrument) {
            switchInstrument(newInst);
        }
    } else if (idx == 1) { // Synth Patch
        Instrument *inst = getActiveInstrument();
        if (inst) {
            inst->setPatch((int)roundf(_param_patch));
        }
    } else if (idx == 2 || idx == 8) { // Drum Kit
        int kit = (int)roundf(_param_drum_kit);
        if (kit == 0) { // 808 Classic
            _param_kick_tune = 0.0f; _param_snare_tune = 0.0f; _param_tom_tune = 0.0f; _param_drum_drive = 2.0f;
        } else if (kit == 1) { // 909
            _param_kick_tune = 2.0f; _param_snare_tune = 3.0f; _param_tom_tune = 2.0f; _param_drum_drive = 2.5f;
        } else if (kit == 2) { // Linn 9000
            _param_kick_tune = -1.0f; _param_snare_tune = 2.0f; _param_tom_tune = 0.0f; _param_drum_drive = 2.2f;
        } else if (kit == 3) { // MR-12
            _param_kick_tune = 4.0f; _param_snare_tune = 1.0f; _param_tom_tune = 3.0f; _param_drum_drive = 2.0f;
        } else if (kit == 4) { // Tokyo Synth
            _param_kick_tune = -4.0f; _param_snare_tune = 5.0f; _param_tom_tune = 7.0f; _param_drum_drive = 3.0f;
        } else if (kit == 5) { // Power Kit
            _param_kick_tune = 0.0f; _param_snare_tune = -2.0f; _param_tom_tune = -3.0f; _param_drum_drive = 3.2f;
        } else if (kit == 6) { // Percussion
            _param_kick_tune = -3.0f; _param_snare_tune = 0.0f; _param_tom_tune = 5.0f; _param_drum_drive = 1.8f;
        } else if (kit == 7) { // 808 Electro
            _param_kick_tune = 3.0f; _param_snare_tune = 4.0f; _param_tom_tune = 2.0f; _param_drum_drive = 2.8f;
        } else if (kit == 8) { // 808 Sub Boom
            _param_kick_tune = -7.0f; _param_snare_tune = -2.0f; _param_tom_tune = -5.0f; _param_drum_drive = 3.5f;
        }
        updateDrumEngine();
    } else if (idx == 3) { // Synth Vol
        amy_event e = amy_default_event();
        e.synth = 1;
        e.volume = _param_synth_vol / 100.0f;
        amy_add_event(&e);
    } else if (idx == 4 || idx == 9) { // Drum Vol
        amy_event e = amy_default_event();
        e.synth = 10;
        e.volume = _param_drum_vol / 100.0f;
        amy_add_event(&e);
    } else if (idx == 5) { // Audio In Vol
        updateAudioIn();
    } else if (idx == 6) { // Voice Mode
        Instrument *inst = getActiveInstrument();
        if (inst) {
            inst->params.voice_mode = _param_voice_mode;
            inst->applyVoiceMode();
        }
    } else if (idx == 7) { // Glide Time
        Instrument *inst = getActiveInstrument();
        if (inst) {
            inst->params.glide_ms = _param_glide;
            inst->applyVoiceMode();
        }
    } else if (idx >= 10 && idx <= 13) { // Drum Tuning & Drive
        updateDrumEngine();
    } else if (idx == 14) { // Synth MIDI Ch
        if (_midi) {
            _midi->setChannel((uint8_t)roundf(_param_synth_midi_ch));
        }
    } else if (idx == 15) { // Drum MIDI Ch
        if (_midi) {
            _midi->setDrumChannel((uint8_t)roundf(_param_drum_midi_ch));
        }
    }
}

void System::updateAudioIn() {
    amy_event e = amy_default_event();
    e.osc = 60; // Live Audio Input stream
    e.wave = AUDIO_IN0;
    float vol = _param_audio_in_vol / 100.0f;
    e.amp_coefs[COEF_CONST] = vol;
    e.velocity = (vol > 0.001f) ? 1.0f : 0.0f;
    amy_add_event(&e);
}

void System::updateDrumEngine() {
    amy_event e = amy_default_event();
    e.reset_osc = RESET_PATCH;
    e.patch_number = 1025;
    amy_add_event(&e);

    e = amy_default_event();
    e.patch_number = 1025;
    e.wave = PCM;
    e.preset = 1;
    amy_add_event(&e);

    e = amy_default_event();
    e.synth = 10;
    e.patch_number = 1025;
    e.num_voices = 6;
    e.volume = (_param_drum_vol / 100.0f) * (_param_drum_drive / 2.0f);
    e.synth_flags = _SYNTH_FLAGS_MIDI_DRUMS | _SYNTH_FLAGS_IGNORE_NOTE_OFFS;
    amy_add_event(&e);
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

    // Start active melodic synth on Channel 1
    if (_instruments[_currentInstrument]) {
        _instruments[_currentInstrument]->start();
    }

    // Always maintain background multi-timbral Drum Synth on MIDI Channel 10
    updateDrumEngine();

    // Always maintain background live Audio In pass-through
    updateAudioIn();

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
