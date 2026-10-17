#ifndef CUSTOM_POWER_H
#define CUSTOM_POWER_H

#include "../../power_hal.h"
#include <Arduino.h>

/**
 * @brief Minimal power HAL for the custom board (USB-powered, no battery).
 */
class CustomPower : public PowerHAL {
public:
    void begin() override {}
    void shutdown() override { ESP.restart(); }
    float getBatteryVoltage() override { return 0.0f; }
    bool hasBattery() override { return false; }
};

#endif // CUSTOM_POWER_H
