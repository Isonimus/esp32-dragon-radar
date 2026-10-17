#include "menu_cursor.h"

int moveMenuCursor(int cursor, int itemCount, InputAction action) {
    switch (action) {
        case InputAction::UP:
            return (cursor + itemCount - 1) % itemCount;
        case InputAction::DOWN:
        case InputAction::SECONDARY:
            return (cursor + 1) % itemCount;
        default:
            return cursor;
    }
}
