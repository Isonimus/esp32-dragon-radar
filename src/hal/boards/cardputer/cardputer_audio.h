#ifndef CARDPUTER_AUDIO_H
#define CARDPUTER_AUDIO_H

#include <atomic>
#include <ESP_I2S.h>
#include "../../audio_hal.h"
#include "../../../audio/square_wave.h"
#include "../../../audio/tone_sequencer.h"

/**
 * @brief Cardputer ADV audio: square-wave tones over I2S into the ES8311 codec.
 *
 * The game loop runs the shared ToneSequencer and publishes the frequency to
 * sound; a background task keeps the I2S DMA fed, so the speaker never starves
 * while the loop is busy drawing.
 */
class CardputerAudio : public AudioHAL {
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
    static void synthTask(void* self);

    // Pin roles per M5Unified's Cardputer speaker config (identical on the ADV).
    static constexpr int I2S_BCK_PIN  = 41;
    static constexpr int I2S_LRCK_PIN = 43;
    static constexpr int I2S_DATA_PIN = 42;

    // 48 kHz with 16-bit stereo slots is what M5Unified feeds this codec, so
    // the ES8311 clock setup can be copied from it unchanged.
    static constexpr uint32_t SAMPLE_RATE_HZ = 48000;
    // A square wave is rich in harmonics; full scale is harsh on the small
    // speaker. One eighth of full scale is a starting point to tune by ear.
    static constexpr int16_t AMPLITUDE = 32767 / 8;
    // 5 ms per write: short enough that note changes land on time, long
    // enough that the task wakes rarely.
    static constexpr size_t FRAMES_PER_WRITE = 240;
    static constexpr uint32_t SYNTH_TASK_STACK_BYTES = 4096;
    // Above the Arduino loop (priority 1) so a slow frame cannot starve the DMA.
    static constexpr UBaseType_t SYNTH_TASK_PRIORITY = 2;

    I2SClass _i2s;
    ToneSequencer _sequencer;
    SquareWave _wave{SAMPLE_RATE_HZ, AMPLITUDE};
    std::atomic<uint16_t> _targetHz{0};
    int16_t _frames[2 * FRAMES_PER_WRITE];
};

#endif // CARDPUTER_AUDIO_H
