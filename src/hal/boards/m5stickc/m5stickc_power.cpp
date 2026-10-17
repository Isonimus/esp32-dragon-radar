#include "m5stickc_power.h"

void M5StickCPower::begin() {
    pinMode(HOLD_PIN, OUTPUT);
    digitalWrite(HOLD_PIN, HIGH);
}

void M5StickCPower::shutdown() {
    digitalWrite(HOLD_PIN, LOW);
    delay(1000);
    ESP.restart();
}

float M5StickCPower::getBatteryVoltage() {
    // analogReadMilliVolts applies the chip's eFuse calibration. The raw count scaled by
    // 3.3 / 4095 read 0.26 V low on this pin, because the ADC is not linear at the 11 dB
    // attenuation a battery reading needs.
    return analogReadMilliVolts(BAT_ADC_PIN) * BATTERY_DIVIDER_RATIO / MILLIVOLTS_PER_VOLT;
}

bool M5StickCPower::hasBattery() {
    return true;
}
