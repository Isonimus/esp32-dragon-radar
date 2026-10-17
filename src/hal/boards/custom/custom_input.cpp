#include "custom_input.h"

bool CustomInput::begin() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.println("Button input ready (GPIO1).");
    return true;
}

void CustomInput::update() {
    InputAction action = _gesture.update(digitalRead(BUTTON_PIN) == LOW, millis());
    // update() runs every loop but the game reads input at 20 Hz; hold the
    // action until then rather than overwrite it with NONE.
    if (action != InputAction::NONE) _pendingAction = action;
}

InputAction CustomInput::getAction() {
    InputAction action = _pendingAction;
    _pendingAction = InputAction::NONE;
    return action;
}
