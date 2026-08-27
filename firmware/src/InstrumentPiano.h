#pragma once
#include "Instrument.h"

class InstrumentPiano : public Instrument {
public:
    InstrumentPiano() {
        _instrumentName = "Piano";
        _instrumentShortName = "Piano";
    }
    void init() override;
    void start() override;
    void stop() override;

    const ParamDescriptor *getParams() const override { return _baseParams; }
    uint8_t getParamCount() const override { return _baseParamCount; }
};
