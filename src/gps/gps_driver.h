#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#ifdef ARDUINO
#include <Arduino.h>
#endif
#include <TinyGPS++.h>

/**
 * @brief Structure to hold GPS fix data.
 */
struct GpsFix {
    double lat;           // degrees
    double lon;           // degrees
    float  altitude;      // meters
    float  speed;         // m/s
    float  course;        // degrees from north (COG)
    uint8_t satellites;   // number of sats in use
    float  hdop;          // horizontal dilution of precision
    // A position has been received since boot. It may be old: draw from it, but do not
    // treat it as where the player is now.
    bool   hasPosition;
    // The position is at most GpsDriver::FIX_MAX_AGE_MS old, so it describes the player now.
    bool   current;
    uint32_t fixAge;      // ms since the position was received; 0 without one
};

/**
 * @brief Class to handle GPS module communication and parsing.
 */
class GpsDriver {
public:
    // Milliseconds since boot. Injectable so tests can age a fix without waiting.
    using Clock = uint32_t (*)();

    explicit GpsDriver(Clock clock = boardMillis);
    
    /**
     * @brief Initialize UART for GPS communication.
     * @param serial Hardware serial port to use (e.g. Serial1, Serial2)
     * @param rxPin RX pin on ESP32 (TX on GPS module)
     * @param txPin TX pin on ESP32 (RX on GPS module)
     * @param baud UART baud rate
     * @param portName Names this port in the diagnostics line, for boards with several
     */
    void begin(HardwareSerial& serial, int rxPin, int txPin, uint32_t baud, const char* portName);

    /**
     * @brief Release the UART and its pins; update() does nothing afterwards.
     */
    void end();
    
    /**
     * @brief Process incoming serial data from UART.
     */
    void update();

    /**
     * @brief Encode a single character (used for testing or custom streams).
     * @param c Character to encode
     * @return true if a complete sentence was parsed
     */
    bool encode(char c);

    /**
     * @brief Number of NMEA sentences that passed their checksum since begin().
     */
    uint32_t validSentenceCount();
    
    /**
     * @brief Get the latest fix data.
     */
    GpsFix getFix();

    // A fix older than this no longer describes where the player is.
    static constexpr uint32_t FIX_MAX_AGE_MS = 5000;

private:
    // Often enough to watch a fix arrive on the serial monitor without flooding it.
    static constexpr uint32_t DIAGNOSTICS_INTERVAL_MS = 5000;

    static uint32_t boardMillis();

    Clock _clock;
    TinyGPSPlus _gps;
    HardwareSerial* _serial = nullptr;
    const char* _portName = "";
    uint32_t _lastDiag;
    uint32_t _fixSentencesSeen = 0;
    uint32_t _lastFixMs = 0;
};

#endif // GPS_DRIVER_H
