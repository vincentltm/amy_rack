#pragma once
// =============================================================================
// System.h — Unified Architecture with Dedicated DRUM Tab and Multi-Timbral Control
// =============================================================================

#include <Arduino.h>
#include "Config.h"
#include "Instrument.h"

class Display;
class EncoderInput;
class CVManager;
class MidiManager;

enum class NavState : uint8_t {
    TAB_SELECT   = 0, // Turning switches MAIN | SYNTH | DRUM | ENV | FX | MIDI
    PARAM_SELECT = 1, // Turning scrolls settings in active tab
    PARAM_EDIT   = 2  // Turning edits highlighted setting
};

class System {
public:
    void begin(Display &disp, EncoderInput &enc, CVManager &cv, MidiManager &midi);
    void update();

    // --- State accessors ---
    NavState        getNavState()               const { return _navState; }
    TabId           getActiveTab()              const { return _activeTab; }
    Instrument*     getActiveInstrument()       const { return _instruments[_currentInstrument]; }
    uint8_t         getCurrentInstrumentIndex() const { return _currentInstrument; }
    uint8_t         getNumInstruments()         const { return NUM_INSTRUMENTS; }
    const char*     getInstrumentName(uint8_t i) const;

    // Parameter & Tab state
    uint8_t         getSelectedParamIndex()     const { return _selectedParam; }
    bool            isEditingParam()            const { return _navState == NavState::PARAM_EDIT; }
    uint8_t         getTabParamCount()          const { return _tabParamCount; }
    uint8_t         getTabParamRealIndex(uint8_t idx) const;
    const ParamDescriptor* getTabParamDescriptor(uint8_t idx) const;

    void switchInstrument(uint8_t index);
    void updateDrumEngine();
    void updateAudioIn();

private:
    Display      *_display  = nullptr;
    EncoderInput *_encoder  = nullptr;
    CVManager    *_cv       = nullptr;
    MidiManager  *_midi     = nullptr;

    NavState _navState   = NavState::TAB_SELECT;
    TabId    _activeTab  = TabId::TAB_MAIN;

    uint8_t     _currentInstrument = DEFAULT_INSTRUMENT;
    Instrument *_instruments[NUM_INSTRUMENTS];
    void initInstruments();

    // Master & Drum System params (strictly allocated floats)
    float _param_engine        = (float)DEFAULT_INSTRUMENT;
    float _param_patch         = 0.0f;
    float _param_synth_vol     = 85.0f; // 0 - 100%
    float _param_drum_kit      = 0.0f;  // 0..8
    float _param_drum_vol      = 85.0f; // 0 - 100%
    float _param_audio_in_vol  = 0.0f;  // 0 - 100% (live audio in monitor / FX pass-through)
    float _param_voice_mode    = 0.0f;  // 0 = Poly, 1 = Mono Legato
    float _param_glide         = 0.0f;  // 0 - 500 ms
    float _param_kick_tune     = 0.0f;  // -12 to +12 semitones
    float _param_snare_tune    = 0.0f;  // -12 to +12 semitones
    float _param_tom_tune      = 0.0f;  // -12 to +12 semitones
    float _param_drum_drive    = 2.0f;  // 0.5 to 5.0
    float _param_synth_midi_ch = 0.0f;  // 0 = Ch 1, 15 = Ch 16, 16 = Omni
    float _param_drum_midi_ch  = 9.0f;  // 9 = Ch 10
    float _param_cv1           = 0.0f;  // 0 = V/Oct, 1 = Cutoff, 2 = Vol, 3 = Off
    float _param_cv2           = 0.0f;  // 0 = Gate, 1 = ModWhl, 2 = Res, 3 = Off

    ParamDescriptor _masterParams[20];
    uint8_t _masterParamCount = 0;
    void initMasterParams();

    // Parameter filtering per tab
    uint8_t _selectedParam  = 0;
    uint8_t _paramScrollTop = 0;
    uint8_t _tabIndices[MAX_PARAMS];
    uint8_t _tabParamCount  = 0;

    void updateTabParams();
    void onMasterParamChanged(uint8_t idx);

    // State machine handlers
    void handleTabSelect();
    void handleParamSelect();
    void handleParamEdit();

    void enterState(NavState newState);
};

extern System Sys;
