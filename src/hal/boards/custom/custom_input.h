#ifndef CUSTOM_INPUT_H
#define CUSTOM_INPUT_H

#include "../../input_hal.h"
#include "../../single_button_gesture.h"

/**
 * @brief Push-button input for the custom ESP32-C3 board.
 *
 * Uses the SW (switch) pin of a KY-040 rotary encoder module on GPIO1.
 * The switch is active-low: normally HIGH (via INPUT_PULLUP), LOW when pressed.
 * The one button carries every action through tap, double tap and long press.
 */
class CustomInput : public InputHAL {
public:
    bool begin() override;
    void update() override;
    InputAction getAction() override;

private:
    static constexpr int BUTTON_PIN = 1;

    SingleButtonGesture _gesture;
    InputAction         _pendingAction = InputAction::NONE;
};

#endif // CUSTOM_INPUT_H
