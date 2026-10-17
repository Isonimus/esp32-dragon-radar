#ifndef CARDPUTER_INPUT_H
#define CARDPUTER_INPUT_H

#include "../../input_hal.h"
#include "../../press_gesture.h"
#include "tca8418_keymap.h"

/**
 * @brief Cardputer ADV keyboard, read from its TCA8418 controller over I2C.
 *
 * Enter and Backspace mirror the M5StickC's two buttons; the arrows navigate:
 *   Enter     → PRIMARY   (hold: LONG_PRIMARY)
 *   Backspace → SECONDARY (hold: LONG_SECONDARY)
 *   ↑ / ↓     → UP / DOWN (a hold counts as one step)
 *
 * The controller queues key events and pulls its INT line low while any are
 * pending, so a frame with no keypress costs one digitalRead and no I2C traffic.
 */
class CardputerInput : public InputHAL {
public:
    bool begin() override;
    void update() override;
    InputAction getAction() override;

private:
    static constexpr int KEYBOARD_INT_PIN = 11;
    static constexpr uint32_t LONG_PRESS_MS = 1000;

    struct KeyBinding {
        CardputerKey key;
        PressGesture gesture;
        bool held;
    };

    KeyBinding _bindings[4] = {
        {cardputer_keys::ENTER, PressGesture{InputAction::PRIMARY, InputAction::LONG_PRIMARY, LONG_PRESS_MS}, false},
        {cardputer_keys::BACKSPACE, PressGesture{InputAction::SECONDARY, InputAction::LONG_SECONDARY, LONG_PRESS_MS}, false},
        {cardputer_keys::UP, PressGesture{InputAction::UP, InputAction::UP, LONG_PRESS_MS}, false},
        {cardputer_keys::DOWN, PressGesture{InputAction::DOWN, InputAction::DOWN, LONG_PRESS_MS}, false},
    };

    InputAction _pendingAction = InputAction::NONE;

    void drainKeyEvents();
    void applyKeyEvent(const CardputerKeyEvent& event);
    void latch(InputAction action);
};

#endif // CARDPUTER_INPUT_H
