#include <unity.h>
#include "../../src/game/blip_blink.h"
#include "../../src/game/blip_blink.cpp"

static const uint32_t FRAME_MS = 50;

void setUp() {}
void tearDown() {}

void test_blips_are_lit_then_dark_within_each_period() {
    BlipBlink blink;
    uint32_t start = 10 * BLIP_PERIOD_MS;
    TEST_ASSERT_TRUE(blink.update(start).lit);
    TEST_ASSERT_TRUE(blink.update(start + BLIP_LIT_MS - 1).lit);
    TEST_ASSERT_FALSE(blink.update(start + BLIP_LIT_MS).lit);
    TEST_ASSERT_FALSE(blink.update(start + BLIP_PERIOD_MS - 1).lit);
    TEST_ASSERT_TRUE(blink.update(start + BLIP_PERIOD_MS).lit);
}

void test_one_onset_per_period_at_the_start_of_the_lit_phase() {
    BlipBlink blink;
    int onsets = 0;
    const int periods = 5;
    for (uint32_t now = 0; now < periods * BLIP_PERIOD_MS; now += FRAME_MS) {
        BlinkState state = blink.update(now);
        if (state.onset) {
            onsets++;
            TEST_ASSERT_TRUE(state.lit);
            TEST_ASSERT_EQUAL_UINT32(0, now % BLIP_PERIOD_MS);
        }
    }
    TEST_ASSERT_EQUAL_INT(periods, onsets);
}

void test_no_onset_inside_a_phase() {
    BlipBlink blink;
    TEST_ASSERT_TRUE(blink.update(BLIP_PERIOD_MS).onset);
    TEST_ASSERT_FALSE(blink.update(BLIP_PERIOD_MS + FRAME_MS).onset);
    TEST_ASSERT_FALSE(blink.update(BLIP_PERIOD_MS + BLIP_LIT_MS + FRAME_MS).onset);
}

void test_first_update_mid_lit_phase_is_an_onset() {
    // The radar was entered with the blips already lit: they appear, so it beeps.
    BlipBlink blink;
    BlinkState state = blink.update(3 * BLIP_PERIOD_MS + BLIP_LIT_MS / 2);
    TEST_ASSERT_TRUE(state.lit);
    TEST_ASSERT_TRUE(state.onset);
    TEST_ASSERT_FALSE(blink.update(3 * BLIP_PERIOD_MS + BLIP_LIT_MS / 2 + FRAME_MS).onset);
}

void test_first_update_mid_dark_phase_waits_for_the_next_lit_phase() {
    BlipBlink blink;
    BlinkState state = blink.update(3 * BLIP_PERIOD_MS + BLIP_LIT_MS + FRAME_MS);
    TEST_ASSERT_FALSE(state.lit);
    TEST_ASSERT_FALSE(state.onset);
    TEST_ASSERT_TRUE(blink.update(4 * BLIP_PERIOD_MS).onset);
}

void test_a_skipped_lit_phase_start_still_beeps_once() {
    // A long frame (a zoom change reshades the background) can step over the lit
    // phase's first moment; the onset comes with the next frame instead.
    BlipBlink blink;
    blink.update(BLIP_PERIOD_MS - FRAME_MS);
    TEST_ASSERT_TRUE(blink.update(BLIP_PERIOD_MS + 136).onset);
    TEST_ASSERT_FALSE(blink.update(BLIP_PERIOD_MS + 136 + FRAME_MS).onset);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_blips_are_lit_then_dark_within_each_period);
    RUN_TEST(test_one_onset_per_period_at_the_start_of_the_lit_phase);
    RUN_TEST(test_no_onset_inside_a_phase);
    RUN_TEST(test_first_update_mid_lit_phase_is_an_onset);
    RUN_TEST(test_first_update_mid_dark_phase_waits_for_the_next_lit_phase);
    RUN_TEST(test_a_skipped_lit_phase_start_still_beeps_once);
    return UNITY_END();
}
