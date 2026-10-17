#include <unity.h>
#include <vector>
#include "../../src/hal/single_button_gesture.h"
#include "../../src/hal/single_button_gesture.cpp"

static const uint32_t DEBOUNCE_MS = SingleButtonGesture::DEBOUNCE_MS;
static const uint32_t LONG_PRESS_MS = SingleButtonGesture::LONG_PRESS_MS;
static const uint32_t DOUBLE_TAP_MS = SingleButtonGesture::DOUBLE_TAP_MS;
// Well clear of every threshold in both directions.
static const uint32_t QUICK_PRESS_MS = 100;
static const uint32_t SETTLE_MS = DOUBLE_TAP_MS + LONG_PRESS_MS;
static const uint32_t CONTACT_BOUNCE_MS = 10;
static const uint32_t BOUNCE_TOGGLE_MS = 1;

struct Fired {
    uint32_t atMs;
    InputAction action;
};

// Feeds the gesture one sample per millisecond, as the main loop roughly does,
// and records every action it returns.
struct Button {
    SingleButtonGesture gesture;
    uint32_t nowMs;
    std::vector<Fired> fired;

    explicit Button(uint32_t startMs = 0) : nowMs(startMs) {}

    void hold(bool pressed, uint32_t durationMs) {
        for (uint32_t elapsed = 0; elapsed < durationMs; elapsed++) {
            InputAction action = gesture.update(pressed, nowMs);
            if (action != InputAction::NONE) fired.push_back({nowMs, action});
            nowMs++;
        }
    }
    void press(uint32_t durationMs) { hold(true, durationMs); }
    void release(uint32_t durationMs) { hold(false, durationMs); }

    // Contact chatter: the level flips every millisecond for the bounce period.
    void bounce() {
        for (uint32_t elapsed = 0; elapsed < CONTACT_BOUNCE_MS; elapsed += BOUNCE_TOGGLE_MS) {
            hold(elapsed % 2 == 0, BOUNCE_TOGGLE_MS);
        }
    }
};

static void assertActions(const std::vector<InputAction>& expected, const Button& button) {
    TEST_ASSERT_EQUAL_UINT32(expected.size(), button.fired.size());
    for (size_t i = 0; i < expected.size(); i++) {
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected[i]),
                                static_cast<uint8_t>(button.fired[i].action));
    }
}

void test_idle_button_produces_nothing() {
    Button button;
    button.release(SETTLE_MS);
    assertActions({}, button);
}

void test_tap_fires_primary_only_after_the_double_tap_window() {
    Button button;
    button.press(QUICK_PRESS_MS);
    uint32_t releasedAtMs = button.nowMs;
    button.release(DOUBLE_TAP_MS);
    assertActions({}, button);

    button.release(SETTLE_MS);
    assertActions({InputAction::PRIMARY}, button);
    TEST_ASSERT_EQUAL_UINT32(releasedAtMs + DEBOUNCE_MS + DOUBLE_TAP_MS, button.fired[0].atMs);
}

void test_double_tap_fires_secondary_on_the_second_press() {
    Button button;
    button.press(QUICK_PRESS_MS);
    button.release(QUICK_PRESS_MS);
    button.press(QUICK_PRESS_MS);
    assertActions({InputAction::SECONDARY}, button);

    // Releasing the second press must not add a tap.
    button.release(SETTLE_MS);
    assertActions({InputAction::SECONDARY}, button);
}

void test_second_press_after_the_window_is_two_taps() {
    Button button;
    button.press(QUICK_PRESS_MS);
    button.release(SETTLE_MS);
    button.press(QUICK_PRESS_MS);
    button.release(SETTLE_MS);
    assertActions({InputAction::PRIMARY, InputAction::PRIMARY}, button);
}

void test_hold_fires_long_primary_once_while_held() {
    Button button;
    uint32_t pressedAtMs = button.nowMs;
    button.press(LONG_PRESS_MS * 3);
    assertActions({InputAction::LONG_PRIMARY}, button);
    TEST_ASSERT_EQUAL_UINT32(pressedAtMs + DEBOUNCE_MS + LONG_PRESS_MS, button.fired[0].atMs);

    // Letting go after a long press is neither a tap nor half a double tap.
    button.release(SETTLE_MS);
    assertActions({InputAction::LONG_PRIMARY}, button);
}

void test_release_just_before_long_press_is_a_tap() {
    Button button;
    button.press(DEBOUNCE_MS + LONG_PRESS_MS - 1);
    button.release(SETTLE_MS);
    assertActions({InputAction::PRIMARY}, button);
}

void test_tap_after_long_press_is_recognised() {
    Button button;
    button.press(LONG_PRESS_MS * 2);
    button.release(SETTLE_MS);
    button.press(QUICK_PRESS_MS);
    button.release(SETTLE_MS);
    assertActions({InputAction::LONG_PRIMARY, InputAction::PRIMARY}, button);
}

void test_contact_bounce_counts_as_one_tap_not_a_double() {
    Button button;
    button.bounce();
    button.press(QUICK_PRESS_MS);
    button.bounce();
    button.release(SETTLE_MS);
    assertActions({InputAction::PRIMARY}, button);
}

void test_glitch_shorter_than_debounce_is_ignored() {
    Button button;
    button.press(DEBOUNCE_MS - 1);
    button.release(SETTLE_MS);
    assertActions({}, button);
}

void test_tap_across_millis_wraparound_still_fires() {
    Button button(UINT32_MAX - QUICK_PRESS_MS / 2);
    button.press(QUICK_PRESS_MS);
    button.release(SETTLE_MS);
    assertActions({InputAction::PRIMARY}, button);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_idle_button_produces_nothing);
    RUN_TEST(test_tap_fires_primary_only_after_the_double_tap_window);
    RUN_TEST(test_double_tap_fires_secondary_on_the_second_press);
    RUN_TEST(test_second_press_after_the_window_is_two_taps);
    RUN_TEST(test_hold_fires_long_primary_once_while_held);
    RUN_TEST(test_release_just_before_long_press_is_a_tap);
    RUN_TEST(test_tap_after_long_press_is_recognised);
    RUN_TEST(test_contact_bounce_counts_as_one_tap_not_a_double);
    RUN_TEST(test_glitch_shorter_than_debounce_is_ignored);
    RUN_TEST(test_tap_across_millis_wraparound_still_fires);
    return UNITY_END();
}
