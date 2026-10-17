#include <unity.h>
#include "../../src/game/radar_heading.h"
#include "../../src/game/radar_heading.cpp"

static const float WALKING_SPEED_MPS = 1.5f;
static const float STANDING_SPEED_MPS = 0.0f;
static const float COMPASS_HEADING_DEG = 270.0f;

static GpsFix makeFix(float speedMps, float courseDeg) {
    GpsFix fix = {};
    fix.speed = speedMps;
    fix.course = courseDeg;
    fix.hasPosition = true;
    fix.current = true;
    return fix;
}

void test_moving_without_compass_follows_gps_course() {
    RadarHeading heading;
    TEST_ASSERT_EQUAL_FLOAT(90.0f, heading.select(makeFix(WALKING_SPEED_MPS, 90.0f), false, 0.0f));
}

void test_moving_with_compass_follows_gps_course() {
    RadarHeading heading;
    TEST_ASSERT_EQUAL_FLOAT(90.0f,
        heading.select(makeFix(WALKING_SPEED_MPS, 90.0f), true, COMPASS_HEADING_DEG));
}

void test_standing_with_compass_follows_compass() {
    RadarHeading heading;
    TEST_ASSERT_EQUAL_FLOAT(COMPASS_HEADING_DEG,
        heading.select(makeFix(STANDING_SPEED_MPS, 90.0f), true, COMPASS_HEADING_DEG));
}

void test_standing_without_compass_holds_last_course() {
    RadarHeading heading;
    heading.select(makeFix(WALKING_SPEED_MPS, 135.0f), false, 0.0f);
    // A stationary receiver reports a meaningless course; it must not leak through.
    TEST_ASSERT_EQUAL_FLOAT(135.0f, heading.select(makeFix(STANDING_SPEED_MPS, 10.0f), false, 0.0f));
}

void test_never_moved_without_compass_is_north_up() {
    RadarHeading heading;
    TEST_ASSERT_EQUAL_FLOAT(0.0f, heading.select(makeFix(STANDING_SPEED_MPS, 200.0f), false, 0.0f));
}

void test_course_without_a_position_is_ignored() {
    RadarHeading heading;
    GpsFix noPosition = makeFix(WALKING_SPEED_MPS, 90.0f);
    noPosition.hasPosition = false;
    noPosition.current = false;
    TEST_ASSERT_EQUAL_FLOAT(COMPASS_HEADING_DEG, heading.select(noPosition, true, COMPASS_HEADING_DEG));
}

void test_course_of_a_stale_fix_is_ignored() {
    RadarHeading heading;
    // The receiver lost its fix while walking; its last course no longer says where the player faces.
    GpsFix stale = makeFix(WALKING_SPEED_MPS, 90.0f);
    stale.current = false;
    TEST_ASSERT_EQUAL_FLOAT(COMPASS_HEADING_DEG, heading.select(stale, true, COMPASS_HEADING_DEG));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_moving_without_compass_follows_gps_course);
    RUN_TEST(test_moving_with_compass_follows_gps_course);
    RUN_TEST(test_standing_with_compass_follows_compass);
    RUN_TEST(test_standing_without_compass_holds_last_course);
    RUN_TEST(test_never_moved_without_compass_is_north_up);
    RUN_TEST(test_course_without_a_position_is_ignored);
    RUN_TEST(test_course_of_a_stale_fix_is_ignored);
    return UNITY_END();
}
