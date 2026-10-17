#include <unity.h>
#include "../../src/audio/melodies.h"
#include "../../src/audio/melodies.cpp"
#include "../../src/audio/tone_sequencer.h"
#include "../../src/audio/tone_sequencer.cpp"

static const uint32_t START_MS = 1000;
static const uint32_t LEGATO_FULL_SCALE = 100;

static uint32_t totalDurationMs(const Melody& melody) {
    uint32_t total = 0;
    for (size_t i = 0; i < melody.length; i++) total += melody.notes[i].durationMs;
    return total;
}

void test_idle_sequencer_is_silent() {
    ToneSequencer sequencer;
    TEST_ASSERT_EQUAL_UINT16(0, sequencer.frequencyAt(START_MS));
    TEST_ASSERT_FALSE(sequencer.isMelodyPlaying());
}

void test_radar_beep_follows_its_notes_then_falls_silent() {
    ToneSequencer sequencer;
    sequencer.playRadarBeep(START_MS);
    uint32_t noteStart = START_MS;
    for (size_t i = 0; i < melodies::RADAR_BEEP.length; i++) {
        const Note& note = melodies::RADAR_BEEP.notes[i];
        TEST_ASSERT_EQUAL_UINT16(note.frequencyHz, sequencer.frequencyAt(noteStart));
        TEST_ASSERT_EQUAL_UINT16(note.frequencyHz, sequencer.frequencyAt(noteStart + note.durationMs - 1));
        noteStart += note.durationMs;
    }
    TEST_ASSERT_EQUAL_UINT16(0, sequencer.frequencyAt(noteStart));
    TEST_ASSERT_FALSE(sequencer.isMelodyPlaying());
}

void test_radar_beep_is_not_a_melody() {
    ToneSequencer sequencer;
    sequencer.playRadarBeep(START_MS);
    TEST_ASSERT_FALSE(sequencer.isMelodyPlaying());
}

void test_radar_beep_does_not_interrupt_a_melody() {
    ToneSequencer sequencer;
    sequencer.playMelody(melodies::MENU_THEME, START_MS);
    sequencer.playRadarBeep(START_MS + 1);
    TEST_ASSERT_TRUE(sequencer.isMelodyPlaying());
    TEST_ASSERT_EQUAL_UINT16(melodies::MENU_THEME.notes[0].frequencyHz, sequencer.frequencyAt(START_MS + 1));
}

void test_radar_beep_does_not_restart_itself() {
    ToneSequencer sequencer;
    const uint32_t firstNoteMs = melodies::RADAR_BEEP.notes[0].durationMs;
    sequencer.playRadarBeep(START_MS);
    sequencer.playRadarBeep(START_MS + firstNoteMs);
    TEST_ASSERT_EQUAL_UINT16(melodies::RADAR_BEEP.notes[1].frequencyHz, sequencer.frequencyAt(START_MS + firstNoteMs));
}

void test_melody_cuts_off_another_melody() {
    ToneSequencer sequencer;
    sequencer.playMelody(melodies::MENU_THEME, START_MS);
    sequencer.playMelody(melodies::COLLECTION_FANFARE, START_MS + 5);
    TEST_ASSERT_EQUAL_UINT16(melodies::COLLECTION_FANFARE.notes[0].frequencyHz, sequencer.frequencyAt(START_MS + 5));
}

void test_stop_silences_a_melody() {
    ToneSequencer sequencer;
    sequencer.playMelody(melodies::MENU_THEME, START_MS);
    sequencer.stop();
    TEST_ASSERT_FALSE(sequencer.isMelodyPlaying());
    TEST_ASSERT_EQUAL_UINT16(0, sequencer.frequencyAt(START_MS));
}

void test_detached_melody_leaves_a_gap_before_the_next_note() {
    // The win fanfare opens with repeated C5s; without a gap they would merge.
    ToneSequencer sequencer;
    const Melody& fanfare = melodies::WIN_FANFARE;
    const uint32_t firstNoteMs = fanfare.notes[0].durationMs;
    const uint32_t soundingMs = firstNoteMs * fanfare.legatoPercent / LEGATO_FULL_SCALE;
    sequencer.playMelody(fanfare, START_MS);
    TEST_ASSERT_EQUAL_UINT16(fanfare.notes[0].frequencyHz, sequencer.frequencyAt(START_MS + soundingMs - 1));
    TEST_ASSERT_EQUAL_UINT16(0, sequencer.frequencyAt(START_MS + soundingMs));
    TEST_ASSERT_EQUAL_UINT16(fanfare.notes[1].frequencyHz, sequencer.frequencyAt(START_MS + firstNoteMs));
}

