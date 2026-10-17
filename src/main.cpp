#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <iterator>

#include "hal/hal.h"
#include "render/capsule_logo.h"
#include "render/dragon_radar_logo.h"
#include "firmware_version.h"
#include "game/ball_manager.h"
#include "render/radar_renderer.h"
#include "render/screen_layout.h"
#include "render/screen_text.h"
#include "game/radar_heading.h"
#include "game/menu_cursor.h"
#include "game/blip_blink.h"
#include "debug/screen_capture.h"

// --- Game States ---
enum GameState {
    STATE_BOOT,
    STATE_SPLASH,
    STATE_MENU,
    STATE_SETTINGS,
    STATE_GPS_WAIT,
    STATE_GENERATING,
    STATE_RADAR,
    STATE_COLLECTED,
    STATE_WIN
};

// --- Global Objects ---
// getHAL() returns a constant-initialised bundle, so it is safe to call
// during static initialisation.
static BallManager ballManager(getHAL().storage);
static RadarHeading radarHeading;
static bool silentMode = false;

static GameState currentState = STATE_BOOT;

// --- Layout ---
// Measured on the Cardputer ADV, the case hides only the outermost pixel row
// when viewed straight on, but its lip stands above the glass and shadows a few
// more rows at a normal viewing angle, so top-row text keeps a wider margin.
static const int HUD_TOP_PX = 10;
static const int HUD_SIDE_PX = 5;
static const int HUD_LINE_PX = 10;
static const int LOGO_TOP_PX = 3;
static const int VERSION_GAP_PX = 4;
// Where content may go on this panel (ADR-0014): every screen positions from it, not
// from the canvas's size. Set once the display is up.
static ScreenLayout::Rect safeArea;

// The main menu has room for the title card's two stacked lines; the other
// screens start their content higher and get the one-line title (ADR-0006), unless
// it is wider than the safe area (ADR-0014).
enum class TitleLayout { Inline, Stacked };
// Main menu items, centre to centre. The logo-to-first-item gap matches the
// gap between items so the column reads evenly.
static const int MENU_ITEM_PITCH_PX = 28;
// The settings start this far below the title, centre line to its bottom, and this far
// in from the safe area's left, unless the widest setting needs the room.
static const int SETTINGS_BELOW_TITLE_PX = 14;
static const int SETTINGS_LEFT_PX = 20;
static const int SETTINGS_PITCH_PX = 20;
static const int START_BELOW_SETTINGS_PX = 55;
// The GPS wait screen's status lines, bottom edges above the safe area's bottom.
static const int SEARCHING_ABOVE_BOTTOM_PX = 25;
static const int SATELLITES_ABOVE_BOTTOM_PX = 12;
static const int NO_FIX_ABOVE_BOTTOM_PX = 2;
static const int SCATTERING_ABOVE_BOTTOM_PX = 10;
// The GPS wait screen's dragon ball at its full size; on a crowded screen it shrinks to
// the room between the title and the status lines, and its stars with it.
static const int WAITING_BALL_RADIUS_PX = 35;
static const float STAR_ORBIT_PER_BALL_RADIUS = 18.0f / WAITING_BALL_RADIUS_PX;
static const float STAR_RADIUS_PER_BALL_RADIUS = 6.0f / WAITING_BALL_RADIUS_PX;
static const float STAR_INNER_RADIUS_PER_BALL_RADIUS = 2.0f / WAITING_BALL_RADIUS_PX;
static const float SCATTER_BALL_RADIUS_PER_BALL_RADIUS = 15.0f / WAITING_BALL_RADIUS_PX;
static const int BALL_GAP_PX = 2;
// Headlines start this far below the top of the safe area; a prompt sits this far
// above its bottom.
static const int HEADLINE_TOP_PX = 10;
static const int PROMPT_ABOVE_BOTTOM_PX = 2;
static const int WIN_DETAIL_GAP_PX = 4;
// The found ball pulses between its radius and this much larger.
static const int FOUND_BALL_RADIUS_PX = 40;
static const float FOUND_BALL_PULSE = 0.5f;
// The win screen's ring of seven balls.
static const int WIN_RING_RADIUS_PX = 40;
static const int WIN_BALL_RADIUS_PX = 10;
static const int WRAPPED_LINE_GAP_PX = 2;
// The radar's LOST FIX warning sits this far above the bottom of the rim, clear of
// the zoom legend.
static const int LOST_FIX_ABOVE_RIM_PX = 30;
// The cached radar background leaves the custom board little heap (ADR-0012), so play
// logs the heap's low-water mark at each zoom change and on this interval.
static const uint32_t HEAP_LOG_INTERVAL_MS = 10000;
static uint8_t lastCollectedId = 0;
static BlipBlink blipBlink;

