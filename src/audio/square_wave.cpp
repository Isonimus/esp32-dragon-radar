#include "square_wave.h"

// The phase is a fraction of one period scaled to the full uint32_t range,
// so it wraps at exactly one period with no modulo.
static const uint32_t HALF_PERIOD_PHASE = 0x80000000u;
static const unsigned PHASE_BITS = 32;

SquareWave::SquareWave(uint32_t sampleRateHz, int16_t amplitude)
    : _sampleRateHz(sampleRateHz), _amplitude(amplitude) {}

void SquareWave::setFrequency(uint16_t frequencyHz) {
    _phaseStep = static_cast<uint32_t>((static_cast<uint64_t>(frequencyHz) << PHASE_BITS) / _sampleRateHz);
}

void SquareWave::fill(int16_t* stereoFrames, size_t frames) {
    for (size_t frame = 0; frame < frames; frame++) {
        int16_t sample = 0;
        if (_phaseStep != 0) {
            sample = (_phase < HALF_PERIOD_PHASE) ? _amplitude : static_cast<int16_t>(-_amplitude);
            _phase += _phaseStep;
        }
        stereoFrames[2 * frame] = sample;
        stereoFrames[2 * frame + 1] = sample;
    }
}