void test_late_update_does_not_shift_later_notes() {
    // A slow frame skips past note boundaries; the melody must stay on its
    // original timeline instead of restarting the clock at the late update.
    ToneSequencer sequencer;
    const Melody& fanfare = melodies::COLLECTION_FANFARE;
    const uint32_t thirdNoteStart = START_MS + fanfare.notes[0].durationMs + fanfare.notes[1].durationMs;
    sequencer.playMelody(fanfare, START_MS);
    TEST_ASSERT_EQUAL_UINT16(fanfare.notes[2].frequencyHz, sequencer.frequencyAt(thirdNoteStart + 1));
    TEST_ASSERT_EQUAL_UINT16(fanfare.notes[3].frequencyHz, sequencer.frequencyAt(thirdNoteStart + fanfare.notes[2].durationMs));
}

void test_melody_ends_after_its_total_duration() {
    ToneSequencer sequencer;
    const uint32_t total = totalDurationMs(melodies::COLLECTION_FANFARE);
    sequencer.playMelody(melodies::COLLECTION_FANFARE, START_MS);
    TEST_ASSERT_TRUE(sequencer.isMelodyPlaying());
    sequencer.frequencyAt(START_MS + total - 1);
    TEST_ASSERT_TRUE(sequencer.isMelodyPlaying());
    TEST_ASSERT_EQUAL_UINT16(0, sequencer.frequencyAt(START_MS + total));
    TEST_ASSERT_FALSE(sequencer.isMelodyPlaying());
}

void test_playback_survives_millis_wraparound() {
    ToneSequencer sequencer;
    const uint32_t justBeforeWrap = 0xFFFFFFFFu - 10;
    const Note& firstNote = melodies::RADAR_BEEP.notes[0];
    sequencer.playRadarBeep(justBeforeWrap);
    TEST_ASSERT_EQUAL_UINT16(firstNote.frequencyHz, sequencer.frequencyAt(justBeforeWrap + firstNote.durationMs - 1));
    TEST_ASSERT_EQUAL_UINT16(melodies::RADAR_BEEP.notes[1].frequencyHz, sequencer.frequencyAt(justBeforeWrap + firstNote.durationMs));
}

// The sequencer indexes the first note unconditionally, so an empty table
// would read out of bounds; the tables are data, so check them here.
void test_every_melody_has_notes_and_a_sounding_legato() {
    const Melody* all[] = {&melodies::RADAR_BEEP, &melodies::MENU_THEME,
                           &melodies::COLLECTION_FANFARE, &melodies::WIN_FANFARE};
    for (const Melody* melody : all) {
        TEST_ASSERT_GREATER_THAN(0, melody->length);
        TEST_ASSERT_GREATER_THAN_UINT8(0, melody->legatoPercent);
        TEST_ASSERT_LESS_OR_EQUAL_UINT8(LEGATO_FULL_SCALE, melody->legatoPercent);
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_idle_sequencer_is_silent);
    RUN_TEST(test_radar_beep_follows_its_notes_then_falls_silent);
    RUN_TEST(test_radar_beep_is_not_a_melody);
    RUN_TEST(test_radar_beep_does_not_interrupt_a_melody);
    RUN_TEST(test_radar_beep_does_not_restart_itself);
    RUN_TEST(test_melody_cuts_off_another_melody);
    RUN_TEST(test_stop_silences_a_melody);
    RUN_TEST(test_detached_melody_leaves_a_gap_before_the_next_note);
    RUN_TEST(test_late_update_does_not_shift_later_notes);
    RUN_TEST(test_melody_ends_after_its_total_duration);
    RUN_TEST(test_playback_survives_millis_wraparound);
    RUN_TEST(test_every_melody_has_notes_and_a_sounding_legato);
    return UNITY_END();
}
