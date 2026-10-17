#ifndef TCA8418_KEYMAP_H
#define TCA8418_KEYMAP_H

#include <stdint.h>

/**
 * @brief A key on the Cardputer keyboard, addressed as printed:
 * 4 rows × 14 columns, row 0 = the number row, column 0 = leftmost.
 */
struct CardputerKey {
    uint8_t row;
    uint8_t col;
};

inline bool operator==(const CardputerKey& a, const CardputerKey& b) {
    return a.row == b.row && a.col == b.col;
}

namespace cardputer_keys {
constexpr CardputerKey BACKSPACE{0, 13};
constexpr CardputerKey ENTER{2, 13};
// The arrows are printed on ';' and '.'; this driver reads raw keys, so no Fn is needed.
constexpr CardputerKey UP{2, 11};
constexpr CardputerKey DOWN{3, 11};
} // namespace cardputer_keys

struct CardputerKeyEvent {
    CardputerKey key;
    bool pressed;
};

/**
 * @brief Decode one TCA8418 FIFO entry into a key on the printed layout.
 *
 * The ADV wires its 56 keys as a 7×8 controller matrix that does not match the
 * printed layout; the remap follows M5Stack's reference driver (M5Cardputer
 * library, TCA8418KeyboardReader::remap).
 *
 * @return false for anything that is not a key in that 7×8 matrix — an empty
 *         FIFO (0), a GPI event, or a matrix position no key is wired to.
 */
bool decodeTca8418Event(uint8_t eventCode, CardputerKeyEvent& event);

#endif // TCA8418_KEYMAP_H
