#ifndef CUSTOM_IMU_H
#define CUSTOM_IMU_H

#include "../../imu_hal.h"
#include "../../../compass/compass_driver.h"

/**
 * @brief Custom board IMU backed by the LSM303DLHC compass module.
 * Provides tilt-compensated magnetic heading via I2C (GPIO8/GPIO9).
 */
class CustomImu : public ImuHAL {
public:
    bool begin() override;
    void update() override;
    float getHeading() override;
    bool available() override;

private:
    static constexpr int SDA_PIN = 8;
    static constexpr int SCL_PIN = 9;

    CompassDriver _compass;
    bool _ok = false;
};

#endif // CUSTOM_IMU_H
