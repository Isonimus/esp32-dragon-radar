#include <unity.h>
#include "../../src/game/menu_cursor.h"
#include "../../src/game/menu_cursor.cpp"

static const int ITEM_COUNT = 3;
static const int LAST_ITEM = ITEM_COUNT - 1;

void test_down_moves_to_the_next_item() {
    TEST_ASSERT_EQUAL_INT(1, moveMenuCursor(0, ITEM_COUNT, InputAction::DOWN));
}

void test_up_moves_to_the_previous_item() {
    TEST_ASSERT_EQUAL_INT(0, moveMenuCursor(1, ITEM_COUNT, InputAction::UP));
}

void test_down_from_the_last_item_wraps_to_the_first() {
    TEST_ASSERT_EQUAL_INT(0, moveMenuCursor(LAST_ITEM, ITEM_COUNT, InputAction::DOWN));
}

void test_up_from_the_first_item_wraps_to_the_last() {
    TEST_ASSERT_EQUAL_INT(LAST_ITEM, moveMenuCursor(0, ITEM_COUNT, InputAction::UP));
}

void test_secondary_steps_forward_for_boards_without_arrows() {
    TEST_ASSERT_EQUAL_INT(1, moveMenuCursor(0, ITEM_COUNT, InputAction::SECONDARY));
}

void test_other_actions_leave_the_cursor_alone() {
    const InputAction others[] = {InputAction::NONE, InputAction::PRIMARY,
                                  InputAction::LONG_PRIMARY, InputAction::LONG_SECONDARY};
    for (InputAction action : others) {
        TEST_ASSERT_EQUAL_INT(1, moveMenuCursor(1, ITEM_COUNT, action));
    }
}

void test_single_item_menu_stays_put() {
    TEST_ASSERT_EQUAL_INT(0, moveMenuCursor(0, 1, InputAction::UP));
    TEST_ASSERT_EQUAL_INT(0, moveMenuCursor(0, 1, InputAction::DOWN));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_down_moves_to_the_next_item);
    RUN_TEST(test_up_moves_to_the_previous_item);
    RUN_TEST(test_down_from_the_last_item_wraps_to_the_first);
    RUN_TEST(test_up_from_the_first_item_wraps_to_the_last);
    RUN_TEST(test_secondary_steps_forward_for_boards_without_arrows);
    RUN_TEST(test_other_actions_leave_the_cursor_alone);
    RUN_TEST(test_single_item_menu_stays_put);
    return UNITY_END();
}
