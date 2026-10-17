#ifndef M5STICKC_GPS_H
#define M5STICKC_GPS_H

#include "../../gps_hal.h"
#include "../../../gps/gps_driver.h"

/**
 * @brief M5StickC Plus 2 GPS via Grove port (GPIO 33 RX, GPIO 32 TX) at 115200 baud.
 */
class M5StickCGps : public GpsHAL {
public:
    void begin() override;
    void update() override;
    GpsFix getFix() override;

private:
    static constexpr int GPS_RX_PIN = 33;
    static constexpr int GPS_TX_PIN = 32;
    static constexpr uint32_t GPS_BAUD = 115200;

    GpsDriver _driver;
};

#endif // M5STICKC_GPS_H
