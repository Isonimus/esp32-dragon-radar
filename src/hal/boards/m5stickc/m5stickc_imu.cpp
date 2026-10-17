#include "m5stickc_imu.h"

bool M5StickCImu::begin() {
    _ok = _imu.begin();
    return _ok;
}

void M5StickCImu::update() {
    if (_ok) _imu.update();
}

float M5StickCImu::getHeading() {
    return _imu.getHeading();
}

bool M5StickCImu::available() {
    return _ok;
}
