#include "../../hal.h"
#include "../../nvs_storage.h"
#include "m5stickc_display.h"
#include "m5stickc_input.h"
#include "m5stickc_audio.h"
#include "m5stickc_gps.h"
#include "m5stickc_imu.h"
#include "m5stickc_power.h"

static M5StickCDisplay display;
static M5StickCInput   input;
static M5StickCAudio   audio;
static M5StickCGps     gps;
static M5StickCImu     imu;
static M5StickCPower   power;
static NvsStorage storage;

static HardwareHAL hal {
    display, input, audio, gps, imu, power, storage
};

HardwareHAL& getHAL() {
    return hal;
}
