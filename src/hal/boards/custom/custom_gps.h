#ifndef CUSTOM_GPS_H
#define CUSTOM_GPS_H

#include "../../gps_hal.h"
#include "../../../gps/gps_driver.h"

/**
 * @brief GPS HAL for the custom ESP32-C3 board.
 * Uses Serial1 (GPIO4 RX, GPIO5 TX) at 9600 baud with a GY-NEO6MV2 breakout. Ours
 * carries a URANUS5 clone chip, not a u-blox NEO-6M; it speaks the same NMEA at 9600.
 */
class CustomGps : public GpsHAL {
public:
    void begin() override;
    void update() override;
    GpsFix getFix() override;

private:
    static constexpr int GPS_RX_PIN = 4;
    static constexpr int GPS_TX_PIN = 5;
    static constexpr uint32_t GPS_BAUD = 9600;

    GpsDriver _driver;
};

#endif // CUSTOM_GPS_H
