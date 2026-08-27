#include "MidiManager.h"
#include "amy.h"
#include "amy_midi.h"

extern "C" {
    extern void juno_filter_midi_handler(uint8_t *bytes, uint16_t len, uint8_t is_sysex);
}

MidiManager* MidiManager::instance = nullptr;

MidiManager::MidiManager() {
    instance = this;
}

void MidiManager::begin(CVManager& cv) {
    cvManager = &cv;
    listenChannel = 0; // Ch 1
    drumChannel = 9;   // Ch 10
    lastNote = 0;
    lastChannel = 0;
    noteActive = false;
}

void MidiManager::begin() {
    listenChannel = 0;
    drumChannel = 9;
    lastNote = 0;
    lastChannel = 0;
    noteActive = false;
}

void MidiManager::update() {
    // AMY processes MIDI in the background
}

void MidiManager::installMidiHook(amy_config_t& config) {
    instance = this;
    config.amy_external_midi_input_hook = &MidiManager::onMidiReceived;
}

void MidiManager::setMidiInputHook(amy_config_t& config) {
    installMidiHook(config);
}

void MidiManager::onMidiReceived(uint8_t* bytes, uint16_t len, uint8_t is_sysex) {
    if (!instance || !bytes || len == 0 || is_sysex) return;

    uint8_t status = bytes[0] & 0xF0;
    uint8_t channel = bytes[0] & 0x0F;

    bool isDrumMsg = (instance->drumChannel < 16 && channel == instance->drumChannel);
    bool isSynthMsg = (instance->listenChannel >= 16 || channel == instance->listenChannel);

    if (!isDrumMsg && !isSynthMsg) {
        return; // Message is on an unmonitored MIDI channel
    }

    instance->lastChannel = channel;

    if (status == 0x90) { // Note On
        uint8_t note = bytes[1];
        uint8_t velocity = (len > 2) ? bytes[2] : 0;

        if (velocity > 0) {
            instance->lastNote = note;
            instance->noteActive = true;
            if (instance->cvManager && isSynthMsg) {
                instance->cvManager->midiNoteToCVOut(note);
            }
        } else {
            // Velocity 0 is Note Off
            if (instance->lastNote == note) {
                instance->noteActive = false;
                if (instance->cvManager && isSynthMsg) {
                    instance->cvManager->midiNoteOffCVOut();
                }
            }
        }
    } else if (status == 0x80) { // Note Off
        uint8_t note = bytes[1];
        if (instance->lastNote == note) {
            instance->noteActive = false;
            if (instance->cvManager && isSynthMsg) {
                instance->cvManager->midiNoteOffCVOut();
            }
        }
    }

    // Forward to AMY internal handler for MIDI CC / pitch bend
    juno_filter_midi_handler(bytes, len, is_sysex);
}
