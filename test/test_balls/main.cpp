#include <unity.h>
#include "../../src/game/ball_manager.h"
#include "../../src/game/ball_manager.cpp"
#include "../../src/math/coord_math.cpp"
#include "../mocks/fake_storage.h"

static const double ORIGIN_LAT = 40.4168;  // Madrid
static const double ORIGIN_LON = -3.7038;
static const double CAPTURE_RADIUS_M = 10.0;

// Collects the first uncollected ball by standing on it.
static uint8_t collectNextBall(BallManager& manager) {
    for (const Ball& ball : manager.getBalls()) {
        if (!ball.collected) return manager.checkProximity(ball.lat, ball.lon, CAPTURE_RADIUS_M);
    }
    return 0;
}

void test_ball_generation() {
    FakeStorage storage;
    BallManager manager(storage);
    manager.generate(40.4168, -3.7038); // Madrid
    
    const auto& balls = manager.getBalls();
    TEST_ASSERT_EQUAL(7, balls.size());
    TEST_ASSERT_EQUAL(7, manager.remainingCount());

    for (const auto& b : balls) {
        TEST_ASSERT_EQUAL_UINT8(false, b.collected);
        double dist = CoordMath::haversineDistance(40.4168, -3.7038, b.lat, b.lon);
        // Verify they are within 500-3000m
        TEST_ASSERT_TRUE(dist >= 490.0 && dist <= 3100.0);
    }
}

void test_ball_collection() {
    FakeStorage storage;
    BallManager manager(storage);
    manager.generate(0, 0);
    
    auto balls = manager.getBalls();
    double bLat = balls[0].lat;
    double bLon = balls[0].lon;
    
    // Check proximity - should collect ball 1
    uint8_t collectedId = manager.checkProximity(bLat, bLon, 10.0);
    TEST_ASSERT_EQUAL(balls[0].id, collectedId);
    TEST_ASSERT_EQUAL(6, manager.remainingCount());
    
    // Check again at same spot - should not collect anything else
    collectedId = manager.checkProximity(bLat, bLon, 10.0);
    TEST_ASSERT_EQUAL(0, collectedId);
}

void test_saved_game_survives_a_reboot() {
    FakeStorage storage;
    BallManager before(storage);
    before.generate(ORIGIN_LAT, ORIGIN_LON);
    collectNextBall(before);

    BallManager after(storage);
    TEST_ASSERT_TRUE(after.load());
    TEST_ASSERT_EQUAL_UINT32(before.getBalls().size(), after.getBalls().size());
    for (size_t i = 0; i < before.getBalls().size(); i++) {
        const Ball& expected = before.getBalls()[i];
        const Ball& actual = after.getBalls()[i];
        TEST_ASSERT_EQUAL_UINT8(expected.id, actual.id);
        TEST_ASSERT_EQUAL_DOUBLE(expected.lat, actual.lat);
        TEST_ASSERT_EQUAL_DOUBLE(expected.lon, actual.lon);
        TEST_ASSERT_EQUAL(expected.collected, actual.collected);
    }
}

void test_collecting_a_ball_is_saved_at_once() {
    FakeStorage storage;
    BallManager manager(storage);
    manager.generate(ORIGIN_LAT, ORIGIN_LON);
    uint8_t collectedId = collectNextBall(manager);

    TEST_ASSERT_TRUE(storage.saved[collectedId - 1].collected);
}

void test_load_without_a_save_offers_no_game() {
    FakeStorage storage;
    BallManager manager(storage);

    TEST_ASSERT_FALSE(manager.load());
    TEST_ASSERT_FALSE(manager.hasSavedGame());
}

void test_new_game_is_saved_at_once() {
    FakeStorage storage;
    BallManager manager(storage);
    TEST_ASSERT_FALSE(manager.hasSavedGame());

    manager.generate(ORIGIN_LAT, ORIGIN_LON);
    TEST_ASSERT_TRUE(manager.hasSavedGame());
    TEST_ASSERT_TRUE(storage.hasSave);
    TEST_ASSERT_EQUAL_UINT32(BallManager::BALL_COUNT, storage.saved.size());
}

void test_reset_deletes_the_saved_game() {
    FakeStorage storage;
    BallManager manager(storage);
    manager.generate(ORIGIN_LAT, ORIGIN_LON);
    TEST_ASSERT_TRUE(manager.hasSavedGame());

    manager.reset();
    TEST_ASSERT_FALSE(manager.hasSavedGame());
    TEST_ASSERT_FALSE(storage.hasSave);
    TEST_ASSERT_EQUAL_UINT32(0, manager.getBalls().size());
}

// Leaves a saved game in storage with this many balls collected, as a
// previous session would.
static void saveGameWithCollected(FakeStorage& storage, uint8_t collected) {
    BallManager previousSession(storage);
    previousSession.generate(ORIGIN_LAT, ORIGIN_LON);
    for (uint8_t i = 0; i < collected; i++) collectNextBall(previousSession);
}

void test_manager_without_balls_counts_none_collected() {
    FakeStorage storage;
    BallManager manager(storage);
    TEST_ASSERT_EQUAL_UINT8(0, manager.collectedCount());
}

