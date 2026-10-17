#include "ball_manager.h"
#include "../math/coord_math.h"
#include <cstdlib>
#include <ctime>

BallManager::BallManager(StorageHAL& storage) : _storage(storage) {
    _balls.reserve(BALL_COUNT);
}

void BallManager::generate(double originLat, double originLon, double maxDistMeters) {
    _balls.clear();
    
    // Seed random once if not done
    static bool seeded = false;
    if (!seeded) {
        std::srand(std::time(NULL));
        seeded = true;
    }

    double minDist = maxDistMeters * 0.15; // Proportional minimum

    for (uint8_t i = 1; i <= BALL_COUNT; ++i) {
        double bLat, bLon;
        CoordMath::randomPointInAnnulus(originLat, originLon, minDist, maxDistMeters, bLat, bLon);
        
        Ball b;
        b.id = i;
        b.lat = bLat;
        b.lon = bLon;
        b.collected = false;
        _balls.push_back(b);
    }
    save();
}

uint8_t BallManager::checkProximity(double playerLat, double playerLon, double captureRadius) {
    for (auto& ball : _balls) {
        if (!ball.collected) {
            double dist = CoordMath::haversineDistance(playerLat, playerLon, ball.lat, ball.lon);
            if (dist <= captureRadius) {
                ball.collected = true;
                save();
                return ball.id;
            }
        }
    }
    return 0;
}

void BallManager::save() {
    _progressUnsaved = !_storage.saveBalls(_balls);
}

bool BallManager::load() {
    if (!_storage.loadBalls(_balls)) return false;
    // Power lost between the last catch and the win screen leaves a finished
    // save, which would continue onto a radar with nothing left to find. A save
    // with the wrong number of balls is corrupt or from other firmware.
    if (_balls.size() != BALL_COUNT || remainingCount() == 0) {
        reset();
        return false;
    }
    return true;
}

void BallManager::reset() {
    _balls.clear();
    // A failed delete is logged by the storage but not flagged: reset() only
    // runs on a finished or corrupt save, and load() discards either again at
    // the next boot.
    _storage.clear();
}

uint8_t BallManager::remainingCount() const {
    uint8_t count = 0;
    for (const auto& ball : _balls) {
        if (!ball.collected) count++;
    }
    return count;
}

uint8_t BallManager::collectedCount() const {
    uint8_t count = 0;
    for (const auto& ball : _balls) {
        if (ball.collected) count++;
    }
    return count;
}
