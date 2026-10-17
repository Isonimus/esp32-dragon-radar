#include "radar_heading.h"

float RadarHeading::select(const GpsFix& fix, bool compassAvailable, float compassHeadingDeg) {
    bool courseReliable = fix.current && fix.speed > MIN_COURSE_SPEED_MPS;
    if (courseReliable) {
        _lastCourseDeg = fix.course;
        return fix.course;
    }
    if (compassAvailable) return compassHeadingDeg;
    return _lastCourseDeg;
}
