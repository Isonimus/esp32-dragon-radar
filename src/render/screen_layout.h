#ifndef SCREEN_LAYOUT_H
#define SCREEN_LAYOUT_H

/**
 * Where a screen may put its content (ADR-0014). Free of Arduino and LovyanGFX, so the
 * native tests run it.
 */
namespace ScreenLayout {

struct Rect {
    int x;
    int y;
    int width;
    int height;

    int right() const { return x + width; }
    int bottom() const { return y + height; }
    int centreX() const { return x + width / 2; }
};

// The whole panel when it is rectangular; on a round panel, the largest square whose
// pixels all lie inside the circle.
Rect safeArea(int width, int height, bool roundPanel);

// The game's font is LovyanGFX's fixed-width default, 6 px per character at text size 1.
constexpr int GLYPH_WIDTH_PX = 6;

// The most lines a screen wraps a text into; the tests hold ScreenText to it.
constexpr int MAX_WRAPPED_LINES = 3;

struct LineSpan {
    int start;
    int length;
};

// Breaks `text` at spaces into lines of at most maxChars characters, writing up to
// maxLines spans into `lines`, and returns how many it wrote; text past the last line is
// dropped. A word longer than maxChars gets a line of its own and overflows it. The tests
// hold the game's own strings to fit, in lines and in width (ScreenText).
int wrapWords(const char* text, int maxChars, LineSpan* lines, int maxLines);

}  // namespace ScreenLayout

#endif  // SCREEN_LAYOUT_H
