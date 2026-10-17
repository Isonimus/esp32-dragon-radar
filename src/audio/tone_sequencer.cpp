#include "tone_sequencer.h"

static const uint32_t PERCENT = 100;

void ToneSequencer::playRadarBeep(uint32_t nowMs) {
    // The beep repeats every radar sweep; it must never cut off a melody,
    // nor restart itself before it has finished.
    if (_melody != nullptr) return;
    start(melodies::RADAR_BEEP, nowMs, true);
}

void ToneSequencer::playMelody(const Melody& melody, uint32_t nowMs) {
    start(melody, nowMs, false);
}

void ToneSequencer::stop() {
    _melody = nullptr;
    _isBeep = false;
}

bool ToneSequencer::isMelodyPlaying() const {
    return _melody != nullptr && !_isBeep;
}

uint16_t ToneSequencer::frequencyAt(uint32_t nowMs) {
    if (_melody == nullptr) return 0;

    // Advance by each note's own duration rather than jumping to nowMs, so a
    // late update does not stretch the notes that follow.
    while (nowMs - _noteStartMs >= _melody->notes[_noteIndex].durationMs) {
        _noteStartMs += _melody->notes[_noteIndex].durationMs;
        _noteIndex++;
        if (_noteIndex >= _melody->length) {
            stop();
            return 0;
        }
    }

    const Note& note = _melody->notes[_noteIndex];
    const uint32_t soundingMs = note.durationMs * _melody->legatoPercent / PERCENT;
    return (nowMs - _noteStartMs < soundingMs) ? note.frequencyHz : 0;
}

void ToneSequencer::start(const Melody& melody, uint32_t nowMs, bool isBeep) {
    _melody = &melody;
    _isBeep = isBeep;
    _noteIndex = 0;
    _noteStartMs = nowMs;
}
