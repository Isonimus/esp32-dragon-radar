#ifndef STORAGE_HAL_H
#define STORAGE_HAL_H

#include <vector>
#include "../game/ball.h"
#include "../game/game_settings.h"

/**
 * @brief Persistent storage for the game in progress and the player's
 * settings (ADR-0007). Both survive a reboot; there is one saved game at most.
 */
class StorageHAL {
public:
    virtual ~StorageHAL() = default;

    /**
     * @brief Replace `balls` with the saved game.
     * @return false, leaving `balls` untouched, when there is no saved game
     *         or it cannot be read whole.
     */
    virtual bool loadBalls(std::vector<Ball>& balls) = 0;

    /** @brief Overwrite the saved game. @return false if the write failed. */
    virtual bool saveBalls(const std::vector<Ball>& balls) = 0;

    /** @brief Delete the saved game. @return false if it could not be deleted. */
    virtual bool clear() = 0;

    /**
     * @brief Read the saved settings.
     * @return false, leaving `settings` untouched, when none were saved.
     */
    virtual bool loadSettings(GameSettings& settings) = 0;

    /** @brief Overwrite the saved settings. @return false if the write failed. */
    virtual bool saveSettings(const GameSettings& settings) = 0;
};

#endif // STORAGE_HAL_H
