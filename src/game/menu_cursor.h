#ifndef MENU_CURSOR_H
#define MENU_CURSOR_H

#include "../hal/input_hal.h"

/**
 * @brief Where a menu cursor moves for an input action, wrapping at both ends.
 *
 * SECONDARY steps forward as DOWN does, because the M5StickC has no arrows
 * and its second button is the only way to move. Other actions leave it put.
 */
int moveMenuCursor(int cursor, int itemCount, InputAction action);

#endif // MENU_CURSOR_H
