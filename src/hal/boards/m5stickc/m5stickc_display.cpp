#include "m5stickc_display.h"

bool M5StickCDisplay::init() {
    if (!_tft.init()) return false;
    _tft.setRotation(1);
    _tft.setBrightness(255);
    _tft.fillScreen(TFT_BLACK);
    return true;
}

void M5StickCDisplay::setBrightness(uint8_t brightness) {
    _tft.setBrightness(brightness);
}

lgfx::LGFX_Device& M5StickCDisplay::getDevice() {
    return _tft;
}

int M5StickCDisplay::width() {
    return _tft.width();
}

int M5StickCDisplay::height() {
    return _tft.height();
}

bool M5StickCDisplay::isRound() {
    return false;
}