// Menu & Settings state
static int menuCursor = 0;
static int settingsCursor = 0;
// Distance, sound, start.
static const int SETTINGS_ITEM_COUNT = 3;
static const uint16_t distancePresets[] = {500, 1500, 5000};
static const int DISTANCE_PRESET_COUNT = std::size(distancePresets);
static_assert(std::size(ScreenText::DISTANCE_LABELS) == std::size(distancePresets),
              "one settings label per distance preset");
static const int DEFAULT_DISTANCE_PRESET = 1; // Medium
static int distancePresetIdx = DEFAULT_DISTANCE_PRESET; // 0=Easy, 1=Medium, 2=Hard
static bool settingSoundOn = true;
static bool continueMode = false; // true = Continue, false = New Game
// The settings still apply for this session when their save fails, so the game goes on;
// the GPS wait screen says so, since the next boot will revert to the defaults.
static bool settingsUnsaved = false;

// Radar renderer and canvas are created after display init
static RadarRenderer* radar = nullptr;
static LGFX_Sprite*   canvas = nullptr;

// Settings persistence
void saveSettings() {
    settingsUnsaved = !getHAL().storage.saveSettings({static_cast<uint8_t>(distancePresetIdx), settingSoundOn});
}

void loadSettings() {
    GameSettings saved;
    if (getHAL().storage.loadSettings(saved)) {
        // A preset list that shrank since the save falls back to the default.
        distancePresetIdx = saved.distancePresetIdx < DISTANCE_PRESET_COUNT
            ? saved.distancePresetIdx : DEFAULT_DISTANCE_PRESET;
        settingSoundOn = saved.soundOn;
    }
    silentMode = !settingSoundOn;
}

// Helper to draw a 5-pointed star
void fillStar(LGFX_Sprite& c, int x, int y, int r1, int r2, uint16_t color) {
    int x_pts[10];
    int y_pts[10];
    
    for (int i = 0; i < 5; i++) {
        float a1 = -M_PI / 2 + i * (2 * M_PI / 5);
        x_pts[i * 2] = x + cos(a1) * r1;
        y_pts[i * 2] = y + sin(a1) * r1;
        float a2 = a1 + M_PI / 5;
        x_pts[i * 2 + 1] = x + cos(a2) * r2;
        y_pts[i * 2 + 1] = y + sin(a2) * r2;
    }
    
    for (int i = 0; i < 10; i++) {
        c.fillTriangle(x, y, x_pts[i], y_pts[i], x_pts[(i + 1) % 10], y_pts[(i + 1) % 10], color);
    }
}

enum class TextAnchor { Top, Bottom };

// Draws `text` word-wrapped to the safe area's width, each line centred, with the block's
// top or bottom at y. Returns the block's other edge.
int drawWrappedText(LGFX_Sprite& c, const char* text, int textSize, int y, TextAnchor anchor) {
    ScreenLayout::LineSpan lines[ScreenLayout::MAX_WRAPPED_LINES];
    const int maxChars = safeArea.width / (ScreenLayout::GLYPH_WIDTH_PX * textSize);
    const int count = ScreenLayout::wrapWords(text, maxChars, lines, ScreenLayout::MAX_WRAPPED_LINES);

    c.setTextSize(textSize);
    c.setTextDatum(TC_DATUM);
    const int pitch = c.fontHeight() + WRAPPED_LINE_GAP_PX;
    const int blockHeight = count * pitch - WRAPPED_LINE_GAP_PX;
    const int top = anchor == TextAnchor::Top ? y : y - blockHeight;
    for (int i = 0; i < count; i++) {
        char line[64];
        snprintf(line, sizeof(line), "%.*s", lines[i].length, text + lines[i].start);
        c.drawString(line, safeArea.centreX(), top + i * pitch);
    }
    c.setTextDatum(TL_DATUM);
    return anchor == TextAnchor::Top ? top + blockHeight : top;
}

