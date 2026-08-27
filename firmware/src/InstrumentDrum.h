#pragma once
#include "Instrument.h"

#define DRUM_VOICES 8
#define DRUM_NUM_PADS 8

struct DrumMapEntry {
    uint8_t  preset;    // AMY ROM PCM preset (0..10)
    int8_t   pitchOffset;
    uint8_t  padIndex;  // 0..7
};

class InstrumentDrum : public Instrument {
public:
    InstrumentDrum();

    void init() override;
    void start() override;
    void stop() override;
    void drawUI(U8G2 &u8g2) override;

    void noteOn(uint8_t note, float velocity) override;
    void noteOff(uint8_t note) override;

    void onParamChanged(uint8_t paramIndex) override;
    const ParamDescriptor *getParams() const override { return _drumParams; }
    uint8_t getParamCount() const override { return _drumParamCount; }

    uint8_t getSynthChannel() override { return 1; }

    int getPatchCount() const override { return 4; }
    int getCurrentPatch() const override { return _currentKit; }
    void setPatch(int index) override;
    const char *getPatchName(int idx) const override;

private:
    int _currentKit = 0;

    // Parameters
    float _param_kit = 0.0f;
    float _param_kick_tune = 0.0f;   // -12 to +12 semitones
    float _param_snare_tune = 0.0f;  // -12 to +12 semitones
    float _param_tom_tune = 0.0f;    // -12 to +12 semitones
    float _param_drive = 2.0f;       // 0.5 to 5.0

    // 8 Voice round robin oscillator pool (Oscs 0..7)
    uint8_t _currentVoice = 0;
    uint8_t _voiceOscs[DRUM_VOICES] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    // Pad flash animation timestamps (millis)
    unsigned long _padFlashTime[DRUM_NUM_PADS] = { 0 };

    ParamDescriptor _drumParams[MAX_PARAMS];
    uint8_t _drumParamCount = 0;

    void setupSynthVoices();
    void triggerDrum(uint8_t preset, uint8_t pitch, float velocity, uint8_t padIdx);
};
