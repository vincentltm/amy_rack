#pragma once
#include "Config.h"
#include <amy.h>
#include "CVManager.h"

class MidiManager {
public:
    MidiManager(CVManager* cvManager = nullptr);
    void begin();
    void begin(CVManager& cv);
    void update();
    
    void setCVManager(CVManager* cv) { cvManager = cv; }
    void setMidiInputHook(amy_config_t& config);
    void installMidiHook(amy_config_t& config) { setMidiInputHook(config); }
    
    static void onMidiReceived(uint8_t* bytes, uint16_t len, uint8_t is_sysex);
    
    uint8_t getLastNote() const { return lastNote; }
    uint8_t getLastChannel() const { return lastChannel; }
    bool isNoteActive() const { return noteActive; }

private:
    CVManager* cvManager = nullptr;
    static MidiManager* instance;
    
    uint8_t lastNote = 255;
    uint8_t lastChannel = 255;
    bool noteActive = false;
};
