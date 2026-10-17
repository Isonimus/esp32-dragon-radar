#ifndef CARDPUTER_GPS_H
#define CARDPUTER_GPS_H

#include "../../gps_hal.h"
#include "../../../gps/gps_driver.h"
#include "gps_source.h"

/**
 * @brief Cardputer ADV GPS on whichever official accessory is attached (ADR-0005).
 *
 * Listens on the GPS/LoRa cap (RX G15, TX G13) and the Grove port (RX G1, TX G2) at
 * once, and keeps the first port to deliver a checksum-valid NMEA sentence. Both
 * accessories carry an AT6668 at 115200; pins and baud were measured by the sibling
 * adversary firmware on this hardware.
 */
class CardputerGps : public GpsHAL {
public:
    void begin() override;
    void update() override;
    GpsFix getFix() override;

private:
    struct GpsPort {
        const char* name;
        HardwareSerial& serial;
        int rxPin;
        int txPin;
    };

    static constexpr uint32_t GPS_BAUD = 115200;
    // Cap first: when both answer in the same update the order decides, and either works.
    static constexpr size_t PORT_COUNT = 2;
    static const GpsPort PORTS[PORT_COUNT];

    void chooseSource();

    GpsDriver _drivers[PORT_COUNT];
    int _source = NO_GPS_SOURCE;
};

#endif // CARDPUTER_GPS_H
