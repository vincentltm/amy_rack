#include "MidiManager.h"

MidiManager* MidiManager::instance = nullptr;

MidiManager::MidiManager(CVManager* cv) : cvManager(cv) {
    instance = this;
}

void MidiManager::begin() {
    instance = this;
}

void MidiManager::begin(CVManager& cv) {
    cvManager = &cv;
    instance = this;
}

void MidiManager::update() {
    // Handled by AMY internally
}

void MidiManager::setMidiInputHook(amy_config_t& config) {
    config.amy_external_midi_input_hook = onMidiReceived;
}

void MidiManager::onMidiReceived(uint8_t* bytes, uint16_t len, uint8_t is_sysex) {
    if (!instance || len == 0 || is_sysex) return;

    // A typical MIDI message is up to 3 bytes
    uint8_t status = bytes[0] & 0xF0;
    uint8_t channel = bytes[0] & 0x0F;

    if (status == 0x90) { // Note On
        if (len >= 3) {
            uint8_t note = bytes[1];
            uint8_t velocity = bytes[2];
            
            if (velocity > 0) {
                instance->lastNote = note;
                instance->lastChannel = channel;
                instance->noteActive = true;
                if (instance->cvManager) {
                    instance->cvManager->midiNoteToCVOut(note);
                }
            } else {
                // Velocity 0 is Note Off
                if (instance->lastNote == note && instance->lastChannel == channel) {
                    instance->noteActive = false;
                }
                if (instance->cvManager) {
                    instance->cvManager->midiNoteOffCVOut();
                }
            }
        }
    } else if (status == 0x80) { // Note Off
        if (len >= 2) {
            uint8_t note = bytes[1];
            if (instance->lastNote == note && instance->lastChannel == channel) {
                instance->noteActive = false;
            }
            if (instance->cvManager) {
                instance->cvManager->midiNoteOffCVOut();
            }
        }
    }
}
