#include "cardputer_display.h"

bool CardputerDisplay::init() {
    if (!_tft.init()) return false;
    _tft.setRotation(1);
    _tft.setBrightness(255);
    _tft.fillScreen(TFT_BLACK);
    return true;
}

void CardputerDisplay::setBrightness(uint8_t brightness) {
    _tft.setBrightness(brightness);
}

lgfx::LGFX_Device& CardputerDisplay::getDevice() {
    return _tft;
}

int CardputerDisplay::width() {
    return _tft.width();
}

int CardputerDisplay::height() {
    return _tft.height();
}

bool CardputerDisplay::isRound() {
    return false;
}
