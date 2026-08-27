#pragma once
#include "Config.h"
#include <Adafruit_seesaw.h>

class EncoderInput {
public:
    void begin();
    void update();
    
    int getDelta();
    bool wasPressed();
    bool wasLongPressed();
    bool isHeld();

private:
    Adafruit_seesaw seesaw;
    int32_t lastPosition = 0;
    int currentDelta = 0;
    
    bool buttonState = true;
    uint32_t pressStartTime = 0;
    
    bool eventPressed = false;
    bool eventLongPressed = false;
};
