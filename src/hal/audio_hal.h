#ifndef AUDIO_HAL_H
#define AUDIO_HAL_H

/**
 * @brief Abstract audio interface.
 * Covers both I2S speaker (Cardputer) and passive buzzer (M5StickC).
 * Boards without audio hardware provide a no-op stub.
 */
class AudioHAL {
public:
    virtual ~AudioHAL() = default;

    /** @brief False when the sound hardware did not start; true on boards that have none. */
    virtual bool begin() = 0;

    /** @brief Must be called every loop iteration for non-blocking playback. */
    virtual void update() = 0;

    virtual void playRadarBeep() = 0;
    virtual void playMenuTheme() = 0;
    virtual void playCollectionFanfare() = 0;
    virtual void playWinFanfare() = 0;
    virtual void stopMelody() = 0;
    virtual bool isMelodyPlaying() = 0;
};

#endif // AUDIO_HAL_H
