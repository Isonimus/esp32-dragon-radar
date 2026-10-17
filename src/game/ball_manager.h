#ifndef BALL_MANAGER_H
#define BALL_MANAGER_H

#include <vector>
#include <stdint.h>
#include "ball.h"
#include "../hal/storage_hal.h"

/**
 * @brief Manages Dragon Ball generation, tracking, and collection.
 *
 * Every change is saved as it happens, so the balls in memory always match the
 * saved game, unless a save failed (see progressUnsaved()), and the menu can
 * read progress from them without touching storage.
 */
class BallManager {
public:
    static constexpr uint8_t BALL_COUNT = 7;

    explicit BallManager(StorageHAL& storage);

    /**
     * @brief Start a new game: scatter BALL_COUNT balls around an origin and save them.
     * @param originLat Origin latitude
     * @param originLon Origin longitude
     */
    void generate(double originLat, double originLon, double maxDistMeters = 3000.0);

    /**
     * @brief Collect, and save, the first uncollected ball within reach.
     * @param playerLat Player latitude
     * @param playerLon Player longitude
     * @param captureRadius Distance in meters to trigger collection
     * @return Ball ID (1-7) if collected, 0 otherwise
     */
    uint8_t checkProximity(double playerLat, double playerLon, double captureRadius = 10.0);

    /**
     * @brief Get the list of all balls.
     */
    const std::vector<Ball>& getBalls() const { return _balls; }

    /**
     * @brief Drop the balls and delete the saved game.
     */
    void reset();

    /**
     * @brief Total number of uncollected balls.
     */
    uint8_t remainingCount() const;

    /**
     * @brief Replace the balls with the saved game; call once at boot.
     * A finished game (every ball collected) or one without exactly BALL_COUNT
     * balls is deleted rather than loaded.
     * @return true if there is a game in progress to continue.
     */
    bool load();

    /**
     * @brief True while a game is in progress: loaded at boot or started since.
     */
    bool hasSavedGame() const { return !_balls.empty(); }

    uint8_t collectedCount() const;

    /**
     * @brief True while the last save failed, so a reboot would lose progress.
     * Clears at the next save that succeeds, since every save writes the whole game.
     */
    bool progressUnsaved() const { return _progressUnsaved; }

private:
    void save();

    StorageHAL& _storage;
    std::vector<Ball> _balls;
    bool _progressUnsaved = false;
};

#endif // BALL_MANAGER_H
