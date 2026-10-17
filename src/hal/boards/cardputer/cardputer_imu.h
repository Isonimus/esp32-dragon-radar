#ifndef CARDPUTER_IMU_H
#define CARDPUTER_IMU_H

#include "../../imu_hal.h"

/**
 * @brief No-op IMU stub for Cardputer (no IMU hardware).
 */
class CardputerImu : public ImuHAL {
public:
    bool begin() override { return true; }
    void update() override {}
    float getHeading() override { return 0.0f; }
    bool available() override { return false; }
};

#endif // CARDPUTER_IMU_H
