#ifndef GPS_HAL_H
#define GPS_HAL_H

#include "../gps/gps_driver.h"   // GpsFix struct

/**
 * @brief Abstract GPS interface.
 * All boards use TinyGPS++ under the hood but differ in
 * UART pin assignments and baud rates.
 */
class GpsHAL {
public:
    virtual ~GpsHAL() = default;

    virtual void begin() = 0;
    virtual void update() = 0;
    virtual GpsFix getFix() = 0;
};

#endif // GPS_HAL_H
