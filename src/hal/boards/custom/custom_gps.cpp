#include "custom_gps.h"

void CustomGps::begin() {
    _driver.begin(Serial1, GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD, "custom");
}

void CustomGps::update() {
    _driver.update();
}

GpsFix CustomGps::getFix() {
    return _driver.getFix();
}
