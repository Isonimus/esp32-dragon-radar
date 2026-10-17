#include "custom_imu.h"

bool CustomImu::begin() {
    _ok = _compass.begin(SDA_PIN, SCL_PIN);
    return _ok;
}

void CustomImu::update() {
    if (_ok) _compass.update();
}

float CustomImu::getHeading() {
    return _compass.getHeading();
}

bool CustomImu::available() {
    return _ok;
}
