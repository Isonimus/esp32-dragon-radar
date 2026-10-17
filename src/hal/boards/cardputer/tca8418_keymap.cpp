#include "tca8418_keymap.h"

// Event byte layout (TCA8418 datasheet, "Key Event Table"):
// bit 7 = pressed, bits 0-6 = key number, numbered row * 10 + col + 1.
static constexpr uint8_t EVENT_PRESSED_BIT = 0x80;
static constexpr uint8_t EVENT_KEY_MASK = 0x7F;
static constexpr uint8_t KEY_NUMBERS_PER_ROW = 10;

// The ADV uses 7 controller rows and 8 controller columns.
static constexpr uint8_t MATRIX_ROWS = 7;
static constexpr uint8_t MATRIX_COLS = 8;

// Each controller row feeds two adjacent printed columns: controller columns
// 0-3 are the four printed rows of the left one, 4-7 those of the right one.
static constexpr uint8_t PRINTED_ROWS = 4;
static constexpr uint8_t PRINTED_COLUMNS_PER_CONTROLLER_ROW = MATRIX_COLS / PRINTED_ROWS;

bool decodeTca8418Event(uint8_t eventCode, CardputerKeyEvent& event) {
    uint8_t keyNumber = eventCode & EVENT_KEY_MASK;
    if (keyNumber == 0) return false;

    uint8_t matrixRow = (keyNumber - 1) / KEY_NUMBERS_PER_ROW;
    uint8_t matrixCol = (keyNumber - 1) % KEY_NUMBERS_PER_ROW;
    if (matrixRow >= MATRIX_ROWS || matrixCol >= MATRIX_COLS) return false;

    event.pressed = (eventCode & EVENT_PRESSED_BIT) != 0;
    event.key.row = matrixCol % PRINTED_ROWS;
    event.key.col = matrixRow * PRINTED_COLUMNS_PER_CONTROLLER_ROW + matrixCol / PRINTED_ROWS;
    return true;
}
