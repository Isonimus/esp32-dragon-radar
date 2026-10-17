#ifndef INPUT_HAL_H
#define INPUT_HAL_H

#include <Arduino.h>

/**
 * @brief Unified input actions across all boards.
 * Keyboard keys, physical buttons, and touch gestures all
 * map to these abstract actions.
 */
enum class InputAction : uint8_t {
    NONE,
    PRIMARY,           // Confirm / select / tap
    SECONDARY,         // Back / alt / double-tap
    UP,                // Navigate up
    DOWN,              // Navigate down
    LONG_PRIMARY,      // Long press on primary
    LONG_SECONDARY     // Long press on secondary
};

/**
 * @brief Abstract input interface.
 */
class InputHAL {
public:
    virtual ~InputHAL() = default;

    /**
     * @brief Bring up the input hardware.
     * @return false if it did not respond. The game cannot be driven without
     *         input, so the caller must stop rather than boot into a dead UI.
     */
    virtual bool begin() = 0;

    /** @brief Poll hardware and latch new input state. Call once per frame. */
    virtual void update() = 0;

    /** @brief Return the highest-priority action detected this frame. */
    virtual InputAction getAction() = 0;
};

#endif // INPUT_HAL_H
