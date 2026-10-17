#ifndef CARDPUTER_POWER_H
#define CARDPUTER_POWER_H

#include "../../power_hal.h"
#include <Arduino.h>

/**
 * @brief Cardputer power HAL.
 * Battery ADC on GPIO 10. No hold-pin — shutdown just restarts.
 */
class CardputerPower : public PowerHAL {
public:
    void begin() override {}
    void shutdown() override { ESP.restart(); }
    float getBatteryVoltage() override;
    bool hasBattery() override { return true; }

private:
    static constexpr int BAT_ADC_PIN = 10;
};

#endif // CARDPUTER_POWER_H
