#ifndef IMU_DRIVER_H
#define IMU_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

/**
 * @brief Simple driver for MPU6886 IMU.
 */
class ImuDriver {
public:
    ImuDriver();
    bool begin();
    void update();
    
    /**
     * @brief Get device heading/azimuth in degrees [0, 360).
     * This is an estimation based on gyro/accel.
     */
    float getHeading() const { return _heading; }

private:
    float _heading = 0.0f;
    uint32_t _lastUpdate = 0;
    
    void writeReg(uint8_t reg, uint8_t val);
    uint8_t readReg(uint8_t reg);
};

#endif
