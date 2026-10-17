#ifndef RADAR_RENDERER_H
#define RADAR_RENDERER_H

#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>
#include <iterator>
#include <memory>
#include "../gps/gps_driver.h"
#include "../game/ball_manager.h"
#include "radar_background.h"
#include "screen_layout.h"

/**
 * @brief Handles drawing the Dragon Radar UI.
 */
class RadarRenderer {
public:
    explicit RadarRenderer(bool roundPanel);

    /**
     * @brief Builds the shading table and allocates the cached background for a canvas
     * of this size (ADR-0012). False if there is no memory for them.
     */
    bool begin(const LGFX_Sprite& canvas);

    /**
     * @brief Render the full radar frame.
     * @param canvas Sprite to draw on, the size begin() was given
     * @param playerFix Current player GPS fix
     * @param balls List of Dragon Balls
     * @param blipsLit Whether the blink cycle shows the balls' blips this frame
     */
    void render(LGFX_Sprite& canvas, const GpsFix& playerFix, const std::vector<Ball>& balls, bool blipsLit, float rotation = 0.0f);

    const RadarBackground::Layout& layout() const { return _layout; }

    /**
     * @brief Helper to draw a Dragon Ball icon with stars.
     */
    static void drawDragonBall(LGFX_Sprite& canvas, int x, int y, int r, uint8_t stars, bool collected);

    /**
     * @brief Set radar zoom level (range in meters).
     */
    void zoomOut();
    void zoomIn();
    float getZoom() const { return ZOOM_LEVELS[_zoomIndex]; }

private:
    static constexpr float ZOOM_LEVELS[] = { 100.0f, 500.0f, 1000.0f, 5000.0f, 20000.0f };
    static constexpr int NUM_ZOOM_LEVELS = std::size(ZOOM_LEVELS);
    static_assert(NUM_ZOOM_LEVELS == RadarBackground::ZOOM_LEVEL_COUNT,
                  "the background needs a grid density for every zoom level");
    static constexpr int NOT_SHADED = -1;
    // The zoom legend's top, above the bottom of the radar's range, on rectangular panels.
    static constexpr int LEGEND_ABOVE_RANGE_PX = 14;
    int _zoomIndex = 2; // Default 1km view

    bool _roundPanel;
    RadarBackground::Layout _layout = {};
    ScreenLayout::Rect _safeArea = {};
    std::unique_ptr<RadarBackground::RadialTable> _table;
    RadarBackground::StripCache _background;
    int _shadedZoom = NOT_SHADED;

    void drawBalls(LGFX_Sprite& canvas, int cx, int cy, int r, const GpsFix& player, const std::vector<Ball>& balls, float rotation);
    void drawNearestBallIndicator(LGFX_Sprite& canvas, int cx, int cy, int r, const GpsFix& player, const std::vector<Ball>& balls, float rotation);
    void drawNorthIndicator(LGFX_Sprite& canvas, int cx, int cy, int r, float rotation);
    void drawZoomLegend(LGFX_Sprite& canvas, int cx, int cy, int r);
};

#endif // RADAR_RENDERER_H
