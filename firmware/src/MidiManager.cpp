#include "MidiManager.h"
#include <Arduino.h>

extern "C" {
    void juno_filter_midi_handler(uint8_t *bytes, uint16_t len, uint8_t is_sysex);
}

MidiManager* MidiManager::instance = nullptr;

MidiManager::MidiManager(CVManager* cvManager) : cvManager(cvManager) {
    instance = this;
}

void MidiManager::begin() {
    instance = this;
    amy_global.config.amy_external_midi_input_hook = &MidiManager::onMidiReceived;
}

void MidiManager::begin(CVManager& cv) {
    this->cvManager = &cv;
    instance = this;
    amy_global.config.amy_external_midi_input_hook = &MidiManager::onMidiReceived;
}

void MidiManager::update() {
    // AMY handles MIDI polling internally
}

void MidiManager::setMidiInputHook(amy_config_t& config) {
    instance = this;
    config.amy_external_midi_input_hook = &MidiManager::onMidiReceived;
}

void MidiManager::onMidiReceived(uint8_t* bytes, uint16_t len, uint8_t is_sysex) {
    if (!instance || !bytes || len == 0 || is_sysex) return;

    uint8_t status = bytes[0] & 0xF0;
    uint8_t channel = bytes[0] & 0x0F;

    bool isDrumChannel = (channel == 9); // MIDI Ch 10 (0-indexed 9) is dedicated drum channel

    if (!isDrumChannel && instance->listenChannel < 16 && channel != instance->listenChannel) {
        // Filter out if not on current channel (unless Omni mode)
        return;
    }

    instance->lastChannel = channel;

    if (status == 0x90) { // Note On
        uint8_t note = bytes[1];
        uint8_t velocity = (len > 2) ? bytes[2] : 0;

        if (velocity > 0) {
            instance->lastNote = note;
            instance->noteActive = true;
            if (instance->cvManager) {
                instance->cvManager->midiNoteToCVOut(note);
            }
        } else {
            // Velocity 0 is Note Off
            if (instance->lastNote == note) {
                instance->noteActive = false;
                if (instance->cvManager) {
                    instance->cvManager->midiNoteOffCVOut();
                }
            }
        }
    } else if (status == 0x80) { // Note Off
        uint8_t note = bytes[1];
        if (instance->lastNote == note) {
            instance->noteActive = false;
            if (instance->cvManager) {
                instance->cvManager->midiNoteOffCVOut();
            }
        }
    }

    // Forward to AMY internal handler for MIDI CC / pitch bend
    juno_filter_midi_handler(bytes, len, is_sysex);
}
