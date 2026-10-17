#include "Arduino.h"
#include <chrono>
#include <thread>

// millis() is provided by TinyGPS++ if ARDUINO is not defined

void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
