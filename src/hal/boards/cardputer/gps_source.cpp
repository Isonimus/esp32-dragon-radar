#include "gps_source.h"

int chooseGpsSource(const uint32_t validSentenceCounts[], size_t candidateCount) {
    for (size_t port = 0; port < candidateCount; ++port) {
        if (validSentenceCounts[port] > 0) {
            return static_cast<int>(port);
        }
    }
    return NO_GPS_SOURCE;
}