// Draws the logo and the version, and returns the title's bottom edge.
int drawTitle(LGFX_Sprite& c, TitleLayout layout) {
    const bool stacked = layout == TitleLayout::Stacked || DRAGON_RADAR_LOGO_INLINE_WIDTH > safeArea.width;
    const uint32_t* pixels = stacked ? DRAGON_RADAR_LOGO_STACKED : DRAGON_RADAR_LOGO_INLINE;
    const int width = stacked ? DRAGON_RADAR_LOGO_STACKED_WIDTH : DRAGON_RADAR_LOGO_INLINE_WIDTH;
    const int height = stacked ? DRAGON_RADAR_LOGO_STACKED_HEIGHT : DRAGON_RADAR_LOGO_INLINE_HEIGHT;
    const int logoX = safeArea.x + (safeArea.width - width) / 2;
    const int logoTop = safeArea.y + LOGO_TOP_PX;
    const int logoBottom = logoTop + height;
    c.pushAlphaImage(logoX, logoTop, width, height, reinterpret_cast<const lgfx::argb8888_t*>(pixels));

    c.setTextSize(ScreenText::BODY_SIZE);
    c.setTextColor(TFT_WHITE);
    const int versionX = logoX + width + VERSION_GAP_PX;
    if (stacked && versionX + c.textWidth(FIRMWARE_VERSION) <= safeArea.right()) {
        // Beside the second line, which leaves the space under the logo to the menu.
        c.setTextDatum(BL_DATUM);
        c.drawString(FIRMWARE_VERSION, versionX, logoBottom);
        c.setTextDatum(TL_DATUM);
        return logoBottom;
    }
    c.setTextDatum(TR_DATUM);
    c.drawString(FIRMWARE_VERSION, logoX + width, logoBottom);
    c.setTextDatum(TL_DATUM);
    return logoBottom + c.fontHeight();
}

// Returns the title's bottom edge, where the screen's own content starts.
int drawCommonBackground(LGFX_Sprite& c, TitleLayout layout) {
    uint16_t bgColor = 0x2D73;
    uint16_t footerColor = c.color565(8, 112, 175);
    
    c.fillScreen(bgColor);
    // Decoration, so it spans the panel and the round panel's edge clips it (ADR-0014).
    int footerY = c.height() * 3 / 4;
    int footerH = c.height() / 4;
    c.fillRect(0, footerY, c.width(), footerH, footerColor);
    
    uint16_t animLineColor = c.color565(30, 120, 185);
    uint32_t t = millis() % 1000;
    int lineSpacing = footerH / 2;
    int zigzagFreq = 12;
    
    static const int offsets[] = {
        3, -4, 6, -2, 5, -5, 4, -3, 5, -2, 6, -4, 3, -5, 4, -2, 5, -3, 6, -4, 2
    };
    const int numOffsets = sizeof(offsets)/sizeof(offsets[0]);
    
    c.setClipRect(0, footerY, c.width(), footerH);
    for (int i = 0; i < 3; i++) {
        int yBase = footerY + ((t * footerH / 1000) + i * lineSpacing) % footerH;
        int lastX = -zigzagFreq;
        int lastY = yBase + offsets[0];
        int numSegments = (c.width() / zigzagFreq) + 2;
        for (int j = 0; j <= numSegments; j++) {
            int x = (j - 1) * zigzagFreq;
            int nextY = yBase + offsets[j % numOffsets];
            c.drawLine(lastX, lastY - 1, x, nextY - 1, animLineColor);
            c.drawLine(lastX, lastY, x, nextY, animLineColor);
            c.drawLine(lastX, lastY + 1, x, nextY + 1, animLineColor);
            lastX = x;
            lastY = nextY;
        }
    }
    c.clearClipRect();

    return drawTitle(c, layout);
}

struct BallSpot {
    int x;
    int y;
    int radius;
};

// The GPS wait screen's dragon ball, which the scattering screen then breaks up: centred
// in the room between the content's top and the status lines, at full size if it fits.
BallSpot waitingBallSpot(LGFX_Sprite& c, int contentTop) {
    c.setTextSize(ScreenText::BODY_SIZE);
    const int statusTop = safeArea.bottom() - SEARCHING_ABOVE_BOTTOM_PX - c.fontHeight();
    const int room = statusTop - contentTop - 2 * BALL_GAP_PX;
    return {safeArea.centreX(), (contentTop + statusTop) / 2, std::min(WAITING_BALL_RADIUS_PX, room / 2)};
}

