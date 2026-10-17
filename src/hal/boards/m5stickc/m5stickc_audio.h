#ifndef M5STICKC_AUDIO_H
#define M5STICKC_AUDIO_H

#include "../../audio_hal.h"
#include "../../../audio/tone_sequencer.h"

/**
 * @brief M5StickC Plus 2 audio: the shared ToneSequencer drives the passive buzzer on GPIO 2.
 */
class M5StickCAudio : public AudioHAL {
public:
    bool begin() override;
    void update() override;
    void playRadarBeep() override;
    void playMenuTheme() override;
    void playCollectionFanfare() override;
    void playWinFanfare() override;
    void stopMelody() override;
    bool isMelodyPlaying() override;

private:
    static constexpr int BUZZER_PIN = 2;

    ToneSequencer _sequencer;
    uint16_t _soundingHz = 0;
};

#endif // M5STICKC_AUDIO_H
