#include "EncoderInput.h"

void EncoderInput::begin() {
    if (!seesaw.begin(ENCODER_I2C_ADDR)) {
        Serial.println("[Encoder] ERROR: seesaw not found at 0x36!");
    } else {
        seesaw.pinMode(SEESAW_BUTTON_PIN, INPUT_PULLUP);
        seesaw.enableEncoderInterrupt();
        lastPosition = seesaw.getEncoderPosition();
        buttonState = seesaw.digitalRead(SEESAW_BUTTON_PIN);
    }
}

void EncoderInput::update() {
    // 1. Read encoder rotation (clean linear delta)
    int32_t currentPosition = seesaw.getEncoderPosition();
    int32_t diff = currentPosition - lastPosition;
    
    if (diff != 0) {
        currentDelta += diff;
        lastPosition = currentPosition;
    }
    
    // 2. Read button (active LOW: 0 = pressed, 1 = released)
    bool currentButton = seesaw.digitalRead(SEESAW_BUTTON_PIN);
    
    if (buttonState && !currentButton) {
        // Just pressed down (transition HIGH -> LOW)
        pressStartTime = millis();
        longPressHandled = false;
    } else if (!buttonState && !currentButton) {
        // Still held down
        if (!longPressHandled && (millis() - pressStartTime >= ENCODER_LONG_PRESS_MS)) {
            eventLongPressed = true;
            longPressHandled = true; // Handled! Don't trigger short press upon release
        }
    } else if (!buttonState && currentButton) {
        // Just released (transition LOW -> HIGH)
        if (!longPressHandled) {
            // Only trigger short tap if it was not already a long-press
            eventPressed = true;
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
