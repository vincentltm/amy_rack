#pragma once
#include "Instrument.h"

class InstrumentDX7 : public Instrument {
public:
    InstrumentDX7() {
        _instrumentName = "DX7";
        _instrumentShortName = "DX7";
    }
    void init() override;
    void start() override;
    void stop() override;
    void sendAdsr() override;
    void drawUI(U8G2 &u8g2) override;

    int getPatchCount() const override { return 128; }
    int getCurrentPatch() const override { return patch; }
    void setPatch(int index) override;
    const char *getPatchName(int idx) const override;

    void onParamChanged(uint8_t paramIndex) override;
    const ParamDescriptor *getParams() const override { return _dx7Params; }
    uint8_t getParamCount() const override { return _dx7ParamCount; }

private:
    uint8_t patch = 0;
    ParamDescriptor _dx7Params[MAX_PARAMS];
    uint8_t _dx7ParamCount = 0;
};
