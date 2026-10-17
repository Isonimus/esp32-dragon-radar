#include "cardputer_input.h"
#include "cardputer_i2c.h"

// TCA8418 registers and bits (datasheet, "Register Descriptions").
static constexpr uint8_t TCA8418_ADDRESS     = 0x34;
static constexpr uint8_t REG_CFG             = 0x01;
static constexpr uint8_t REG_INT_STAT        = 0x02;
static constexpr uint8_t REG_KEY_LCK_EC      = 0x03;
static constexpr uint8_t REG_KEY_EVENT_A     = 0x04;
static constexpr uint8_t REG_KP_GPIO_1       = 0x1D;  // rows 0-7 into the key matrix
static constexpr uint8_t REG_KP_GPIO_2       = 0x1E;  // columns 0-7
static constexpr uint8_t REG_KP_GPIO_3       = 0x1F;  // columns 8-9
static constexpr uint8_t CFG_KEY_EVENT_IRQ   = 0x01;
static constexpr uint8_t INT_STAT_KEY_EVENT  = 0x01;
static constexpr uint8_t INT_STAT_ALL        = 0x1F;
static constexpr uint8_t EVENT_COUNT_MASK    = 0x0F;

// The ADV wires rows 0-6 and columns 0-7; columns 8-9 stay plain GPIO.
static constexpr uint8_t MATRIX_ROW_MASK     = 0x7F;
static constexpr uint8_t MATRIX_COL_MASK     = 0xFF;
static constexpr uint8_t MATRIX_COL_HI_MASK  = 0x00;

bool CardputerInput::begin() {
    if (!cardputer_i2c::begin()) {
        Serial.println("[kbd] I2C bus failed to start");
        return false;
    }

    uint8_t config = 0;
    if (!cardputer_i2c::readRegister(TCA8418_ADDRESS, REG_CFG, config)) {
        Serial.println("[kbd] TCA8418 not responding at 0x34");
        return false;
    }

    bool configured =
        cardputer_i2c::writeRegister(TCA8418_ADDRESS, REG_KP_GPIO_1, MATRIX_ROW_MASK) &&
        cardputer_i2c::writeRegister(TCA8418_ADDRESS, REG_KP_GPIO_2, MATRIX_COL_MASK) &&
        cardputer_i2c::writeRegister(TCA8418_ADDRESS, REG_KP_GPIO_3, MATRIX_COL_HI_MASK) &&
        cardputer_i2c::writeRegister(TCA8418_ADDRESS, REG_CFG, config | CFG_KEY_EVENT_IRQ);
    if (!configured) {
        Serial.println("[kbd] TCA8418 configuration write failed");
        return false;
    }

    // Discard anything queued before boot so a key held during reset is not
    // replayed as a press.
    uint8_t event = 0;
    do {
        if (!cardputer_i2c::readRegister(TCA8418_ADDRESS, REG_KEY_EVENT_A, event)) {
            Serial.println("[kbd] I2C read failed while flushing boot events");
            return false;
        }
    } while (event != 0);
    if (!cardputer_i2c::writeRegister(TCA8418_ADDRESS, REG_INT_STAT, INT_STAT_ALL)) {
        Serial.println("[kbd] I2C clear of boot interrupts failed");
        return false;
    }

    // INT is open-drain and only ever pulled low by the controller.
    pinMode(KEYBOARD_INT_PIN, INPUT_PULLUP);
    Serial.println("[kbd] TCA8418 keyboard ready");
    return true;
}

void CardputerInput::update() {
    if (digitalRead(KEYBOARD_INT_PIN) == LOW) drainKeyEvents();

    // Long presses complete while the key is still held, so the gestures need
    // a tick every frame, not only when the controller reports an event.
    uint32_t now = millis();
    for (KeyBinding& binding : _bindings) {
        latch(binding.gesture.update(binding.held, now));
    }
}

void CardputerInput::drainKeyEvents() {
    uint8_t pending = 0;
    if (!cardputer_i2c::readRegister(TCA8418_ADDRESS, REG_KEY_LCK_EC, pending)) {
        Serial.println("[kbd] I2C read of event count failed");
        return;
    }
    pending &= EVENT_COUNT_MASK;

    for (uint8_t i = 0; i < pending; i++) {
        uint8_t code = 0;
        if (!cardputer_i2c::readRegister(TCA8418_ADDRESS, REG_KEY_EVENT_A, code)) {
            Serial.println("[kbd] I2C read of key event failed");
            return;
        }
        CardputerKeyEvent event;
        if (!decodeTca8418Event(code, event)) {
            Serial.printf("[kbd] undecodable event 0x%02X\n", code);
            continue;
        }
        Serial.printf("[kbd] %s row %u col %u\n",
                      event.pressed ? "down" : "up", event.key.row, event.key.col);
        applyKeyEvent(event);
    }

    // Write-1-to-clear; the controller re-asserts INT if more events arrived meanwhile.
    if (!cardputer_i2c::writeRegister(TCA8418_ADDRESS, REG_INT_STAT, INT_STAT_KEY_EVENT)) {
        Serial.println("[kbd] I2C clear of key interrupt failed");
    }
}

void CardputerInput::applyKeyEvent(const CardputerKeyEvent& event) {
    // A tap can queue its press and release within one frame; feeding each
    // event straight to the gesture keeps that tap from being lost.
    uint32_t now = millis();
    for (KeyBinding& binding : _bindings) {
        if (event.key == binding.key) {
            binding.held = event.pressed;
            latch(binding.gesture.update(binding.held, now));
            return;
        }
    }
}

void CardputerInput::latch(InputAction action) {
    // Keep an unconsumed action; getAction() runs slower than update().
    if (_pendingAction == InputAction::NONE) _pendingAction = action;
}

InputAction CardputerInput::getAction() {
    InputAction act = _pendingAction;
    _pendingAction = InputAction::NONE;
    return act;
}
