#pragma once
#include "Instrument.h"

class InstrumentAnalog : public Instrument {
public:
    InstrumentAnalog();

    void init() override;
    void start() override;
    void stop() override;
    
    void onParamChanged(uint8_t paramIndex) override;
    const ParamDescriptor *getParams() const override { return _analogParams; }
    uint8_t getParamCount() const override { return _analogParamCount; }

private:
    float _osc1_wave_f = 0.0f;
    float _osc2_wave_f = 0.0f;
    float _osc2_detune = 0.5f;
    float _osc_balance = 0.5f;

    ParamDescriptor _analogParams[MAX_PARAMS];
    uint8_t _analogParamCount = 0;

    static constexpr uint8_t OSC_1 = 0;
    static constexpr uint8_t OSC_2 = 1;
    static constexpr uint8_t OSC_NOISE = 2;
    static constexpr uint8_t OSC_LFO_FILTER = 3;
    static constexpr uint8_t OSC_LFO_PITCH = 4;

    void setupSynthVoices();
    void updateOsc1Wave();
    void updateOsc2Wave();
    void updateOscDetune();
    void sendAdsr() override;
    void updateOscBalance();
    void configNoise();
    void configLfo();
};