void updateGpsWaitScreen(LGFX_Sprite& c, const GpsFix& fix) {
    int contentTop = drawCommonBackground(c, TitleLayout::Inline);
    if (settingsUnsaved) {
        c.setTextDatum(TC_DATUM);
        c.setTextColor(TFT_RED);
        c.setTextSize(ScreenText::BODY_SIZE);
        c.drawString(ScreenText::SETTINGS_NOT_SAVED, safeArea.centreX(), contentTop + BALL_GAP_PX);
        contentTop += BALL_GAP_PX + c.fontHeight();
    }
    
    uint16_t ballColor = c.color565(243, 147, 7);
    uint16_t borderColor = 0xFDA0;
    uint16_t starColor = TFT_RED;
    
    const BallSpot ball = waitingBallSpot(c, contentTop);
    int cx = ball.x;
    int cy = ball.y;
    int r = ball.radius;
    
    c.fillCircle(cx, cy, r, ballColor);
    c.drawCircle(cx, cy, r, borderColor);
    
    uint32_t animTime = millis() % 2800;
    int starsToShow = (animTime / 400) + 1;
    
    int starRadius = r * STAR_RADIUS_PER_BALL_RADIUS;
    int innerRadius = r * STAR_INNER_RADIUS_PER_BALL_RADIUS;
    float starDist = r * STAR_ORBIT_PER_BALL_RADIUS;
    
    for (int i = 0; i < starsToShow; i++) {
        int sx, sy;
        if (i == 6) { sx = cx; sy = cy; }
        else {
            float angle = (i * 60.0f - 90.0f) * (M_PI / 180.0f);
            sx = cx + std::cos(angle) * starDist;
            sy = cy + std::sin(angle) * starDist;
        }
        fillStar(c, sx, sy, starRadius, innerRadius, starColor);
    }
    
    c.setTextDatum(BC_DATUM);
    c.setTextColor(TFT_WHITE);
    c.setTextSize(ScreenText::BODY_SIZE);
    
    c.drawString(ScreenText::SEARCHING, cx, safeArea.bottom() - SEARCHING_ABOVE_BOTTOM_PX);
    
    char satStr[32];
    snprintf(satStr, sizeof(satStr), ScreenText::SATELLITES_FORMAT, fix.satellites);
    c.drawString(satStr, cx, safeArea.bottom() - SATELLITES_ABOVE_BOTTOM_PX);

    if (!fix.current) {
        c.setTextColor(TFT_RED);
        c.drawString(ScreenText::NO_FIX, cx, safeArea.bottom() - NO_FIX_ABOVE_BOTTOM_PX);
    }
    c.setTextDatum(TL_DATUM);
    
    c.pushSprite(0, 0);
}

void updateScatteringScreen(LGFX_Sprite& c, HardwareHAL& hal) {
    static uint32_t scatterStart = 0;
    if (scatterStart == 0) scatterStart = millis();
    
    uint32_t elapsed = millis() - scatterStart;
    float progress = std::min(1.0f, elapsed / 3000.0f);
    
    const int titleBottom = drawCommonBackground(c, TitleLayout::Inline);
    
    // The balls break out of the GPS wait screen's ball, so they share its spot and size.
    const BallSpot origin = waitingBallSpot(c, titleBottom);
    int cx = origin.x;
    int cy = origin.y;
    uint16_t ballColor = c.color565(243, 147, 7);
    uint16_t borderColor = 0xFDA0;
    uint16_t starColor = TFT_RED;
    
    float scatterProgress = 0.0f;
    if (elapsed > 1000) {
        scatterProgress = (elapsed - 1000) / 2000.0f;
        scatterProgress = scatterProgress * scatterProgress;
    }
    
    float baseDist = origin.radius * STAR_ORBIT_PER_BALL_RADIUS;
    float maxFlyDist = 200.0f;
    
    for (int i = 0; i < 7; i++) {
        float angle;
        if (i < 6) {
            angle = (i * 60.0f - 90.0f) * (M_PI / 180.0f);
        } else {
            angle = 0.0f;
        }
        
        float currentDist = baseDist + scatterProgress * maxFlyDist;
        int bx, by;
        
        if (i < 6) {
            bx = cx + std::cos(angle) * currentDist;
            by = cy + std::sin(angle) * currentDist;
        } else {
            bx = cx + scatterProgress * 50;
            by = cy + scatterProgress * 100;
        }
        
        if (elapsed <= 1000) {
            bx += random(-1, 2);
            by += random(-1, 2);
        }
        
        float scale = 1.0f - scatterProgress * 0.6f;
        int r = origin.radius * SCATTER_BALL_RADIUS_PER_BALL_RADIUS * scale;
        int sr = origin.radius * STAR_RADIUS_PER_BALL_RADIUS * scale;
        int ir = origin.radius * STAR_INNER_RADIUS_PER_BALL_RADIUS * scale;
        
        if (scatterProgress > 0.1f) {
            for (int t = 1; t <= 3; t++) {
                float trailProgress = std::max(0.0f, scatterProgress - t * 0.05f);
                float trailDist = baseDist + trailProgress * maxFlyDist;
                int tx, ty;
                if (i < 6) {
                    tx = cx + std::cos(angle) * trailDist;
                    ty = cy + std::sin(angle) * trailDist;
                } else {
                    tx = cx + trailProgress * 50;
                    ty = cy + trailProgress * 100;
                }
                uint8_t alpha = 200 / t;
                uint16_t tColor = c.color565(255 * alpha / 255, 165 * alpha / 255, 0);
                c.fillCircle(tx, ty, r * (0.8f / t), tColor);
            }
        }
        
        c.fillCircle(bx, by, r, ballColor);
        c.drawCircle(bx, by, r, borderColor);
        fillStar(c, bx, by, sr, ir, starColor);
    }
    
    c.setTextColor(TFT_WHITE);
    if (elapsed < 1500) {
        drawWrappedText(c, ScreenText::SCATTERING, ScreenText::HEADLINE_SIZE,
                        safeArea.bottom() - SCATTERING_ABOVE_BOTTOM_PX, TextAnchor::Bottom);
    }
    
    c.pushSprite(0, 0);
    
    if (progress >= 1.0f) {
        scatterStart = 0;
        hal.audio.stopMelody();
        currentState = STATE_RADAR;
    }
}

