#include "screen_layout.h"

#include <math.h>

namespace ScreenLayout {

Rect safeArea(int width, int height, bool roundPanel) {
    if (!roundPanel) return {0, 0, width, height};
    // A square of half side R / sqrt(2) touches the circle at its corners. Flooring it
    // keeps the outer corner of the outermost pixel inside.
    const int radius = width / 2;
    const int halfSide = static_cast<int>(floorf(radius * static_cast<float>(M_SQRT1_2)));
    return {width / 2 - halfSide, height / 2 - halfSide, 2 * halfSide, 2 * halfSide};
}

int wrapWords(const char* text, int maxChars, LineSpan* lines, int maxLines) {
    int count = 0;
    int position = 0;
    while (text[position] != '\0' && count < maxLines) {
        while (text[position] == ' ') position++;
        if (text[position] == '\0') break;

        const int start = position;
        int end = start;  // one past the last word that fits
        int scan = start;
        while (text[scan] != '\0') {
            int wordEnd = scan;
            while (text[wordEnd] != '\0' && text[wordEnd] != ' ') wordEnd++;
            const bool firstWord = end == start;
            if (!firstWord && wordEnd - start > maxChars) break;
            end = wordEnd;
            scan = wordEnd;
            while (text[scan] == ' ') scan++;
            if (firstWord && end - start > maxChars) break;
        }
        lines[count++] = {start, end - start};
        position = end;
    }
    return count;
}

}  // namespace ScreenLayout
