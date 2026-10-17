#include "m5stickc_gps.h"

void M5StickCGps::begin() {
    _driver.begin(Serial2, GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD, "grove");
}

void M5StickCGps::update() {
    _driver.update();
}

GpsFix M5StickCGps::getFix() {
    return _driver.getFix();
}
