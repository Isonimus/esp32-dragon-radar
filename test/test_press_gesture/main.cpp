#include <unity.h>
#include "../../src/hal/press_gesture.h"
#include "../../src/hal/press_gesture.cpp"

static const uint32_t LONG_PRESS_MS = 1000;
static const uint32_t SHORT_HOLD_MS = 100;

static PressGesture makeGesture() {
    return PressGesture(InputAction::PRIMARY, InputAction::LONG_PRIMARY, LONG_PRESS_MS);
}

static void assertAction(InputAction expected, InputAction actual) {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected), static_cast<uint8_t>(actual));
}

void test_short_press_taps_on_release_not_on_press() {
    PressGesture gesture = makeGesture();
    assertAction(InputAction::NONE, gesture.update(true, 0));
    assertAction(InputAction::NONE, gesture.update(true, SHORT_HOLD_MS));
    assertAction(InputAction::PRIMARY, gesture.update(false, SHORT_HOLD_MS + 1));
}

void test_idle_key_produces_nothing() {
    PressGesture gesture = makeGesture();
    assertAction(InputAction::NONE, gesture.update(false, 0));
    assertAction(InputAction::NONE, gesture.update(false, LONG_PRESS_MS * 2));
}

void test_hold_past_threshold_fires_long_press_while_held() {
    PressGesture gesture = makeGesture();
    gesture.update(true, 0);
    assertAction(InputAction::LONG_PRIMARY, gesture.update(true, LONG_PRESS_MS + 1));
}

void test_hold_exactly_at_threshold_is_not_yet_long() {
    PressGesture gesture = makeGesture();
    gesture.update(true, 0);
    assertAction(InputAction::NONE, gesture.update(true, LONG_PRESS_MS));
}

void test_long_press_fires_once_and_release_does_not_tap() {
    PressGesture gesture = makeGesture();
    gesture.update(true, 0);
    gesture.update(true, LONG_PRESS_MS + 1);
    assertAction(InputAction::NONE, gesture.update(true, LONG_PRESS_MS * 3));
    assertAction(InputAction::NONE, gesture.update(false, LONG_PRESS_MS * 3 + 1));
}

void test_next_press_after_long_press_can_tap() {
    PressGesture gesture = makeGesture();
    gesture.update(true, 0);
    gesture.update(true, LONG_PRESS_MS + 1);
    gesture.update(false, LONG_PRESS_MS + 2);
    gesture.update(true, LONG_PRESS_MS * 2);
    assertAction(InputAction::PRIMARY, gesture.update(false, LONG_PRESS_MS * 2 + SHORT_HOLD_MS));
}

void test_short_press_across_millis_wraparound_still_taps() {
    // millis() wraps after ~49.7 days; unsigned subtraction must keep a short hold short.
    PressGesture gesture = makeGesture();
    const uint32_t justBeforeWrap = UINT32_MAX - SHORT_HOLD_MS / 2;
    gesture.update(true, justBeforeWrap);
    assertAction(InputAction::NONE, gesture.update(true, justBeforeWrap + SHORT_HOLD_MS));
    assertAction(InputAction::PRIMARY, gesture.update(false, justBeforeWrap + SHORT_HOLD_MS + 1));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_short_press_taps_on_release_not_on_press);
    RUN_TEST(test_idle_key_produces_nothing);
    RUN_TEST(test_hold_past_threshold_fires_long_press_while_held);
    RUN_TEST(test_hold_exactly_at_threshold_is_not_yet_long);
    RUN_TEST(test_long_press_fires_once_and_release_does_not_tap);
    RUN_TEST(test_next_press_after_long_press_can_tap);
    RUN_TEST(test_short_press_across_millis_wraparound_still_taps);
    return UNITY_END();
}
