#include <unity.h>
#include "../../src/hal/boards/cardputer/tca8418_keymap.h"
#include "../../src/hal/boards/cardputer/tca8418_keymap.cpp"

// Facts about the hardware, stated independently of the decoder's constants.
static const uint8_t PRESSED_BIT = 0x80;
static const uint8_t WIRED_ROWS = 7;
static const uint8_t WIRED_COLS = 8;
static const uint8_t LAYOUT_ROWS = 4;
static const uint8_t LAYOUT_COLS = 14;
static const uint8_t FIRST_GPI_EVENT = 97;  // TCA8418 datasheet: GPI events start at 97

static uint8_t keyNumber(uint8_t matrixRow, uint8_t matrixCol) {
    return matrixRow * 10 + matrixCol + 1;
}

void test_empty_fifo_entry_is_rejected() {
    CardputerKeyEvent event;
    TEST_ASSERT_FALSE(decodeTca8418Event(0x00, event));
    TEST_ASSERT_FALSE(decodeTca8418Event(PRESSED_BIT, event));
}

void test_gpi_event_is_rejected() {
    CardputerKeyEvent event;
    TEST_ASSERT_FALSE(decodeTca8418Event(FIRST_GPI_EVENT | PRESSED_BIT, event));
}

void test_unwired_matrix_column_is_rejected() {
    CardputerKeyEvent event;
    TEST_ASSERT_FALSE(decodeTca8418Event(keyNumber(0, WIRED_COLS) | PRESSED_BIT, event));
}

void test_unwired_matrix_row_is_rejected() {
    CardputerKeyEvent event;
    TEST_ASSERT_FALSE(decodeTca8418Event(keyNumber(WIRED_ROWS, 0) | PRESSED_BIT, event));
}

void test_pressed_bit_sets_pressed_and_clear_bit_means_released() {
    CardputerKeyEvent event;
    TEST_ASSERT_TRUE(decodeTca8418Event(keyNumber(3, 2) | PRESSED_BIT, event));
    TEST_ASSERT_TRUE(event.pressed);
    TEST_ASSERT_TRUE(decodeTca8418Event(keyNumber(3, 2), event));
    TEST_ASSERT_FALSE(event.pressed);
}

void test_every_wired_key_lands_on_a_distinct_printed_key() {
    bool seen[LAYOUT_ROWS][LAYOUT_COLS] = {};
    for (uint8_t row = 0; row < WIRED_ROWS; row++) {
        for (uint8_t col = 0; col < WIRED_COLS; col++) {
            CardputerKeyEvent event;
            TEST_ASSERT_TRUE(decodeTca8418Event(keyNumber(row, col) | PRESSED_BIT, event));
            TEST_ASSERT_LESS_THAN_UINT8(LAYOUT_ROWS, event.key.row);
            TEST_ASSERT_LESS_THAN_UINT8(LAYOUT_COLS, event.key.col);
            TEST_ASSERT_FALSE_MESSAGE(seen[event.key.row][event.key.col], "two matrix keys share a printed key");
            seen[event.key.row][event.key.col] = true;
        }
    }
}

// Enter and Backspace are the keys the game binds. Their matrix positions come
// from M5Stack's reference driver and must be confirmed on the device.
void test_enter_is_matrix_row_6_col_6() {
    CardputerKeyEvent event;
    TEST_ASSERT_TRUE(decodeTca8418Event(keyNumber(6, 6) | PRESSED_BIT, event));
    TEST_ASSERT_TRUE(event.key == cardputer_keys::ENTER);
}

void test_backspace_is_matrix_row_6_col_4() {
    CardputerKeyEvent event;
    TEST_ASSERT_TRUE(decodeTca8418Event(keyNumber(6, 4) | PRESSED_BIT, event));
    TEST_ASSERT_TRUE(event.key == cardputer_keys::BACKSPACE);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_empty_fifo_entry_is_rejected);
    RUN_TEST(test_gpi_event_is_rejected);
    RUN_TEST(test_unwired_matrix_column_is_rejected);
    RUN_TEST(test_unwired_matrix_row_is_rejected);
    RUN_TEST(test_pressed_bit_sets_pressed_and_clear_bit_means_released);
    RUN_TEST(test_every_wired_key_lands_on_a_distinct_printed_key);
    RUN_TEST(test_enter_is_matrix_row_6_col_6);
    RUN_TEST(test_backspace_is_matrix_row_6_col_4);
    return UNITY_END();
}
