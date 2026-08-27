#include "CVManager.h"
#include <Wire.h>
#include <cmath>

void CVManager::begin() {
    setupCVInput();
    
    // For v1.5 DAC half-scale fix: write full-scale range config to register 0x01.
    Wire.beginTransmission(CV_DAC_I2C_ADDR);
    Wire.write(GP8413_REG_RANGE);
    Wire.write(GP8413_RANGE_10V); 
    Wire.endTransmission();

    // Initialize CV outputs to 0V / gate low
    setCVOut(CV_OUT_VOCT, 0.0f);
    setGate(false);
}

void CVManager::update() {
    // Polling hook if CV input ADCs are hooked up
}

void CVManager::setupCVInput() {
    gateInState = false;
    lastCVNote = 60;
}

void CVManager::handleCVInput(float gateVoltage, float pitchVoltage) {
    if (!gateInState && gateVoltage >= CV_GATE_THRESHOLD) {
        gateInState = true;
        // 1V/octave pitch calculation relative to C4 (MIDI note 60 = 0V)
        int note = CV_VOCT_REF_NOTE + (int)roundf(pitchVoltage * 12.0f);
        note = constrain(note, 0, 127);
        lastCVNote = (uint8_t)note;

        // Trigger note in AMY
        amy_event e = amy_default_event();
        e.synth = SYNTH_CHANNEL_DEFAULT;
        e.midi_note = lastCVNote;
        e.velocity = 1.0f;
        amy_add_event(&e);
    } else if (gateInState && gateVoltage < CV_GATE_RESET) {
        gateInState = false;

        // Release note in AMY
        amy_event e = amy_default_event();
        e.synth = SYNTH_CHANNEL_DEFAULT;
        e.midi_note = lastCVNote;
        e.velocity = 0.0f;
        amy_add_event(&e);
    }
}

void CVManager::setCVOut(uint8_t channel, float voltage) {
    if (voltage < CV_DAC_RANGE_MIN) voltage = CV_DAC_RANGE_MIN;
    if (voltage > CV_DAC_RANGE_MAX) voltage = CV_DAC_RANGE_MAX;
    
    // Map -10..+10V to 0..32767 DAC value
    float normalized = (voltage - CV_DAC_RANGE_MIN) / CV_DAC_RANGE_TOTAL;
    uint16_t dacValue = (uint16_t)(normalized * CV_DAC_RESOLUTION);
    
    uint8_t reg = (channel == 0) ? GP8413_REG_CH0 : GP8413_REG_CH1;
    
    Wire.beginTransmission(CV_DAC_I2C_ADDR);
    Wire.write(reg);
    Wire.write(dacValue & 0xFF);         // lowByte
    Wire.write((dacValue >> 8) & 0xFF);  // highByte
    Wire.endTransmission();
}

void CVManager::midiNoteToCVOut(uint8_t note) {
    float voltage = (note - CV_VOCT_REF_NOTE) / 12.0f;
    setCVOut(CV_OUT_VOCT, voltage);
    setGate(true);
}

void CVManager::midiNoteOffCVOut() {
    setGate(false);
}

void CVManager::setGate(bool high) {
    setCVOut(CV_OUT_GATE, high ? CV_GATE_HIGH_V : CV_GATE_LOW_V);
}
