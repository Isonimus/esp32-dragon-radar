#include <unity.h>
#include "../../src/hal/boards/cardputer/gps_source.h"
#include "../../src/hal/boards/cardputer/gps_source.cpp"

static const size_t PORT_COUNT = 2;

void setUp() {}
void tearDown() {}

void test_no_source_while_no_port_has_parsed_a_sentence() {
    const uint32_t counts[PORT_COUNT] = {0, 0};
    TEST_ASSERT_EQUAL_INT(NO_GPS_SOURCE, chooseGpsSource(counts, PORT_COUNT));
}

void test_picks_the_only_port_with_valid_sentences() {
    const uint32_t counts[PORT_COUNT] = {0, 3};
    TEST_ASSERT_EQUAL_INT(1, chooseGpsSource(counts, PORT_COUNT));
}

void test_first_port_wins_when_both_have_valid_sentences() {
    const uint32_t counts[PORT_COUNT] = {1, 5};
    TEST_ASSERT_EQUAL_INT(0, chooseGpsSource(counts, PORT_COUNT));
}

void test_no_source_when_there_are_no_candidates() {
    const uint32_t counts[PORT_COUNT] = {7, 7};
    TEST_ASSERT_EQUAL_INT(NO_GPS_SOURCE, chooseGpsSource(counts, 0));
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_no_source_while_no_port_has_parsed_a_sentence);
    RUN_TEST(test_picks_the_only_port_with_valid_sentences);
    RUN_TEST(test_first_port_wins_when_both_have_valid_sentences);
    RUN_TEST(test_no_source_when_there_are_no_candidates);
    return UNITY_END();
}
