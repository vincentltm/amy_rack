#pragma once
#include "Instrument.h"
#include "analog_patches.h"

class InstrumentAnalog : public Instrument {
public:
    InstrumentAnalog();

    void init() override;
    void start() override;
    void stop() override;
    void drawUI(U8G2 &u8g2) override;
    
    void onParamChanged(uint8_t paramIndex) override;
    const ParamDescriptor *getParams() const override { return _analogParams; }
    uint8_t getParamCount() const override { return _analogParamCount; }

    int getPatchCount() const override { return 12; }
    int getCurrentPatch() const override { return _currentPatch; }
    void setPatch(int index) override;
    const char *getPatchName(int idx) const override;

private:
    int _currentPatch = 0;

    float _osc1_wave_f = 2.0f; // Default Saw
    float _osc2_wave_f = 2.0f; // Default Saw
    float _osc2_detune_pct = 25.0f; // 0-100%
    float _osc_balance_pct = 50.0f; // 0-100%

    ParamDescriptor _analogParams[MAX_PARAMS];
    uint8_t _analogParamCount = 0;

    static constexpr uint8_t OSC_1 = 0;
    static constexpr uint8_t OSC_2 = 1;
    static constexpr uint8_t OSC_NOISE = 2;
    static constexpr uint8_t OSC_LFO_FILTER = 3;
    static constexpr uint8_t OSC_LFO_PITCH = 4;

    void setupSynthVoices();
    void loadPreset(int idx);
    void updateOsc1Wave();
    void updateOsc2Wave();
    void updateOscDetune();
    void sendAdsr() override;
    void sendFilter() override;
    void updateOscBalance();
    void configNoise();
    void configLfo();
    
    void drawWaveShape(U8G2 &u8g2, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t waveType);
};
