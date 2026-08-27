#pragma once
#include <Arduino.h>
#include "Config.h"
#include "amy.h"
#include "CVManager.h"

class MidiManager {
public:
    MidiManager();
    void begin(CVManager& cv);
    void begin();
    void update();
    void installMidiHook(amy_config_t& config);
    void setMidiInputHook(amy_config_t& config);
    
    void setChannel(uint8_t ch) { listenChannel = ch; }
    uint8_t getChannel() const { return listenChannel; }

    void setDrumChannel(uint8_t ch) { drumChannel = ch; }
    uint8_t getDrumChannel() const { return drumChannel; }

    uint8_t getLastNote() const { return lastNote; }
    uint8_t getLastChannel() const { return lastChannel; }
    bool isNoteActive() const { return noteActive; }

    void setCVManager(CVManager* cv) { cvManager = cv; }

    static void onMidiReceived(uint8_t* bytes, uint16_t len, uint8_t is_sysex);

private:
    static MidiManager* instance;
    CVManager* cvManager = nullptr;
    
    uint8_t listenChannel = 0; // 0 = Ch 1, 15 = Ch 16, 16 = Omni
    uint8_t drumChannel = 9;   // 9 = Ch 10
    uint8_t lastNote = 0;
    uint8_t lastChannel = 0;
    bool noteActive = false;
};
