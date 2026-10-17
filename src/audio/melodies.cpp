#include "melodies.h"

#define MELODY_LENGTH(notes) (sizeof(notes) / sizeof((notes)[0]))

static const uint8_t FULL_LEGATO = 100;
// Separates repeated notes in the fanfares without shortening them audibly.
static const uint8_t DETACHED_LEGATO = 90;

static const uint16_t REST = 0;

// Two rising chirps, like the radar in the show.
static const Note RADAR_BEEP_NOTES[] = {
    {4100, 80}, {REST, 30}, {4800, 40},
};

// Ascending arpeggio: C5, E5, G5, C6.
static const Note COLLECTION_FANFARE_NOTES[] = {
    {523, 110}, {659, 110}, {784, 110}, {1047, 330},
};

static const Note WIN_FANFARE_NOTES[] = {
    {523, 150}, {523, 150}, {523, 150}, {523, 450}, {415, 450},
    {466, 450}, {523, 150}, {466, 150}, {523, 600},
};

// Dragon Ball main theme, lead melody, loosely arranged from a MIDI transcription
// (channels 1-2) that is not kept in the repo.
// The 120 ms run after the first long rest is the channel-2 fill, C#5 B4 A4 C#5
// as in that MIDI (it was C#5 A#4 F#4 C#5, which sounded flat on the device).
static const Note MENU_THEME_NOTES[] = {
    {494, 239}, {466, 119}, {370, 179}, {REST, 800}, {466, 239}, {554, 239},
    {554, 239}, {494, 119}, {466, 239}, {REST, 800}, {466, 239}, {554, 239},
    {554, 239}, {494, 119}, {740, 2999}, {REST, 121}, {370, 119}, {415, 119},
    {440, 839}, {494, 879}, {REST, 161}, {554, 399}, {587, 359}, {REST, 800},
    {587, 239}, {659, 119}, {659, 359}, {587, 359}, {REST, 800}, {554, 120},
    {494, 120}, {440, 120}, {554, 239}, {REST, 800}, {466, 239}, {REST, 800},
    {466, 239}, {REST, 241}, {494, 239}, {466, 119}, {370, 179}, {REST, 800},
    {466, 239}, {554, 119}, {554, 359}, {494, 119}, {466, 239}, {REST, 800},
    {466, 239}, {554, 239}, {554, 239}, {494, 119}, {740, 2999}, {REST, 121},
    {370, 119}, {415, 119}, {440, 839}, {494, 879}, {REST, 161}, {554, 399},
    {587, 359}, {REST, 800}, {659, 239}, {587, 119}, {587, 359}, {554, 239},
    {REST, 800}, {554, 2399},
};

namespace melodies {
const Melody RADAR_BEEP = {RADAR_BEEP_NOTES, MELODY_LENGTH(RADAR_BEEP_NOTES), FULL_LEGATO};
const Melody MENU_THEME = {MENU_THEME_NOTES, MELODY_LENGTH(MENU_THEME_NOTES), FULL_LEGATO};
const Melody COLLECTION_FANFARE = {COLLECTION_FANFARE_NOTES, MELODY_LENGTH(COLLECTION_FANFARE_NOTES), DETACHED_LEGATO};
const Melody WIN_FANFARE = {WIN_FANFARE_NOTES, MELODY_LENGTH(WIN_FANFARE_NOTES), DETACHED_LEGATO};
}
