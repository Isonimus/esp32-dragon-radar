#ifndef TONE_SEQUENCER_H
#define TONE_SEQUENCER_H

#include "melodies.h"

/**
 * @brief Decides which single tone should sound at a given moment.
 *
 * Pure timing and priority rules shared by every board with sound; a board
 * only turns the returned frequency into a buzzer or speaker signal. It does
 * not block, so the game keeps drawing while a melody plays.
 */
class ToneSequencer {
public:
    /** @brief Starts the radar beep, unless a melody or another beep is playing. */
    void playRadarBeep(uint32_t nowMs);

    /** @brief Starts a melody, cutting off whatever was playing. */
    void playMelody(const Melody& melody, uint32_t nowMs);

    void stop();

    /** @brief True while a melody (not the radar beep) is playing. */
    bool isMelodyPlaying() const;

    /** @brief Frequency to sound now, in Hz; 0 means silence. Advances playback. */
    uint16_t frequencyAt(uint32_t nowMs);

private:
    void start(const Melody& melody, uint32_t nowMs, bool isBeep);

    const Melody* _melody = nullptr;
    bool _isBeep = false;
    size_t _noteIndex = 0;
    uint32_t _noteStartMs = 0;
};

#endif // TONE_SEQUENCER_H
