#include "gps_driver.h"

GpsDriver::GpsDriver(Clock clock) : _clock(clock), _serial(nullptr), _lastDiag(0) {}

uint32_t GpsDriver::boardMillis() {
    return millis();
}

void GpsDriver::begin(HardwareSerial& serial, int rxPin, int txPin, uint32_t baud, const char* portName) {
#ifdef ARDUINO
    _serial = &serial;
    _portName = portName;
    _serial->begin(baud, SERIAL_8N1, rxPin, txPin);
    Serial.printf("[GPS %s] UART on RX:%d TX:%d at %lu baud\n", _portName, rxPin, txPin, (unsigned long)baud);
#endif
}

void GpsDriver::end() {
#ifdef ARDUINO
    if (!_serial) return;
    _serial->end();
    _serial = nullptr;
#endif
}

void GpsDriver::update() {
#ifdef ARDUINO
    if (!_serial) return;
    while (_serial->available() > 0) {
        encode(_serial->read());
    }

    uint32_t now = millis();
    if (now - _lastDiag >= DIAGNOSTICS_INTERVAL_MS) {
        _lastDiag = now;
        Serial.printf("[GPS %s] chars=%lu  parsed=%lu  failed=%lu  sats=%d  fix=%s\n",
            _portName,
            _gps.charsProcessed(),
            _gps.passedChecksum(),
            _gps.failedChecksum(),
            _gps.satellites.isValid() ? (int)_gps.satellites.value() : -1,
            getFix().current ? "YES" : "NO");
    }
#endif
}

bool GpsDriver::encode(char c) {
    bool sentenceEnded = _gps.encode(c);
    // TinyGPS++ counts a sentence with a fix exactly when it stores its position, so a
    // new count marks a new fix. Timing it here, rather than with location.age(), lets
    // the injected clock age it.
    if (_gps.sentencesWithFix() != _fixSentencesSeen) {
        _fixSentencesSeen = _gps.sentencesWithFix();
        _lastFixMs = _clock();
    }
    return sentenceEnded;
}

uint32_t GpsDriver::validSentenceCount() {
    return _gps.passedChecksum();
}

GpsFix GpsDriver::getFix() {
    GpsFix fix;
    fix.hasPosition = _gps.location.isValid();
    if (fix.hasPosition) {
        fix.lat = _gps.location.lat();
        fix.lon = _gps.location.lng();
        fix.altitude = _gps.altitude.meters();
        fix.speed = _gps.speed.mps();
        fix.course = _gps.course.deg();
        fix.satellites = _gps.satellites.value();
        fix.hdop = _gps.hdop.hdop();
        fix.fixAge = _clock() - _lastFixMs;
        fix.current = fix.fixAge <= FIX_MAX_AGE_MS;
    } else {
        fix.current = false;
        fix.lat = 0;
        fix.lon = 0;
        fix.altitude = 0;
        fix.speed = 0;
        fix.course = 0;
        fix.satellites = _gps.satellites.value();
        fix.hdop = 0;
        fix.fixAge = 0;
    }
    return fix;
}
