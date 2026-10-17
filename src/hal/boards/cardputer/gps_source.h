#ifndef GPS_SOURCE_H
#define GPS_SOURCE_H

#include <stddef.h>
#include <stdint.h>

// Returned while no candidate port has delivered a checksum-valid NMEA sentence.
constexpr int NO_GPS_SOURCE = -1;

/**
 * @brief Picks the GPS port to keep: the first, in the order given, that has parsed at
 * least one checksum-valid NMEA sentence (ADR-0005).
 *
 * Valid sentences rather than received bytes, because an unconnected RX pin can pick up
 * noise, and noise never passes the NMEA checksum.
 */
int chooseGpsSource(const uint32_t validSentenceCounts[], size_t candidateCount);

#endif // GPS_SOURCE_H
