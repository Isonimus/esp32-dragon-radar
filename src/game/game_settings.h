#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

#include <stdint.h>

/**
 * @brief The player's choices on the settings screen, kept across reboots.
 */
struct GameSettings {
    uint8_t distancePresetIdx;  // Index into the scatter-distance presets
    bool soundOn;
};

#endif // GAME_SETTINGS_H
