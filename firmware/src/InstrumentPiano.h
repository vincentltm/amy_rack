#pragma once
#include "Instrument.h"

class InstrumentPiano : public Instrument {
public:
    InstrumentPiano();

    void init() override;
    void start() override;
    void stop() override;
    void drawUI(U8G2 &u8g2) override;

    int getPatchCount() const override { return 5; }
    int getCurrentPatch() const override { return _currentPatch; }
    void setPatch(int index) override;
    const char *getPatchName(int idx) const override;

private:
    int _currentPatch = 0;
};
