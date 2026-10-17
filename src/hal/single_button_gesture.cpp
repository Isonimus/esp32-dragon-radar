#include "single_button_gesture.h"

InputAction SingleButtonGesture::update(bool pressed, uint32_t nowMs) {
    if (pressed != _lastReading) {
        _lastChangeMs = nowMs;
        _lastReading  = pressed;
    }
    if (nowMs - _lastChangeMs < DEBOUNCE_MS) return InputAction::NONE;

    bool justPressed  = pressed && !_stable;
    bool justReleased = !pressed && _stable;
    _stable = pressed;

    // A gesture returns to IDLE with the button still down; IDLE waits for a
    // fresh press, which needs a release first, so letting go adds nothing.
    switch (_state) {
        case State::IDLE:
            if (justPressed) {
                _state        = State::PRESSED;
                _pressStartMs = nowMs;
            }
            break;
        case State::PRESSED:
            if (justReleased) {
                _state        = State::WAIT_SECOND_PRESS;
                _releasedAtMs = nowMs;
            }
            break;
        case State::WAIT_SECOND_PRESS:
            if (justPressed) {
                _state = State::IDLE;
                return InputAction::SECONDARY;
            }
            break;
    }

    if (_state == State::PRESSED && nowMs - _pressStartMs >= LONG_PRESS_MS) {
        _state = State::IDLE;
        return InputAction::LONG_PRIMARY;
    }
    if (_state == State::WAIT_SECOND_PRESS && nowMs - _releasedAtMs >= DOUBLE_TAP_MS) {
        _state = State::IDLE;
        return InputAction::PRIMARY;
    }
    return InputAction::NONE;
}
