#ifndef COMPASS_DRIVER_H
#define COMPASS_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <LSM303.h>

/**
 * @brief Driver for the LSM303DLHC magnetometer + accelerometer.
 * Provides tilt-compensated magnetic heading via the Pololu LSM303 library.
 */
class CompassDriver {
public:
    /**
     * @brief Initialise I2C on the given pins and configure the sensor.
     * @return true if the LSM303 was detected and initialised.
     */
    bool begin(int sda, int scl);

    /**
     * @brief Read fresh data from the sensor and recompute heading.
     * Call this once per loop iteration (the library handles timing).
     */
    void update();

    /**
     * @brief Tilt-compensated magnetic heading in degrees [0, 360).
     */
    float getHeading() const { return _heading; }

private:
    LSM303 _compass;
    float  _heading  = 0.0f;

    // enableDefault() runs the GY-511's LSM303DLHC magnetometer at 7.5 Hz (~133 ms
    // per sample); reading more often returns the same sample for the cost of an
    // I2C transaction in the frame loop.
    static constexpr uint32_t READ_INTERVAL_MS = 100;
    uint32_t _lastRead = 0;
};

#endif // COMPASS_DRIVER_H