static void logMinFreeHeap() {
    Serial.printf("[radar] min_free_heap=%lu\n", static_cast<unsigned long>(ESP.getMinFreeHeap()));
}

// Stops boot where the fault is visible: the game cannot run without this part.
[[noreturn]] static void haltOnScreen(lgfx::LGFX_Device& tft, const char* message) {
    Serial.printf("Setup: %s - halting\n", message);
    tft.setTextColor(TFT_RED);
    // Centred, because the custom board's round panel does not show its corners.
    tft.setTextDatum(textdatum_t::middle_center);
    tft.drawString(message, tft.width() / 2, tft.height() / 2);
    while (true) delay(1000);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    HardwareHAL& hal = getHAL();

    Serial.printf("Dragon Radar %s booting...\n", FIRMWARE_VERSION);

    // Power management (hold-pin etc.); pin writes only, nothing that can fail.
    hal.power.begin();

    // Display: without a panel no message can be shown, so serial is all there is.
    Serial.println("Setup: Initializing Display...");
    if (!hal.display.init()) {
        Serial.println("Setup: Display FAILED - halting");
        while (true) delay(1000);
    }
    auto& tft = hal.display.getDevice();
    tft.setTextColor(TFT_GREEN);
    Serial.println("Setup: Display OK");
    safeArea = ScreenLayout::safeArea(tft.width(), tft.height(), hal.display.isRound());

    // Input
    Serial.println("Setup: Initializing Input...");
    if (!hal.input.begin()) haltOnScreen(tft, "Input hardware not found");
    Serial.println("Setup: Input OK");

    // IMU: optional, the radar falls back to the GPS course for its heading.
    Serial.println("Setup: Initializing IMU...");
    if (!hal.imu.begin()) {
        Serial.println("Setup: IMU FAILED - heading from GPS course only");
    } else {
        Serial.printf("Setup: IMU %s\n", hal.imu.available() ? "OK" : "none fitted");
    }

    // Audio: optional, an audio fault costs only the sound.
    Serial.println("Setup: Initializing Audio...");
    Serial.println(hal.audio.begin() ? "Setup: Audio OK" : "Setup: Audio FAILED - continuing silent");

    // GPS: the UART start reports nothing, so a missing module surfaces in
    // GPS_WAIT as no fix, not here.
    Serial.println("Setup: Initializing GPS...");
    hal.gps.begin();
    Serial.println("Setup: GPS UART started");

    // Canvas (sprite for double-buffered rendering); without it every frame draws nowhere.
    Serial.println("Setup: Creating Canvas...");
    canvas = new LGFX_Sprite(&tft);
    canvas->setColorDepth(16);
    if (canvas->createSprite(tft.width(), tft.height()) == nullptr) {
        haltOnScreen(tft, "No memory for frame buffer");
    }
    Serial.println("Setup: Canvas OK");

    // Radar renderer; its cached background is allocated next to the canvas, before the
    // heap fragments further (ADR-0012).
    radar = new RadarRenderer(hal.display.isRound());
    if (!radar->begin(*canvas)) haltOnScreen(tft, "No memory for radar background");
    Serial.println("Setup: Radar background OK");

    Serial.println("All Systems Initialized.");
    loadSettings();
    // Loaded before the menu first draws, which shows its progress.
    if (ballManager.load()) {
        Serial.printf("Setup: saved game %u/%u\n", ballManager.collectedCount(), BallManager::BALL_COUNT);
    } else {
        Serial.println("Setup: no game to continue");
    }
    currentState = STATE_SPLASH;
}

