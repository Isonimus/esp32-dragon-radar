#ifndef M5STICKC_IMU_H
#define M5STICKC_IMU_H

#include "../../imu_hal.h"
#include "../../../imu/imu_driver.h"

/**
 * @brief M5StickC Plus 2 IMU via MPU6886.
 * Wraps the existing ImuDriver.
 */
class M5StickCImu : public ImuHAL {
public:
    bool begin() override;
    void update() override;
    float getHeading() override;
    bool available() override;

private:
    ImuDriver _imu;
    bool _ok = false;
};

#endif // M5STICKC_IMU_H
