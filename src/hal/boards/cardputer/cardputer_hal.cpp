#include "../../hal.h"
#include "../../nvs_storage.h"
#include "cardputer_display.h"
#include "cardputer_input.h"
#include "cardputer_audio.h"
#include "cardputer_gps.h"
#include "cardputer_imu.h"
#include "cardputer_power.h"

static CardputerDisplay display;
static CardputerInput   input;
static CardputerAudio   audio;
static CardputerGps     gps;
static CardputerImu     imu;
static CardputerPower   power;
static NvsStorage storage;

static HardwareHAL hal {
    display, input, audio, gps, imu, power, storage
};

HardwareHAL& getHAL() {
    return hal;
}
