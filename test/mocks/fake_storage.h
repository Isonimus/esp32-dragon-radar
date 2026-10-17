#ifndef FAKE_STORAGE_H
#define FAKE_STORAGE_H

#include <vector>
#include "../../src/hal/storage_hal.h"

// In-memory StorageHAL: what the device would keep in flash across a reboot.
struct FakeStorage : public StorageHAL {
    bool hasSave = false;
    std::vector<Ball> saved;
    // Makes every write report failure and change nothing, like a full or worn flash.
    bool failWrites = false;

    bool loadBalls(std::vector<Ball>& balls) override {
        if (!hasSave) return false;
        balls = saved;
        return true;
    }
    bool saveBalls(const std::vector<Ball>& balls) override {
        if (failWrites) return false;
        saved = balls;
        hasSave = true;
        return true;
    }
    bool clear() override {
        if (failWrites) return false;
        saved.clear();
        hasSave = false;
        return true;
    }
    bool loadSettings(GameSettings&) override { return false; }
    bool saveSettings(const GameSettings&) override { return true; }
};

#endif // FAKE_STORAGE_H
