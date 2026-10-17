#include <unity.h>
#include "../../src/math/coord_math.h"
#include "../../src/math/coord_math.cpp"
#include <math.h>

void test_normalize_angle() {
    TEST_ASSERT_EQUAL_DOUBLE(0.0, CoordMath::normalizeAngle(0.0));
    TEST_ASSERT_EQUAL_DOUBLE(180.0, CoordMath::normalizeAngle(180.0));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, CoordMath::normalizeAngle(361.0));
    TEST_ASSERT_EQUAL_DOUBLE(359.0, CoordMath::normalizeAngle(-1.0));
    TEST_ASSERT_EQUAL_DOUBLE(0.0, CoordMath::normalizeAngle(360.0));
}

void test_distance_calculation() {
    // 0,0 to 0,0
    TEST_ASSERT_FLOAT_WITHIN(0.1, 0.0, CoordMath::haversineDistance(0, 0, 0, 0));
    
    // ~111km North (1 degree of latitude)
    // 1 degree lat is approx 111.1km
    TEST_ASSERT_FLOAT_WITHIN(100.0, 111100.0, CoordMath::haversineDistance(0, 0, 1, 0));
    
    // Madrid to Barcelona (approx 504km)
    // Madrid: 40.4168, -3.7038
    // Barcelona: 41.3851, 2.1734
    TEST_ASSERT_FLOAT_WITHIN(5000.0, 504000.0, CoordMath::haversineDistance(40.4168, -3.7038, 41.3851, 2.1734));
}

void test_bearing_calculation() {
    // North (0 to 1,0)
    TEST_ASSERT_FLOAT_WITHIN(0.1, 0.0, CoordMath::calculateBearing(0, 0, 1, 0));
    
    // East (0,0 to 0,1)
    TEST_ASSERT_FLOAT_WITHIN(0.1, 90.0, CoordMath::calculateBearing(0, 0, 0, 1));
    
    // South (1,0 to 0,0)
    TEST_ASSERT_FLOAT_WITHIN(0.1, 180.0, CoordMath::calculateBearing(1, 0, 0, 0));
    
    // West (0,1 to 0,0)
    TEST_ASSERT_FLOAT_WITHIN(0.1, 270.0, CoordMath::calculateBearing(0, 1, 0, 0));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_normalize_angle);
    RUN_TEST(test_distance_calculation);
    RUN_TEST(test_bearing_calculation);
    return UNITY_END();
}
