#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include "storage_hal.h"

/**
 * @brief StorageHAL over the ESP32's NVS flash through Arduino's Preferences.
 * Every supported board is an ESP32, so all of them share this one (ADR-0007).
 */
class NvsStorage : public StorageHAL {
public:
    bool loadBalls(std::vector<Ball>& balls) override;
    bool saveBalls(const std::vector<Ball>& balls) override;
    bool clear() override;
    bool loadSettings(GameSettings& settings) override;
    bool saveSettings(const GameSettings& settings) override;
};

#endif // NVS_STORAGE_H
