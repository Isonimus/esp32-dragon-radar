#include "cardputer_i2c.h"
#include <Wire.h>

namespace cardputer_i2c {

static constexpr int      SDA_PIN = 8;
static constexpr int      SCL_PIN = 9;
static constexpr uint32_t BUS_FREQUENCY_HZ = 400000;

static bool started = false;

bool begin() {
    if (started) return true;
    started = Wire.begin(SDA_PIN, SCL_PIN, BUS_FREQUENCY_HZ);
    return started;
}

bool writeRegister(uint8_t deviceAddress, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(deviceAddress);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool readRegister(uint8_t deviceAddress, uint8_t reg, uint8_t& value) {
    Wire.beginTransmission(deviceAddress);
    Wire.write(reg);
    // Repeated start: release the bus only after the read completes.
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(deviceAddress, static_cast<uint8_t>(1)) != 1) return false;
    value = static_cast<uint8_t>(Wire.read());
    return true;
}

} // namespace cardputer_i2c
