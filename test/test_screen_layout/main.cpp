#include <unity.h>
#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "../../src/render/screen_layout.h"
#include "../../src/render/screen_layout.cpp"
#include "../../src/render/screen_text.h"

using namespace ScreenLayout;

static const int ROUND_SIZE = 240;
static const int RECT_WIDTH = 240;
static const int RECT_HEIGHT = 135;
// The worst-case numbers the screens put in their formatted text.
static const unsigned MOST_SATELLITES = 99;
static const unsigned BALL_COUNT = 7;

void setUp() {}
void tearDown() {}

// Pixel (x, y) covers [x, x + 1) x [y, y + 1). It is inside the round panel if its corner
// farthest from the centre is.
static bool pixelInsideCircle(int x, int y) {
    const double centre = ROUND_SIZE / 2.0;
    const double radius = ROUND_SIZE / 2.0;
    const double farX = x + 0.5 < centre ? x : x + 1;
    const double farY = y + 0.5 < centre ? y : y + 1;
    return hypot(farX - centre, farY - centre) <= radius;
}

static bool squareInsideCircle(int inset) {
    for (int y = inset; y < ROUND_SIZE - inset; y++) {
        for (int x = inset; x < ROUND_SIZE - inset; x++) {
            if (!pixelInsideCircle(x, y)) return false;
        }
    }
    return true;
}

void test_round_safe_area_is_the_inscribed_square() {
    const Rect area = safeArea(ROUND_SIZE, ROUND_SIZE, true);
    TEST_ASSERT_EQUAL_INT(36, area.x);
    TEST_ASSERT_EQUAL_INT(36, area.y);
    TEST_ASSERT_EQUAL_INT(168, area.width);
    TEST_ASSERT_EQUAL_INT(168, area.height);
    TEST_ASSERT_EQUAL_INT(ROUND_SIZE / 2, area.centreX());
}

void test_round_safe_area_lies_inside_the_circle_and_cannot_grow() {
    const Rect area = safeArea(ROUND_SIZE, ROUND_SIZE, true);
    TEST_ASSERT_TRUE(squareInsideCircle(area.x));
    TEST_ASSERT_FALSE(squareInsideCircle(area.x - 1));
}

void test_rect_safe_area_is_the_whole_panel() {
    const Rect area = safeArea(RECT_WIDTH, RECT_HEIGHT, false);
    TEST_ASSERT_EQUAL_INT(0, area.x);
    TEST_ASSERT_EQUAL_INT(0, area.y);
    TEST_ASSERT_EQUAL_INT(RECT_WIDTH, area.right());
    TEST_ASSERT_EQUAL_INT(RECT_HEIGHT, area.bottom());
}

static std::string lineText(const char* text, const LineSpan& line) {
    return std::string(text + line.start, line.length);
}

void test_wrap_keeps_a_fitting_text_on_one_line() {
    LineSpan lines[MAX_WRAPPED_LINES];
    TEST_ASSERT_EQUAL_INT(1, wrapWords("ab cd", 5, lines, MAX_WRAPPED_LINES));
    TEST_ASSERT_EQUAL_STRING("ab cd", lineText("ab cd", lines[0]).c_str());
}

void test_wrap_breaks_at_the_last_space_that_fits() {
    const char* text = "ab cd ef";
    LineSpan lines[MAX_WRAPPED_LINES];
    TEST_ASSERT_EQUAL_INT(2, wrapWords(text, 6, lines, MAX_WRAPPED_LINES));
    TEST_ASSERT_EQUAL_STRING("ab cd", lineText(text, lines[0]).c_str());
    TEST_ASSERT_EQUAL_STRING("ef", lineText(text, lines[1]).c_str());
}

void test_wrap_skips_runs_of_spaces_between_lines() {
    const char* text = "  ab   cd  ";
    LineSpan lines[MAX_WRAPPED_LINES];
    TEST_ASSERT_EQUAL_INT(2, wrapWords(text, 3, lines, MAX_WRAPPED_LINES));
    TEST_ASSERT_EQUAL_STRING("ab", lineText(text, lines[0]).c_str());
    TEST_ASSERT_EQUAL_STRING("cd", lineText(text, lines[1]).c_str());
}

void test_wrap_gives_an_overlong_word_its_own_line() {
    const char* text = "a abcdef b";
    LineSpan lines[MAX_WRAPPED_LINES];
    TEST_ASSERT_EQUAL_INT(3, wrapWords(text, 3, lines, MAX_WRAPPED_LINES));
    TEST_ASSERT_EQUAL_STRING("a", lineText(text, lines[0]).c_str());
    TEST_ASSERT_EQUAL_STRING("abcdef", lineText(text, lines[1]).c_str());
    TEST_ASSERT_EQUAL_STRING("b", lineText(text, lines[2]).c_str());
}

void test_wrap_stops_at_the_line_limit() {
    LineSpan lines[2];
    TEST_ASSERT_EQUAL_INT(2, wrapWords("a b c", 1, lines, 2));
}