void loop() {
    HardwareHAL& hal = getHAL();
    LGFX_Sprite& c = *canvas;
    auto& tft = hal.display.getDevice();

    hal.gps.update();
    hal.audio.update();
    hal.imu.update();
    hal.input.update();

    GpsFix fix = hal.gps.getFix();

    // Before this loop draws, so the canvas still holds the frame on the display.
    serviceScreenCapture(c, Serial);

    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate > 50) { // 20Hz refresh

        InputAction action = hal.input.getAction();

        // Global action handling
        if (action == InputAction::LONG_PRIMARY) {
            if (currentState == STATE_MENU) {
                Serial.println("Shutting down...");
                hal.power.shutdown();
            }
        }
        if (action == InputAction::LONG_SECONDARY) {
            silentMode = !silentMode;
            Serial.printf("Silent mode: %s\n", silentMode ? "ON" : "OFF");
        }

        // Zoom controls (only in radar state)
        if (currentState == STATE_RADAR) {
            if (action == InputAction::PRIMARY) {
                radar->zoomIn();
                Serial.println("Zoom In");
                logMinFreeHeap();
            }
            if (action == InputAction::SECONDARY || action == InputAction::LONG_PRIMARY) {
                radar->zoomOut();
                Serial.println("Zoom Out");
                logMinFreeHeap();
            }
        }

        switch (currentState) {
            case STATE_SPLASH: {
                static uint32_t splashStart = 0;
                if (splashStart == 0) splashStart = millis();
                
                uint32_t elapsed = millis() - splashStart;
                int brightness = 0;
                
                if (elapsed < 1000) {
                    brightness = (elapsed * 255) / 1000;
                } else if (elapsed < 2000) {
                    brightness = 255;
                } else if (elapsed < 3000) {
                    brightness = 255 - ((elapsed - 2000) * 255) / 1000;
                } else {
                    hal.display.setBrightness(255);
                    currentState = STATE_MENU;
                    if (!silentMode) hal.audio.playMenuTheme();
                    splashStart = 0;
                    break;
                }
                
                c.fillScreen(TFT_BLACK);
                
                int lx = (c.width() - 110) / 2;
                int ly = (c.height() - 109) / 2;
                
                hal.display.setBrightness(brightness);
                
                if (c.drawBmp(data_capsule_bmp, data_capsule_bmp_len, lx, ly)) {
                    for (int y = ly; y < ly + 109; y++) {
                        for (int x = lx; x < lx + 110; x++) {
                            uint16_t col = c.readPixel(x, y);
                            if (col > 0x8000) c.drawPixel(x, y, TFT_BLACK);
                            else c.drawPixel(x, y, TFT_WHITE);
                        }
                    }
                }
                
                c.pushSprite(0, 0);
            }
            break;

            case STATE_MENU: {
                const int titleBottom = drawCommonBackground(c, TitleLayout::Stacked);
                
                int cx = safeArea.centreX();
                uint16_t goldColor = 0xFDA0;
                
                bool hasSave = ballManager.hasSavedGame();
                int numItems = hasSave ? 2 : 1;
                menuCursor = menuCursor % numItems;
                
                c.setTextSize(ScreenText::HEADLINE_SIZE);
                c.setTextDatum(MC_DATUM);
                const int itemHeight = c.fontHeight();
                const int itemGap = MENU_ITEM_PITCH_PX - itemHeight;
                const int yStart = titleBottom + itemGap + itemHeight / 2;
                
                if (hasSave) {
                    char contLabel[32];
                    snprintf(contLabel, sizeof(contLabel), ScreenText::CONTINUE_FORMAT, ballManager.collectedCount(), BallManager::BALL_COUNT);
                    c.setTextColor(menuCursor == 0 ? goldColor : TFT_WHITE);
                    c.drawString(contLabel, cx, yStart);
                    c.setTextColor(menuCursor == 1 ? goldColor : TFT_WHITE);
                    c.drawString(ScreenText::NEW_GAME, cx, yStart + MENU_ITEM_PITCH_PX);
                } else {
                    c.setTextColor(goldColor);
                    c.drawString(ScreenText::NEW_GAME, cx, yStart);
                }
                c.setTextDatum(TL_DATUM);
                
                c.pushSprite(0, 0);
                
                menuCursor = moveMenuCursor(menuCursor, numItems, action);
                if (action == InputAction::PRIMARY) {
                    if (hasSave && menuCursor == 0) {
                        continueMode = true;
                        currentState = STATE_GPS_WAIT;
                        Serial.println("Menu: Continue");
                    } else {
                        continueMode = false;
                        // The old save stays until generate() overwrites it, so a
                        // reboot before the new balls are scattered can still continue it.
                        currentState = STATE_SETTINGS;
                        Serial.println("Menu: New Game");
                    }
                }
            }
            break;

            case STATE_SETTINGS: {
                const int titleBottom = drawCommonBackground(c, TitleLayout::Inline);
                
                int cx = safeArea.centreX();
                uint16_t goldColor = 0xFDA0;
                
                int yBase = titleBottom + SETTINGS_BELOW_TITLE_PX;
                c.setTextSize(ScreenText::BODY_SIZE);
                c.setTextDatum(ML_DATUM);

                // Left-aligned so the arrows line up, and kept still as the distance
                // label changes: placed for the widest one.
                char distStr[40];
                int widestSetting = 0;
                for (const char* label : ScreenText::DISTANCE_LABELS) {
                    snprintf(distStr, sizeof(distStr), ScreenText::DISTANCE_FORMAT, label);
                    widestSetting = std::max(widestSetting, static_cast<int>(c.textWidth(distStr)));
                }
                const int settingsX = safeArea.x + std::min(SETTINGS_LEFT_PX, (safeArea.width - widestSetting) / 2);
                
                c.setTextColor(settingsCursor == 0 ? goldColor : TFT_WHITE);
                snprintf(distStr, sizeof(distStr), ScreenText::DISTANCE_FORMAT, ScreenText::DISTANCE_LABELS[distancePresetIdx]);
                c.drawString(distStr, settingsX, yBase);
                
                c.setTextColor(settingsCursor == 1 ? goldColor : TFT_WHITE);
                char sndStr[20];
                snprintf(sndStr, sizeof(sndStr), ScreenText::SOUND_FORMAT, settingSoundOn ? "ON" : "OFF");
                c.drawString(sndStr, settingsX, yBase + SETTINGS_PITCH_PX);
                
                c.setTextDatum(MC_DATUM);
                c.setTextSize(ScreenText::HEADLINE_SIZE);
                c.setTextColor(settingsCursor == 2 ? goldColor : TFT_WHITE);
                c.drawString(ScreenText::START, cx, yBase + START_BELOW_SETTINGS_PX);
                c.setTextDatum(TL_DATUM);
                
                c.pushSprite(0, 0);
                
                settingsCursor = moveMenuCursor(settingsCursor, SETTINGS_ITEM_COUNT, action);
                if (action == InputAction::PRIMARY) {
                    if (settingsCursor == 0) {
                        distancePresetIdx = (distancePresetIdx + 1) % DISTANCE_PRESET_COUNT;
                    } else if (settingsCursor == 1) {
                        settingSoundOn = !settingSoundOn;
                    } else {
                        saveSettings();
                        silentMode = !settingSoundOn;
                        currentState = STATE_GPS_WAIT;
                        Serial.printf("Settings: dist=%um, sound=%s\n",
                            distancePresets[distancePresetIdx],
                            settingSoundOn ? "ON" : "OFF");
                    }
                }
            }
            break;

            case STATE_GPS_WAIT:
                updateGpsWaitScreen(c, fix);
                if (fix.current && fix.satellites >= 4) {
                    Serial.println("Fix acquired.");
                    if (continueMode) {
                        hal.audio.stopMelody();
                        currentState = STATE_RADAR;
                    } else {
                        currentState = STATE_GENERATING;
                    }
                }
                break;

            case STATE_GENERATING:
                updateScatteringScreen(c, hal);
                
                {
                    static bool generated = false;
                    if (!generated) {
                        ballManager.generate(fix.lat, fix.lon, distancePresets[distancePresetIdx]);
                        Serial.println("7 Dragon Balls generated.");
                        generated = true;
                    }
                    if (currentState == STATE_RADAR) {
                        generated = false;
                    }
                }
                break;

            case STATE_RADAR: {
                static uint32_t lastHeapLog = 0;
                if (millis() - lastHeapLog >= HEAP_LOG_INTERVAL_MS) {
                    logMinFreeHeap();
                    lastHeapLog = millis();
                }

                float rotation = radarHeading.select(fix, hal.imu.available(), hal.imu.getHeading());

                BlinkState blink = blipBlink.update(millis());
                if (blink.onset && !silentMode) hal.audio.playRadarBeep();
                radar->render(c, fix, ballManager.getBalls(), blink.lit, rotation);
                
                // HUD overlay
                c.setTextColor(TFT_GREEN);
                c.setTextSize(1);
                const int hudLeft = safeArea.x + HUD_SIDE_PX;
                const int hudRight = safeArea.right() - HUD_SIDE_PX;
                const int hudTop = safeArea.y + HUD_TOP_PX;
                c.setCursor(hudLeft, hudTop);
                c.printf("Sats: %u", fix.satellites);
                
                if (hal.power.hasBattery()) {
                    c.setCursor(hudLeft, hudTop + HUD_LINE_PX);
                    c.printf("Bat: %.1fV", hal.power.getBatteryVoltage());
                }

                c.setTextDatum(TR_DATUM);
                char ballsStr[16];
                snprintf(ballsStr, sizeof(ballsStr), "Balls: %u/%u", ballManager.collectedCount(), BallManager::BALL_COUNT);
                c.drawString(ballsStr, hudRight, hudTop);
                // Stays up until a save succeeds: a reboot now would lose progress.
                if (ballManager.progressUnsaved()) {
                    c.setTextColor(TFT_RED);
                    c.drawString("NOT SAVED", hudRight, hudTop + HUD_LINE_PX);
                    c.setTextColor(TFT_GREEN);
                }
                c.setTextDatum(TL_DATUM);
                
                int cx = radar->layout().cx;
                int cy = radar->layout().cy;
                int r = radar->layout().rangeRadius;

                if (fix.hasPosition && !fix.current) {
                    c.setTextColor(TFT_RED);
                    c.setTextSize(1);
                    // Centred by the font's own measure: a hand-picked x offset left it
                    // 6 px off centre.
                    c.setTextDatum(TC_DATUM);
                    // On the round panel the rim is past the safe area, so it stays a
                    // line clear of the zoom legend at the safe area's bottom.
                    const int lostFixTop = std::min(cy + r - LOST_FIX_ABOVE_RIM_PX,
                                                    safeArea.bottom() - 2 * HUD_LINE_PX);
                    c.drawString("LOST FIX", cx, lostFixTop);
                    c.setTextDatum(TL_DATUM);
                }
                
                c.pushSprite(0, 0);
                
                // A stale position was already checked while it was current.
                if (fix.current) {
                    uint8_t id = ballManager.checkProximity(fix.lat, fix.lon);
                    if (id > 0) {
                        lastCollectedId = id;
                        currentState = STATE_COLLECTED;
                        if (!silentMode) hal.audio.playCollectionFanfare();
                        Serial.printf("COLLECTED BALL %u!\n", id);
                    }
                }
            }
            break;
                
            case STATE_COLLECTED: {
                c.fillScreen(TFT_BLACK);
                
                static uint32_t collectStart = 0;
                if (collectStart == 0) collectStart = millis();
                
                uint32_t elapsed = millis() - collectStart;
                
                c.setTextColor(TFT_YELLOW);
                char headline[32];
                snprintf(headline, sizeof(headline), ScreenText::BALL_FOUND_FORMAT, lastCollectedId);
                const int headlineBottom = drawWrappedText(c, headline, ScreenText::HEADLINE_SIZE,
                                                           safeArea.y + HEADLINE_TOP_PX, TextAnchor::Top);

                // Pulses in the room under the headline, shrunk if its largest size
                // would not fit there.
                const int room = safeArea.bottom() - headlineBottom;
                const float largest = std::min(FOUND_BALL_RADIUS_PX * (1.0f + FOUND_BALL_PULSE), room / 2.0f);
                float scale = 1.0f + FOUND_BALL_PULSE * sin(elapsed * 0.005f);
                int ballR = largest / (1.0f + FOUND_BALL_PULSE) * scale;
                
                RadarRenderer::drawDragonBall(c, safeArea.centreX(), headlineBottom + room / 2, ballR, lastCollectedId, false);
                
                c.pushSprite(0, 0);
                
                if (elapsed > 4000) {
                    collectStart = 0;
                    if (ballManager.remainingCount() == 0) {
                        currentState = STATE_WIN;
                        if (!silentMode) hal.audio.playWinFanfare();
                    } else {
                        currentState = STATE_RADAR;
                    }
                }
                break;
            }
                
            case STATE_WIN: {
                uint8_t pulse = 127 + 127 * sin(millis() * 0.002f);
                c.fillScreen(tft.color565(0, pulse / 4, 0));
                
                c.setTextColor(TFT_WHITE);
                const int headlineBottom = drawWrappedText(c, ScreenText::WIN_HEADLINE, ScreenText::HEADLINE_SIZE,
                                                           safeArea.y + HEADLINE_TOP_PX, TextAnchor::Top);
                const int detailBottom = drawWrappedText(c, ScreenText::WIN_DETAIL, ScreenText::BODY_SIZE,
                                                         headlineBottom + WIN_DETAIL_GAP_PX, TextAnchor::Top);
                const int promptTop = drawWrappedText(c, ScreenText::WIN_PROMPT, ScreenText::BODY_SIZE,
                                                      safeArea.bottom() - PROMPT_ABOVE_BOTTOM_PX, TextAnchor::Bottom);

                // The ring fills the room between the texts, at full size if it fits.
                const int ringCentreY = (detailBottom + promptTop) / 2;
                const int ringRadius = std::min(WIN_RING_RADIUS_PX,
                                                (promptTop - detailBottom) / 2 - WIN_BALL_RADIUS_PX);
                for (int i = 1; i <= 7; i++) {
                    float a = (i * 51.4f) * (M_PI/180.0f);
                    int bx = safeArea.centreX() + cos(a) * ringRadius;
                    int by = ringCentreY + sin(a) * ringRadius;
                    RadarRenderer::drawDragonBall(c, bx, by, WIN_BALL_RADIUS_PX, i, false);
                }
                
                c.pushSprite(0, 0);
                
                if (action == InputAction::PRIMARY) {
                    ballManager.reset();
                    currentState = STATE_MENU;
                    Serial.println("Win: returning to menu");
                }
                break;
            }

            default:
                break;
        }
        lastUpdate = millis();
    }
    
    delay(10);
}
