#include "cardputer_power.h"

float CardputerPower::getBatteryVoltage() {
    int raw = analogRead(BAT_ADC_PIN);
    return (raw * 3.3f / 4095.0f) * 2.0f;
}
