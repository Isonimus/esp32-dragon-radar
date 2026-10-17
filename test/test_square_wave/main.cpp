#include <unity.h>
#include "../../src/audio/square_wave.h"
#include "../../src/audio/square_wave.cpp"

static const uint32_t SAMPLE_RATE_HZ = 48000;
static const int16_t AMPLITUDE = 4000;
static const size_t ONE_SECOND = SAMPLE_RATE_HZ;

static int16_t buffer[2 * ONE_SECOND];

static size_t countRisingEdges(const int16_t* stereoFrames, size_t frames) {
    size_t edges = 0;
    for (size_t frame = 1; frame < frames; frame++) {
        if (stereoFrames[2 * (frame - 1)] < 0 && stereoFrames[2 * frame] > 0) edges++;
    }
    return edges;
}

void test_zero_frequency_is_silence() {
    SquareWave wave(SAMPLE_RATE_HZ, AMPLITUDE);
    wave.setFrequency(0);
    wave.fill(buffer, ONE_SECOND);
    for (size_t i = 0; i < 2 * ONE_SECOND; i++) TEST_ASSERT_EQUAL_INT16(0, buffer[i]);
}

void test_one_second_holds_one_cycle_per_hertz() {
    // 4100 Hz does not divide 48 kHz evenly, so this also checks that the
    // rounding of the per-sample step does not drift the pitch.
    const uint16_t frequencyHz = 4100;
    SquareWave wave(SAMPLE_RATE_HZ, AMPLITUDE);
    wave.setFrequency(frequencyHz);
    wave.fill(buffer, ONE_SECOND);
    TEST_ASSERT_UINT_WITHIN(1, frequencyHz, countRisingEdges(buffer, ONE_SECOND));
}

void test_samples_swing_between_plus_and_minus_amplitude() {
    SquareWave wave(SAMPLE_RATE_HZ, AMPLITUDE);
    wave.setFrequency(523);
    wave.fill(buffer, ONE_SECOND);
    for (size_t i = 0; i < 2 * ONE_SECOND; i++) {
        TEST_ASSERT_TRUE(buffer[i] == AMPLITUDE || buffer[i] == -AMPLITUDE);
    }
}

void test_left_and_right_channels_match() {
    SquareWave wave(SAMPLE_RATE_HZ, AMPLITUDE);
    wave.setFrequency(1047);
    wave.fill(buffer, ONE_SECOND);
    for (size_t frame = 0; frame < ONE_SECOND; frame++) {
        TEST_ASSERT_EQUAL_INT16(buffer[2 * frame], buffer[2 * frame + 1]);
    }
}

void test_split_fills_join_without_a_phase_jump() {
    const uint16_t frequencyHz = 659;
    const size_t firstChunk = 1001;
    static int16_t whole[2 * ONE_SECOND];

    SquareWave continuous(SAMPLE_RATE_HZ, AMPLITUDE);
    continuous.setFrequency(frequencyHz);
    continuous.fill(whole, ONE_SECOND);

    SquareWave chunked(SAMPLE_RATE_HZ, AMPLITUDE);
    chunked.setFrequency(frequencyHz);
    chunked.fill(buffer, firstChunk);
    chunked.fill(buffer + 2 * firstChunk, ONE_SECOND - firstChunk);

    TEST_ASSERT_EQUAL_INT16_ARRAY(whole, buffer, 2 * ONE_SECOND);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_zero_frequency_is_silence);
    RUN_TEST(test_one_second_holds_one_cycle_per_hertz);
    RUN_TEST(test_samples_swing_between_plus_and_minus_amplitude);
    RUN_TEST(test_left_and_right_channels_match);
    RUN_TEST(test_split_fills_join_without_a_phase_jump);
    return UNITY_END();
}
