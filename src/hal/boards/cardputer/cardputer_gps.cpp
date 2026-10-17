#include "cardputer_gps.h"

const CardputerGps::GpsPort CardputerGps::PORTS[PORT_COUNT] = {
    {"cap", Serial1, 15, 13},
    {"grove", Serial2, 1, 2},
};

void CardputerGps::begin() {
    for (size_t port = 0; port < PORT_COUNT; ++port) {
        _drivers[port].begin(PORTS[port].serial, PORTS[port].rxPin, PORTS[port].txPin,
                             GPS_BAUD, PORTS[port].name);
    }
}

void CardputerGps::update() {
    if (_source != NO_GPS_SOURCE) {
        _drivers[_source].update();
        return;
    }
    for (GpsDriver& driver : _drivers) {
        driver.update();
    }
    chooseSource();
}

void CardputerGps::chooseSource() {
    uint32_t validSentenceCounts[PORT_COUNT];
    for (size_t port = 0; port < PORT_COUNT; ++port) {
        validSentenceCounts[port] = _drivers[port].validSentenceCount();
    }
    _source = chooseGpsSource(validSentenceCounts, PORT_COUNT);
    if (_source == NO_GPS_SOURCE) return;

    // The unused port is released so its pins stop feeding a parser nobody reads. The
    // choice is never revisited: a swapped accessory needs a reboot (ADR-0005).
    for (size_t port = 0; port < PORT_COUNT; ++port) {
        if (static_cast<int>(port) != _source) {
            _drivers[port].end();
        }
    }
    Serial.printf("[GPS] using the %s port\n", PORTS[_source].name);
}

GpsFix CardputerGps::getFix() {
    if (_source == NO_GPS_SOURCE) {
        GpsFix noFix = {};
        return noFix;
    }
    return _drivers[_source].getFix();
}
