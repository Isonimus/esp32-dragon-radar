#include "m5stickc_input.h"

bool M5StickCInput::begin() {
    // GPIO 37/39 are input-only pads with no internal pull-ups; the board
    // pulls both buttons up externally, so INPUT_PULLUP would only log an error.
    pinMode(BTN_A_PIN, INPUT);
    pinMode(BTN_B_PIN, INPUT);
    return true;
}

void M5StickCInput::update() {
    uint32_t now = millis();
    // Both gestures advance every frame, so an edge on one button is never
    // lost while the other button's action waits to be consumed.
    InputAction actionA = _buttonA.update(digitalRead(BTN_A_PIN) == LOW, now);
    InputAction actionB = _buttonB.update(digitalRead(BTN_B_PIN) == LOW, now);

    // An unconsumed action is kept; A wins a same-frame tie.
    if (_pendingAction == InputAction::NONE) _pendingAction = actionA;
    if (_pendingAction == InputAction::NONE) _pendingAction = actionB;
}

InputAction M5StickCInput::getAction() {
    InputAction act = _pendingAction;
    _pendingAction = InputAction::NONE; // LATCH & CONSUME
    return act;
}
