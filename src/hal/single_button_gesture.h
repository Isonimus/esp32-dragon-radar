#ifndef SINGLE_BUTTON_GESTURE_H
#define SINGLE_BUTTON_GESTURE_H

#include "input_hal.h"

/**
 * @brief Gets both primary and secondary actions out of a board's only button.
 *
 * A tap is PRIMARY, a double tap SECONDARY and a long press LONG_PRIMARY. A
 * tap fires only once the double-tap window has passed, since until then it
 * may still become a double tap. Raw samples are debounced first, because a
 * bouncing contact would otherwise read as a double tap. Pure logic: the
 * caller supplies the button state and the clock, so it is host-testable
 * (CLAUDE.md invariant 3).
 */
class SingleButtonGesture {
public:
    // A level must hold this long before it counts as a press or a release.
    static constexpr uint32_t DEBOUNCE_MS = 30;
    static constexpr uint32_t LONG_PRESS_MS = 800;
    // How long after a release a second press still makes a double tap.
    static constexpr uint32_t DOUBLE_TAP_MS = 400;

    /** @brief Feed the raw button state; returns the action it completes, or NONE. */
    InputAction update(bool pressed, uint32_t nowMs);

private:
    enum class State : uint8_t { IDLE, PRESSED, WAIT_SECOND_PRESS };

    bool     _lastReading   = false;
    bool     _stable        = false;
    uint32_t _lastChangeMs  = 0;

    State    _state         = State::IDLE;
    uint32_t _pressStartMs  = 0;
    uint32_t _releasedAtMs  = 0;
};

#endif // SINGLE_BUTTON_GESTURE_H
