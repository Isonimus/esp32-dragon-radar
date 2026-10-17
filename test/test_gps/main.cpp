#define UNIT_TEST
#include <unity.h>
#include <chrono>                   // Needed for TinyGPS++ native millis()
#include "../../src/gps/gps_driver.h"
#include "../../src/gps/gps_driver.cpp" // Include implementation for native test linking
#include "../../.pio/libdeps/native/TinyGPSPlus/src/TinyGPS++.cpp" // Include library implementation
#include "../mocks/Arduino.cpp"        // Include mock implementation

// Status A: the receiver has a fix, at about 48.1173 N, 11.5167 E.
static const char* const RMC_WITH_FIX =
    "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n";
// Status V: the receiver has lost its fix, as a module does under a roof.
static const char* const RMC_WITHOUT_FIX =
    "$GPRMC,123520,V,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*77\r\n";
static const uint32_t FIRST_FIX_AT_MS = 1000;

static uint32_t fakeNowMs = 0;

static uint32_t fakeClock() {
    return fakeNowMs;
}

static void feed(GpsDriver& driver, const char* sentence) {
    for (int i = 0; sentence[i] != '\0'; i++) {
        driver.encode(sentence[i]);
    }
}

void setUp() {
    fakeNowMs = FIRST_FIX_AT_MS;
}
void tearDown() {}

void test_no_position_before_any_fix() {
    GpsDriver driver(fakeClock);
    GpsFix fix = driver.getFix();
    TEST_ASSERT_FALSE(fix.hasPosition);
    TEST_ASSERT_FALSE(fix.current);
    TEST_ASSERT_EQUAL_INT(0, fix.satellites);
}

void test_parsed_fix_is_current_with_its_position() {
    GpsDriver driver(fakeClock);
    feed(driver, RMC_WITH_FIX);

    GpsFix fix = driver.getFix();
    TEST_ASSERT_TRUE(fix.hasPosition);
    TEST_ASSERT_TRUE(fix.current);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 48.1173, fix.lat);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 11.5167, fix.lon);
}

void test_fix_is_still_current_at_max_age() {
    GpsDriver driver(fakeClock);
    feed(driver, RMC_WITH_FIX);
    fakeNowMs = FIRST_FIX_AT_MS + GpsDriver::FIX_MAX_AGE_MS;

    GpsFix fix = driver.getFix();
    TEST_ASSERT_TRUE(fix.current);
    TEST_ASSERT_EQUAL_UINT32(GpsDriver::FIX_MAX_AGE_MS, fix.fixAge);
}

void test_fix_past_max_age_keeps_position_but_is_not_current() {
    GpsDriver driver(fakeClock);
    feed(driver, RMC_WITH_FIX);
    fakeNowMs = FIRST_FIX_AT_MS + GpsDriver::FIX_MAX_AGE_MS + 1;

    GpsFix fix = driver.getFix();
    TEST_ASSERT_TRUE(fix.hasPosition);
    TEST_ASSERT_FALSE(fix.current);
    TEST_ASSERT_EQUAL_UINT32(GpsDriver::FIX_MAX_AGE_MS + 1, fix.fixAge);
}

void test_sentence_without_fix_does_not_refresh_a_stale_fix() {
    GpsDriver driver(fakeClock);
    feed(driver, RMC_WITH_FIX);
    fakeNowMs = FIRST_FIX_AT_MS + GpsDriver::FIX_MAX_AGE_MS + 1;
    feed(driver, RMC_WITHOUT_FIX);

    TEST_ASSERT_FALSE(driver.getFix().current);
}

void test_new_fix_makes_a_stale_fix_current_again() {
    GpsDriver driver(fakeClock);
    feed(driver, RMC_WITH_FIX);
    fakeNowMs = FIRST_FIX_AT_MS + GpsDriver::FIX_MAX_AGE_MS + 1;
    feed(driver, RMC_WITH_FIX);

    GpsFix fix = driver.getFix();
    TEST_ASSERT_TRUE(fix.current);
    TEST_ASSERT_EQUAL_UINT32(0, fix.fixAge);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_no_position_before_any_fix);
    RUN_TEST(test_parsed_fix_is_current_with_its_position);
    RUN_TEST(test_fix_is_still_current_at_max_age);
    RUN_TEST(test_fix_past_max_age_keeps_position_but_is_not_current);
    RUN_TEST(test_sentence_without_fix_does_not_refresh_a_stale_fix);
    RUN_TEST(test_new_fix_makes_a_stale_fix_current_again);
    return UNITY_END();
}