// The menu shows "Continue (n/7)" from the manager, so it must never offer a
// saved game whose balls it has not loaded.
void test_saved_game_is_offered_only_once_loaded() {
    const uint8_t COLLECTED = 2;
    FakeStorage storage;
    saveGameWithCollected(storage, COLLECTED);

    BallManager manager(storage);
    TEST_ASSERT_FALSE(manager.hasSavedGame());

    TEST_ASSERT_TRUE(manager.load());
    TEST_ASSERT_TRUE(manager.hasSavedGame());
    TEST_ASSERT_EQUAL_UINT8(COLLECTED, manager.collectedCount());
}

// Power lost between the last catch and the win screen leaves a 7/7 save,
// which would continue onto a radar with nothing left to find.
void test_finished_save_is_discarded_on_load() {
    FakeStorage storage;
    saveGameWithCollected(storage, BallManager::BALL_COUNT);

    BallManager manager(storage);
    TEST_ASSERT_FALSE(manager.load());
    TEST_ASSERT_FALSE(manager.hasSavedGame());
    TEST_ASSERT_FALSE(storage.hasSave);
}

// A save from a corrupt write or another firmware would print "Continue (9/7)".
void test_save_with_the_wrong_ball_count_is_discarded_on_load() {
    FakeStorage storage;
    saveGameWithCollected(storage, 1);
    storage.saved.pop_back();

    BallManager manager(storage);
    TEST_ASSERT_FALSE(manager.load());
    TEST_ASSERT_FALSE(manager.hasSavedGame());
    TEST_ASSERT_FALSE(storage.hasSave);
}

// New Game keeps the old save until the new balls are scattered, so a reboot
// before then can still continue it; that is only safe if scattering replaces
// every ball of the old game.
void test_new_game_replaces_the_previous_save_entirely() {
    FakeStorage storage;
    saveGameWithCollected(storage, BallManager::BALL_COUNT - 1);

    BallManager manager(storage);
    TEST_ASSERT_TRUE(manager.load());
    manager.generate(ORIGIN_LAT, ORIGIN_LON);

    BallManager afterReboot(storage);
    TEST_ASSERT_TRUE(afterReboot.load());
    TEST_ASSERT_EQUAL_UINT8(0, afterReboot.collectedCount());
    TEST_ASSERT_EQUAL_UINT32(BallManager::BALL_COUNT, afterReboot.getBalls().size());
    for (size_t i = 0; i < manager.getBalls().size(); i++) {
        TEST_ASSERT_EQUAL_DOUBLE(manager.getBalls()[i].lat, afterReboot.getBalls()[i].lat);
        TEST_ASSERT_EQUAL_DOUBLE(manager.getBalls()[i].lon, afterReboot.getBalls()[i].lon);
    }
}

void test_game_saved_normally_is_not_flagged_unsaved() {
    FakeStorage storage;
    BallManager manager(storage);
    manager.generate(ORIGIN_LAT, ORIGIN_LON);
    collectNextBall(manager);
    TEST_ASSERT_FALSE(manager.progressUnsaved());
}

void test_failed_save_of_a_new_game_is_flagged() {
    FakeStorage storage;
    storage.failWrites = true;
    BallManager manager(storage);
    manager.generate(ORIGIN_LAT, ORIGIN_LON);
    TEST_ASSERT_TRUE(manager.progressUnsaved());
}

void test_failed_save_of_a_catch_is_flagged() {
    FakeStorage storage;
    BallManager manager(storage);
    manager.generate(ORIGIN_LAT, ORIGIN_LON);

    storage.failWrites = true;
    collectNextBall(manager);
    TEST_ASSERT_TRUE(manager.progressUnsaved());
}

// Every save writes the whole game, so one that succeeds also writes whatever
// the failed one lost, and the warning can go.
void test_next_successful_save_clears_the_flag_and_catches_up() {
    FakeStorage storage;
    BallManager manager(storage);
    manager.generate(ORIGIN_LAT, ORIGIN_LON);

    storage.failWrites = true;
    collectNextBall(manager);
    storage.failWrites = false;
    collectNextBall(manager);

    TEST_ASSERT_FALSE(manager.progressUnsaved());
    BallManager afterReboot(storage);
    TEST_ASSERT_TRUE(afterReboot.load());
    TEST_ASSERT_EQUAL_UINT8(2, afterReboot.collectedCount());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_ball_generation);
    RUN_TEST(test_ball_collection);
    RUN_TEST(test_saved_game_survives_a_reboot);
    RUN_TEST(test_collecting_a_ball_is_saved_at_once);
    RUN_TEST(test_load_without_a_save_offers_no_game);
    RUN_TEST(test_new_game_is_saved_at_once);
    RUN_TEST(test_reset_deletes_the_saved_game);
    RUN_TEST(test_manager_without_balls_counts_none_collected);
    RUN_TEST(test_saved_game_is_offered_only_once_loaded);
    RUN_TEST(test_finished_save_is_discarded_on_load);
    RUN_TEST(test_save_with_the_wrong_ball_count_is_discarded_on_load);
    RUN_TEST(test_new_game_replaces_the_previous_save_entirely);
    RUN_TEST(test_game_saved_normally_is_not_flagged_unsaved);
    RUN_TEST(test_failed_save_of_a_new_game_is_flagged);
    RUN_TEST(test_failed_save_of_a_catch_is_flagged);
    RUN_TEST(test_next_successful_save_clears_the_flag_and_catches_up);
    return UNITY_END();
}
