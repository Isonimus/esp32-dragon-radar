#ifndef SQUARE_WAVE_H
#define SQUARE_WAVE_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Generates a square wave as 16-bit stereo PCM.
 *
 * A square wave is what the M5StickC's passive buzzer produces, so boards
 * with a speaker sound like the same game. The phase carries over between
 * buffers and frequency changes, so consecutive fills join without a click.
 */
class SquareWave {
public:
    SquareWave(uint32_t sampleRateHz, int16_t amplitude);

    /** @brief 0 Hz outputs silence. */
    void setFrequency(uint16_t frequencyHz);

    /** @brief Writes `frames` interleaved left/right sample pairs. */
    void fill(int16_t* stereoFrames, size_t frames);

private:
    const uint32_t _sampleRateHz;
    const int16_t _amplitude;
    uint32_t _phase = 0;
    uint32_t _phaseStep = 0;
};

#endif // SQUARE_WAVE_H
