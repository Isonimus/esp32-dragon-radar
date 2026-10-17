#ifndef M5STICKC_INPUT_H
#define M5STICKC_INPUT_H

#include "../../input_hal.h"
#include "../../press_gesture.h"

/**
 * @brief M5StickC Plus 2 button input mapped to InputAction.
 * BtnA (GPIO 37) → PRIMARY / LONG_PRIMARY
 * BtnB (GPIO 39) → SECONDARY / LONG_SECONDARY
 */
class M5StickCInput : public InputHAL {
public:
    bool begin() override;
    void update() override;
    InputAction getAction() override;

private:
    static constexpr int BTN_A_PIN = 37;
    static constexpr int BTN_B_PIN = 39;

    static constexpr uint32_t LONG_PRESS_MS = 1000;

    PressGesture _buttonA{InputAction::PRIMARY, InputAction::LONG_PRIMARY, LONG_PRESS_MS};
    PressGesture _buttonB{InputAction::SECONDARY, InputAction::LONG_SECONDARY, LONG_PRESS_MS};

    InputAction _pendingAction = InputAction::NONE;
};

#endif // M5STICKC_INPUT_H
