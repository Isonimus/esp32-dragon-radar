#include "nvs_storage.h"
#include <Arduino.h>
#include <Preferences.h>

// The namespace and key names are the saved-game format (ADR-0007): renaming
// one orphans every save already on a device.
static const char* const NAMESPACE = "dragon_radar";
// Settings live apart, so deleting the saved game leaves them alone.
static const char* const SETTINGS_NAMESPACE = "dr_settings";
static const char* const DISTANCE_KEY = "distIdx";
static const char* const SOUND_KEY = "soundOn";
static const char* const COUNT_KEY = "count";
static const char* const COLLECTED_KEY_FORMAT = "b%u";
static const char* const LAT_KEY_FORMAT = "lat%u";
static const char* const LON_KEY_FORMAT = "lon%u";
// Room for the longest key, "lat255", and its terminator.
static constexpr size_t KEY_BUFFER_SIZE = 8;

struct BallKeys {
    char collected[KEY_BUFFER_SIZE];
    char lat[KEY_BUFFER_SIZE];
    char lon[KEY_BUFFER_SIZE];

    explicit BallKeys(unsigned id) {
        snprintf(collected, sizeof(collected), COLLECTED_KEY_FORMAT, id);
        snprintf(lat, sizeof(lat), LAT_KEY_FORMAT, id);
        snprintf(lon, sizeof(lon), LON_KEY_FORMAT, id);
    }
};

bool NvsStorage::loadBalls(std::vector<Ball>& balls) {
    Preferences prefs;
    // Read-only begin fails when the namespace was never written: no save yet.
    if (!prefs.begin(NAMESPACE, true)) return false;

    uint8_t count = prefs.getUChar(COUNT_KEY, 0);
    std::vector<Ball> loaded;
    loaded.reserve(count);
    for (unsigned id = 1; id <= count; ++id) {
        BallKeys keys(id);
        if (!prefs.isKey(keys.collected) || !prefs.isKey(keys.lat) || !prefs.isKey(keys.lon)) {
            Serial.printf("[save] ball %u is missing from the saved game; ignoring the save\n", id);
            prefs.end();
            return false;
        }
        Ball ball;
        ball.id = id;
        ball.collected = prefs.getBool(keys.collected);
        ball.lat = prefs.getDouble(keys.lat);
        ball.lon = prefs.getDouble(keys.lon);
        loaded.push_back(ball);
    }
    prefs.end();
    if (loaded.empty()) return false;
    balls = loaded;
    return true;
}

bool NvsStorage::saveBalls(const std::vector<Ball>& balls) {
    Preferences prefs;
    if (!prefs.begin(NAMESPACE, false)) {
        Serial.println("[save] FAILED to open NVS; progress is not saved");
        return false;
    }
    bool ok = prefs.putUChar(COUNT_KEY, balls.size()) > 0;
    for (const Ball& ball : balls) {
        BallKeys keys(ball.id);
        ok = prefs.putBool(keys.collected, ball.collected) > 0 && ok;
        ok = prefs.putDouble(keys.lat, ball.lat) > 0 && ok;
        ok = prefs.putDouble(keys.lon, ball.lon) > 0 && ok;
    }
    prefs.end();
    if (!ok) Serial.println("[save] FAILED to write the saved game; progress is not saved");
    return ok;
}

bool NvsStorage::clear() {
    Preferences prefs;
    bool ok = prefs.begin(NAMESPACE, false) && prefs.clear();
    prefs.end();
    if (!ok) Serial.println("[save] FAILED to delete the saved game");
    return ok;
}

bool NvsStorage::loadSettings(GameSettings& settings) {
    Preferences prefs;
    // Read-only begin fails when the namespace was never written: no settings yet.
    if (!prefs.begin(SETTINGS_NAMESPACE, true)) return false;
    bool complete = prefs.isKey(DISTANCE_KEY) && prefs.isKey(SOUND_KEY);
    if (complete) {
        settings.distancePresetIdx = prefs.getUChar(DISTANCE_KEY);
        settings.soundOn = prefs.getBool(SOUND_KEY);
    } else {
        Serial.println("[save] saved settings are incomplete; using defaults");
    }
    prefs.end();
    return complete;
}

bool NvsStorage::saveSettings(const GameSettings& settings) {
    Preferences prefs;
    if (!prefs.begin(SETTINGS_NAMESPACE, false)) {
        Serial.println("[save] FAILED to open NVS; settings are not saved");
        return false;
    }
    bool ok = prefs.putUChar(DISTANCE_KEY, settings.distancePresetIdx) > 0;
    ok = prefs.putBool(SOUND_KEY, settings.soundOn) > 0 && ok;
    prefs.end();
    if (!ok) Serial.println("[save] FAILED to write the settings");
    return ok;
}
