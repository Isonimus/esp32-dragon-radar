#ifndef SCREEN_TEXT_H
#define SCREEN_TEXT_H

/**
 * The text the menu and loading screens draw, and its sizes, in one place so the native
 * tests can hold every line to the safe area on both panel shapes (ADR-0014).
 */
namespace ScreenText {

// LovyanGFX text sizes: 1 is the 6 x 8 px default font, 2 doubles it.
constexpr int BODY_SIZE = 1;
constexpr int HEADLINE_SIZE = 2;

constexpr const char* CONTINUE_FORMAT = "Continue (%u/%u)";
constexpr const char* NEW_GAME = "New Game";

constexpr const char* DISTANCE_FORMAT = "Distance: < %s >";
constexpr const char* DISTANCE_LABELS[] = {"Easy (500m)", "Medium (1.5km)", "Hard (5km)"};
constexpr const char* SOUND_FORMAT = "Sound:    < %s >";
constexpr const char* START = "[ START ]";

constexpr const char* SEARCHING = "Searching for satellites...";
constexpr const char* SATELLITES_FORMAT = "Satellites: %u";
constexpr const char* NO_FIX = "No Fix (Seek Clear Sky)";
constexpr const char* SETTINGS_NOT_SAVED = "Settings not saved";

constexpr const char* SCATTERING = "SCATTERING BALLS...";

constexpr const char* BALL_FOUND_FORMAT = "%u-STAR BALL FOUND!";

constexpr const char* WIN_HEADLINE = "SHENRON SUMMONED!";
constexpr const char* WIN_DETAIL = "All 7 Dragon Balls collected.";
constexpr const char* WIN_PROMPT = "Press to continue...";

}  // namespace ScreenText

#endif  // SCREEN_TEXT_H
