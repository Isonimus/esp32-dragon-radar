#include "custom_display.h"

bool CustomDisplay::init() {
    if (!_tft.init()) return false;
    _tft.setRotation(0);      // Round display — rotation 0 keeps it centered
    _tft.setBrightness(255);
    _tft.fillScreen(TFT_BLACK);
    return true;
}

void CustomDisplay::setBrightness(uint8_t brightness) {
    // GC9A01 backlight is hardwired to 3.3V — no PWM control.
    // Method is a no-op but satisfies the interface.
    (void)brightness;
}

lgfx::LGFX_Device& CustomDisplay::getDevice() {
    return _tft;
}

int CustomDisplay::width() {
    return _tft.width();
}

int CustomDisplay::height() {
    return _tft.height();
}

bool CustomDisplay::isRound() {
    return true;
}
