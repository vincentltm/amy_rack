#include "EncoderInput.h"

void EncoderInput::begin() {
    if (!seesaw.begin(ENCODER_I2C_ADDR)) {
        // Initialization failed, maybe add a debug print here
    } else {
        seesaw.pinMode(SEESAW_BUTTON_PIN, INPUT_PULLUP);
        seesaw.enableEncoderInterrupt();
        lastPosition = seesaw.getEncoderPosition();
    }
}

void EncoderInput::update() {
    int32_t currentPosition = seesaw.getEncoderPosition();
    int32_t diff = currentPosition - lastPosition;
    
    if (diff != 0) {
        if (abs(diff) >= ENCODER_ACCEL_THRESHOLD) {
            diff *= ENCODER_ACCEL_FACTOR;
        }
        currentDelta += diff;
        lastPosition = currentPosition;
    }
    
    bool currentButton = seesaw.digitalRead(SEESAW_BUTTON_PIN);
    
    // button logic (active low)
    if (buttonState && !currentButton) { // Just pressed
        pressStartTime = millis();
    } else if (!buttonState && currentButton) { // Just released
        if (millis() - pressStartTime < ENCODER_LONG_PRESS_MS) {
            eventPressed = true;
        }
    } else if (!buttonState && !currentButton) { // Held
        if (millis() - pressStartTime >= ENCODER_LONG_PRESS_MS) {
            eventLongPressed = true;
            // Prevent multiple long presses for a single hold by resetting the timer
            // or just keeping it active until released. 
            // We'll mark the press as 'consumed' by resetting the start time.
            pressStartTime = millis(); 
        }
    }
    
    buttonState = currentButton;
}

int EncoderInput::getDelta() {
    int d = currentDelta;
    currentDelta = 0;
    return d;
}

bool EncoderInput::wasPressed() {
    bool p = eventPressed;
    eventPressed = false;
    return p;
}

bool EncoderInput::wasLongPressed() {
    bool lp = eventLongPressed;
    eventLongPressed = false;
    return lp;
}

bool EncoderInput::isHeld() {
    return !buttonState;
}
