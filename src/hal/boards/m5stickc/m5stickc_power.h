#ifndef M5STICKC_POWER_H
#define M5STICKC_POWER_H

#include "../../power_hal.h"
#include <Arduino.h>

/**
 * @brief M5StickC Plus 2 power management.
 * Uses HOLD pin (GPIO 4) for keep-alive and battery ADC (GPIO 38).
 */
class M5StickCPower : public PowerHAL {
public:
    void begin() override;
    void shutdown() override;
    float getBatteryVoltage() override;
    bool hasBattery() override;

private:
    static constexpr int HOLD_PIN    = 4;
    // GPIO 38, not 34: on the Plus 2, GPIO 34 sat at full scale (4095) and showed 6.6 V,
    // while 38 read 1826 mV, a 3.65 V cell through the board's 1:2 divider.
    static constexpr int BAT_ADC_PIN = 38;
    static constexpr float BATTERY_DIVIDER_RATIO = 2.0f;
    static constexpr float MILLIVOLTS_PER_VOLT = 1000.0f;
};

#endif // M5STICKC_POWER_H
