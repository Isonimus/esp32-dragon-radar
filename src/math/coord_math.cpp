#include "coord_math.h"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CoordMath {

    const double EARTH_RADIUS = 6371000.0; // Meters

    static inline double toRadians(double degrees) {
        return degrees * (M_PI / 180.0);
    }

    static inline double toDegrees(double radians) {
        return radians * (180.0 / M_PI);
    }

    double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
        double dLat = toRadians(lat2 - lat1);
        double dLon = toRadians(lon2 - lon1);

        double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                   std::cos(toRadians(lat1)) * std::cos(toRadians(lat2)) *
                   std::sin(dLon / 2) * std::sin(dLon / 2);
        
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return EARTH_RADIUS * c;
    }

    double normalizeAngle(double angle) {
        double result = std::fmod(angle, 360.0);
        if (result < 0) {
            result += 360.0;
        }
        return result;
    }

    double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
        double lat1Rad = toRadians(lat1);
        double lat2Rad = toRadians(lat2);
        double dLon = toRadians(lon2 - lon1);

        double y = std::sin(dLon) * std::cos(lat2Rad);
        double x = std::cos(lat1Rad) * std::sin(lat2Rad) -
                   std::sin(lat1Rad) * std::cos(lat2Rad) * std::cos(dLon);
        
        double bearingRad = std::atan2(y, x);
        return normalizeAngle(toDegrees(bearingRad));
    }

    void destinationPoint(double lat, double lon, double bearing, double distance, double &outLat, double &outLon) {
        double latRad = toRadians(lat);
        double lonRad = toRadians(lon);
        double bearingRad = toRadians(bearing);
        double dR = distance / EARTH_RADIUS;

        double outLatRad = std::asin(std::sin(latRad) * std::cos(dR) +
                                    std::cos(latRad) * std::sin(dR) * std::cos(bearingRad));
        double outLonRad = lonRad + std::atan2(std::sin(bearingRad) * std::sin(dR) * std::cos(latRad),
                                               std::cos(dR) - std::sin(latRad) * std::sin(outLatRad));
        
        outLat = toDegrees(outLatRad);
        outLon = toDegrees(outLonRad);
    }

    void randomPointInAnnulus(double lat, double lon, double minDist, double maxDist, double &outLat, double &outLon) {
        // Uniform random distribution in an annulus
        // r = sqrt(random(R1^2, R2^2))
        double r1sq = minDist * minDist;
        double r2sq = maxDist * maxDist;
        double r = std::sqrt(r1sq + (double)std::rand() / RAND_MAX * (r2sq - r1sq));
        double bearing = (double)std::rand() / RAND_MAX * 360.0;
        
        destinationPoint(lat, lon, bearing, r, outLat, outLon);
    }

}
