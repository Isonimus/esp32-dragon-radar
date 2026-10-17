#ifndef MELODIES_H
#define MELODIES_H

#include <stddef.h>
#include <stdint.h>

/** @brief One step of a melody; a frequency of 0 is a rest. */
struct Note {
    uint16_t frequencyHz;
    uint16_t durationMs;
};

/**
 * @brief A note sequence and how much of each step actually sounds.
 * A legato below 100 leaves a short silence at the end of every note, so
 * repeated pitches are heard as separate notes rather than one long tone.
 */
struct Melody {
    const Note* notes;
    size_t length;
    uint8_t legatoPercent;
};

// Shared by every board with sound, so the tunes are defined once.
namespace melodies {
extern const Melody RADAR_BEEP;
extern const Melody MENU_THEME;
extern const Melody COLLECTION_FANFARE;
extern const Melody WIN_FANFARE;
}

#endif // MELODIES_H