void test_wrap_of_empty_text_has_no_lines() {
    LineSpan lines[MAX_WRAPPED_LINES];
    TEST_ASSERT_EQUAL_INT(0, wrapWords("", 10, lines, MAX_WRAPPED_LINES));
    TEST_ASSERT_EQUAL_INT(0, wrapWords("   ", 10, lines, MAX_WRAPPED_LINES));
}

struct ScreenLine {
    std::string text;
    int textSize;
    // Drawn with drawString, so it must fit one line; the others are word-wrapped.
    bool singleLine;
};

static std::vector<ScreenLine> gameText() {
    char buffer[64];
    std::vector<ScreenLine> lines;
    snprintf(buffer, sizeof(buffer), ScreenText::CONTINUE_FORMAT, BALL_COUNT, BALL_COUNT);
    lines.push_back({buffer, ScreenText::HEADLINE_SIZE, true});
    lines.push_back({ScreenText::NEW_GAME, ScreenText::HEADLINE_SIZE, true});
    for (const char* label : ScreenText::DISTANCE_LABELS) {
        snprintf(buffer, sizeof(buffer), ScreenText::DISTANCE_FORMAT, label);
        lines.push_back({buffer, ScreenText::BODY_SIZE, true});
    }
    snprintf(buffer, sizeof(buffer), ScreenText::SOUND_FORMAT, "OFF");
    lines.push_back({buffer, ScreenText::BODY_SIZE, true});
    lines.push_back({ScreenText::START, ScreenText::HEADLINE_SIZE, true});
    lines.push_back({ScreenText::SEARCHING, ScreenText::BODY_SIZE, true});
    snprintf(buffer, sizeof(buffer), ScreenText::SATELLITES_FORMAT, MOST_SATELLITES);
    lines.push_back({buffer, ScreenText::BODY_SIZE, true});
    lines.push_back({ScreenText::NO_FIX, ScreenText::BODY_SIZE, true});
    lines.push_back({ScreenText::SETTINGS_NOT_SAVED, ScreenText::BODY_SIZE, true});
    lines.push_back({ScreenText::SCATTERING, ScreenText::HEADLINE_SIZE, false});
    snprintf(buffer, sizeof(buffer), ScreenText::BALL_FOUND_FORMAT, BALL_COUNT);
    lines.push_back({buffer, ScreenText::HEADLINE_SIZE, false});
    lines.push_back({ScreenText::WIN_HEADLINE, ScreenText::HEADLINE_SIZE, false});
    lines.push_back({ScreenText::WIN_DETAIL, ScreenText::BODY_SIZE, false});
    lines.push_back({ScreenText::WIN_PROMPT, ScreenText::BODY_SIZE, false});
    return lines;
}

// Every word comes back, in order, and every line fits the safe area's width.
static void assertGameTextFits(const Rect& area) {
    for (const ScreenLine& screenLine : gameText()) {
        const char* text = screenLine.text.c_str();
        const int maxChars = area.width / (GLYPH_WIDTH_PX * screenLine.textSize);
        LineSpan lines[MAX_WRAPPED_LINES];
        const int count = wrapWords(text, maxChars, lines, MAX_WRAPPED_LINES);

        std::string rejoined;
        for (int i = 0; i < count; i++) {
            TEST_ASSERT_LESS_OR_EQUAL_INT_MESSAGE(maxChars, lines[i].length, text);
            if (i > 0) rejoined += ' ';
            rejoined += lineText(text, lines[i]);
        }
        TEST_ASSERT_EQUAL_STRING_MESSAGE(text, rejoined.c_str(), text);
        if (screenLine.singleLine) TEST_ASSERT_EQUAL_INT_MESSAGE(1, count, text);
    }
}

void test_game_text_fits_the_round_safe_area() {
    assertGameTextFits(safeArea(ROUND_SIZE, ROUND_SIZE, true));
}

void test_game_text_fits_the_rect_safe_area() {
    assertGameTextFits(safeArea(RECT_WIDTH, RECT_HEIGHT, false));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_round_safe_area_is_the_inscribed_square);
    RUN_TEST(test_round_safe_area_lies_inside_the_circle_and_cannot_grow);
    RUN_TEST(test_rect_safe_area_is_the_whole_panel);
    RUN_TEST(test_wrap_keeps_a_fitting_text_on_one_line);
    RUN_TEST(test_wrap_breaks_at_the_last_space_that_fits);
    RUN_TEST(test_wrap_skips_runs_of_spaces_between_lines);
    RUN_TEST(test_wrap_gives_an_overlong_word_its_own_line);
    RUN_TEST(test_wrap_stops_at_the_line_limit);
    RUN_TEST(test_wrap_of_empty_text_has_no_lines);
    RUN_TEST(test_game_text_fits_the_round_safe_area);
    RUN_TEST(test_game_text_fits_the_rect_safe_area);
    return UNITY_END();
}
