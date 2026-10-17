#ifndef COORD_MATH_H
#define COORD_MATH_H

/**
 * @brief Utility for GPS coordinate mathematics.
 */
namespace CoordMath {

    /**
     * @brief Calculate Haversine distance between two points in meters.
     * @param lat1 Latitude of point 1 (degrees)
     * @param lon1 Longitude of point 1 (degrees)
     * @param lat2 Latitude of point 2 (degrees)
     * @param lon2 Longitude of point 2 (degrees)
     * @return distance in meters
     */
    double haversineDistance(double lat1, double lon1, double lat2, double lon2);

    /**
     * @brief Calculate initial bearing from point 1 to point 2.
     * @param lat1 Latitude of point 1 (degrees)
     * @param lon1 Longitude of point 1 (degrees)
     * @param lat2 Latitude of point 2 (degrees)
     * @param lon2 Longitude of point 2 (degrees)
     * @return bearing in degrees [0, 360) where 0 is North
     */
    double calculateBearing(double lat1, double lon1, double lat2, double lon2);

    /**
     * @brief Calculate destination point given origin, bearing, and distance.
     * @param lat Origin latitude (degrees)
     * @param lon Origin longitude (degrees)
     * @param bearing Bearing from origin (degrees)
     * @param distance Distance from origin (meters)
     * @param outLat Output latitude (degrees)
     * @param outLon Output longitude (degrees)
     */
    void destinationPoint(double lat, double lon, double bearing, double distance, double &outLat, double &outLon);

    /**
     * @brief Generate a random point between minDist and maxDist from origin.
     * @param lat Origin latitude (degrees)
     * @param lon Origin longitude (degrees)
     * @param minDist Minimum distance (meters)
     * @param maxDist Maximum distance (meters)
     * @param outLat Output latitude (degrees)
     * @param outLon Output longitude (degrees)
     */
    void randomPointInAnnulus(double lat, double lon, double minDist, double maxDist, double &outLat, double &outLon);

}

#endif // COORD_MATH_H
