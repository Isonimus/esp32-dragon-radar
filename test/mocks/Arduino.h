#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

/**
 * Minimal Arduino shim for native testing
 */

typedef uint8_t byte;

#define TWO_PI 6.283185307179586476925286766559
#define PI 3.1415926535897932384626433832795
#define radians(x) ((x)*0.017453292519943295769236907684886)
#define degrees(x) ((x)*57.295779513082320876798154814105)
#define sq(x) ((x)*(x))

// millis() is provided by TinyGPS++ for non-Arduino envs
unsigned long millis(); 
void delay(unsigned long ms);

#define SERIAL_8N1 0x800001c

/**
 * Minimal HardwareSerial stub for native testing.
 * Never actually called — GPS tests use encode() directly.
 */
class HardwareSerial {
public:
    void begin(unsigned long baud, uint32_t config = 0, int rxPin = -1, int txPin = -1) {
        (void)baud; (void)config; (void)rxPin; (void)txPin;
    }
    int available() { return 0; }
    int read() { return -1; }
};

#endif // ARDUINO_H
