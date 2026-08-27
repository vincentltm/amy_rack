#pragma once
#include "Instrument.h"

class InstrumentPiano : public Instrument {
public:
    InstrumentPiano();
    void init() override;
    void start() override;
    void stop() override;
    void drawUI(U8G2 &u8g2) override;
};
