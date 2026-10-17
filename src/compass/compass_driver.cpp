#include "compass_driver.h"

bool CompassDriver::begin(int sda, int scl) {
    Wire.begin(sda, scl);
    delay(10);  // Let I2C peripheral settle before first transaction

    if (!_compass.init()) {
        Serial.println("LSM303 not found!");
        return false;
    }

    _compass.enableDefault();
    Serial.println("LSM303 compass ready.");
    return true;
}

void CompassDriver::update() {
    // Polling faster than the magnetometer's output rate only repeats I2C reads
    uint32_t now = millis();
    if (now - _lastRead < READ_INTERVAL_MS) return;
    _lastRead = now;

    _compass.read();

    // Pololu heading() returns tilt-compensated heading in degrees [0, 360)
    float raw = _compass.heading();

    // The GY-511 module is mounted 180° rotated on the custom board.
    // Correct by adding a 180° offset.
    _heading = fmod(raw + 180.0f, 360.0f);
}
