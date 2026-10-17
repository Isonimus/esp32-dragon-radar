#ifndef RADAR_HEADING_H
#define RADAR_HEADING_H

#include "../gps/gps_driver.h"

/**
 * @brief Chooses which way "up" points on the radar.
 *
 * GPS course over ground is only meaningful while moving, and a compass
 * only exists on some boards, so neither source is enough on its own:
 *   moving         → GPS course (direction of travel)
 *   standing still → compass heading if the board has one,
 *                    otherwise the last course seen while moving,
 *                    so pausing does not spin the radar back to north.
 */
class RadarHeading {
public:
    /**
     * @brief Heading in degrees [0, 360) to rotate the radar by this frame.
     * @param fix               Latest GPS fix.
     * @param compassAvailable  True if the board has a working heading sensor.
     * @param compassHeadingDeg Heading from that sensor; ignored when unavailable.
     */
    float select(const GpsFix& fix, bool compassAvailable, float compassHeadingDeg);

private:
    // Below roughly a walking pace, GPS course is dominated by position jitter.
    static constexpr float MIN_COURSE_SPEED_MPS = 1.0f;

    // North-up until the first reliable course arrives.
    float _lastCourseDeg = 0.0f;
};

#endif // RADAR_HEADING_H
