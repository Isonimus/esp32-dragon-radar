#include "imu_driver.h"

#define MPU6886_ADDR 0x68

ImuDriver::ImuDriver() {}

bool ImuDriver::begin() {
    Wire.begin(21, 22); // SDA, SCL for M5StickC
    
    // Check WHO_AM_I
    if (readReg(0x75) != 0x19) {
        Serial.println("MPU6886 not found!");
        return false;
    }
    
    // Wake up
    writeReg(0x6B, 0x00);
    delay(10);
    
    _lastUpdate = millis();
    return true;
}

void ImuDriver::update() {
    uint32_t now = millis();
    float dt = (now - _lastUpdate) / 1000.0f;
    _lastUpdate = now;
    
    // Read Gyro Z (Registers 0x47-0x48)
    Wire.beginTransmission(MPU6886_ADDR);
    Wire.write(0x47);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6886_ADDR, 2);
    
    if (Wire.available() == 2) {
        int16_t gz = (Wire.read() << 8) | Wire.read();
        float gz_deg = gz / 131.0f; // Default 250dps scale
        
        // Simple integration (will drift, but good for local orientation)
        _heading -= gz_deg * dt; 
        if (_heading >= 360.0f) _heading -= 360.0f;
        if (_heading < 0.0f) _heading += 360.0f;
    }
}

void ImuDriver::writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MPU6886_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

uint8_t ImuDriver::readReg(uint8_t reg) {
    Wire.beginTransmission(MPU6886_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6886_ADDR, 1);
    return Wire.available() ? Wire.read() : 0;
}

