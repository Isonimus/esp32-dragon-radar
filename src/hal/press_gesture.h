#ifndef PRESS_GESTURE_H
#define PRESS_GESTURE_H

#include "input_hal.h"

/**
 * @brief Turns one key's held/released samples into tap and long-press actions.
 *
 * A tap fires on release, so it can never also become a long press; a long
 * press fires once while still held, so the player gets feedback without
 * letting go. Pure logic: the caller supplies the key state and the clock,
 * which keeps it host-testable and shared by every board with discrete keys.
 */
class PressGesture {
public:
    PressGesture(InputAction tapAction, InputAction longPressAction, uint32_t longPressMs);

    /** @brief Feed the current key state; returns the action it completes, or NONE. */
    InputAction update(bool pressed, uint32_t nowMs);

private:
    const InputAction _tapAction;
    const InputAction _longPressAction;
    const uint32_t    _longPressMs;

    bool     _wasPressed   = false;
    bool     _longFired    = false;
    uint32_t _pressStartMs = 0;
};

#endif // PRESS_GESTURE_H
