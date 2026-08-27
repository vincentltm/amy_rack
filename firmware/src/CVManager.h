#pragma once
#include "Config.h"

extern "C" {
#include <amy.h>
}

class CVManager {
public:
    void begin();
    void update();
    
    void setupCVInput();
    void handleCVInput(float gateVoltage, float pitchVoltage);
    
    void setCVOut(uint8_t channel, float voltage);
    void midiNoteToCVOut(uint8_t note);
    void midiNoteOffCVOut();
    void setGate(bool high);

    bool isGateInActive() const { return gateInState; }
    uint8_t getLastCVNote() const { return lastCVNote; }

private:
    bool gateInState = false;
    uint8_t lastCVNote = 60;
};
