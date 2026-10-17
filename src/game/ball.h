#ifndef BALL_H
#define BALL_H

#include <stdint.h>

/**
 * @brief Structure representing a Dragon Ball.
 */
struct Ball {
    uint8_t id;         // ID from 1 to 7
    double lat;         // Latitude in degrees
    double lon;         // Longitude in degrees
    bool collected;     // Collection status
};

#endif // BALL_H
