#include "press_gesture.h"

PressGesture::PressGesture(InputAction tapAction, InputAction longPressAction, uint32_t longPressMs)
    : _tapAction(tapAction), _longPressAction(longPressAction), _longPressMs(longPressMs) {}

InputAction PressGesture::update(bool pressed, uint32_t nowMs) {
    InputAction action = InputAction::NONE;

    if (pressed && !_wasPressed) {
        _pressStartMs = nowMs;
        _longFired = false;
    } else if (pressed && !_longFired && (nowMs - _pressStartMs > _longPressMs)) {
        _longFired = true;
        action = _longPressAction;
    } else if (!pressed && _wasPressed && !_longFired) {
        action = _tapAction;
    }

    _wasPressed = pressed;
    return action;
}
