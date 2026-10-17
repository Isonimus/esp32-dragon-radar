#include "../../hal.h"
#include "../../nvs_storage.h"
#include "custom_display.h"
#include "custom_input.h"
#include "custom_audio.h"
#include "custom_gps.h"
#include "custom_imu.h"
#include "custom_power.h"

static CustomDisplay display;
static CustomInput   input;
static CustomAudio   audio;
static CustomGps     gps;
static CustomImu     imu;
static CustomPower   power;
static NvsStorage storage;

static HardwareHAL hal {
    display, input, audio, gps, imu, power, storage
};

HardwareHAL& getHAL() {
    return hal;
}
