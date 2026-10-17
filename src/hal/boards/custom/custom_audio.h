#ifndef CUSTOM_AUDIO_H
#define CUSTOM_AUDIO_H

#include "../../audio_hal.h"

/**
 * @brief No-op audio stub for the custom board (no buzzer/speaker yet).
 */
class CustomAudio : public AudioHAL {
public:
    bool begin() override { return true; }
    void update() override {}
    void playRadarBeep() override {}
    void playMenuTheme() override {}
    void playCollectionFanfare() override {}
    void playWinFanfare() override {}
    void stopMelody() override {}
    bool isMelodyPlaying() override { return false; }
};

#endif // CUSTOM_AUDIO_H
