#include "radar_renderer.h"
#include "../math/coord_math.h"

#include <new>

// Dragon Ball Radar palette. The screen, grid, player marker and bezel are shaded in
// radar_background.
static constexpr uint16_t RADAR_BALL   = 0xFFE0;  // Yellow (TFT_YELLOW)
static constexpr uint16_t RADAR_TRI    = 0xF800;  // Red (TFT_RED)

RadarRenderer::RadarRenderer(bool roundPanel) : _roundPanel(roundPanel) {}

bool RadarRenderer::begin(const LGFX_Sprite& canvas) {
    _layout = RadarBackground::layoutFor(canvas.width(), canvas.height(), _roundPanel);
    _safeArea = ScreenLayout::safeArea(canvas.width(), canvas.height(), _roundPanel);
    _shadedZoom = NOT_SHADED;
    _table.reset(new (std::nothrow) RadarBackground::RadialTable);
    if (!_table) return false;
    RadarBackground::buildRadialTable(*_table);
    return _background.allocate(_layout.width, _layout.height);
}

void RadarRenderer::render(LGFX_Sprite& canvas, const GpsFix& playerFix, const std::vector<Ball>& balls, bool blipsLit, float rotation) {
    int cx = _layout.cx;
    int cy = _layout.cy;
    int r = _layout.rangeRadius;

    // Everything static is cached; it changes only with the grid density (ADR-0012).
    if (_shadedZoom != _zoomIndex) {
        _background.shade(_layout, _zoomIndex, *_table);
        _shadedZoom = _zoomIndex;
    }
    _background.copyInto(static_cast<uint16_t*>(canvas.getBuffer()));

    if (blipsLit) drawBalls(canvas, cx, cy, r, playerFix, balls, rotation);
    // Off-screen nearest ball indicator
    drawNearestBallIndicator(canvas, cx, cy, r, playerFix, balls, rotation);
    
    // North indicator triangle
    drawNorthIndicator(canvas, cx, cy, r, rotation);

    drawZoomLegend(canvas, cx, cy, r);
}

void RadarRenderer::drawZoomLegend(LGFX_Sprite& canvas, int cx, int cy, int r) {
    char legend[16];
    if (getZoom() >= 1000)
        snprintf(legend, sizeof(legend), "%.0fkm", getZoom() / 1000.0f);
    else
        snprintf(legend, sizeof(legend), "%.0fm", getZoom());

    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(1);
    if (_roundPanel) {
        // The radar's bounding square runs past the round panel, so the legend sits at
        // the bottom centre of the safe area instead (ADR-0014).
        canvas.setTextDatum(BC_DATUM);
        canvas.drawString(legend, _safeArea.centreX(), _safeArea.bottom());
    } else {
        // Bottom-left of the radar's bounding square, beside the screen.
        canvas.setTextDatum(TL_DATUM);
        canvas.drawString(legend, cx - r, cy + r - LEGEND_ABOVE_RANGE_PX);
    }
    canvas.setTextDatum(TL_DATUM);
}

void RadarRenderer::drawNorthIndicator(LGFX_Sprite& canvas, int cx, int cy, int r, float rotation) {
    // Small red triangle on the inner edge pointing to magnetic North (0°)
    // Angle 0° is North, but on screen 0° is Right (+X), so subtract 90 to get Top (-Y)
    float northRad = (-90.0f - rotation) * (M_PI / 180.0f);
    
    int tx = cx + std::cos(northRad) * (r - 2);
    int ty = cy + std::sin(northRad) * (r - 2);
    
    int triSize = 4;
    int x0 = tx + std::cos(northRad) * triSize;
    int y0 = ty + std::sin(northRad) * triSize;
    int x1 = tx + std::cos(northRad + 2.094f) * triSize;
    int y1 = ty + std::sin(northRad + 2.094f) * triSize;
    int x2 = tx + std::cos(northRad - 2.094f) * triSize;
    int y2 = ty + std::sin(northRad - 2.094f) * triSize;
    
    canvas.fillTriangle(x0, y0, x1, y1, x2, y2, RADAR_TRI); // Red
}

void RadarRenderer::zoomIn() {
    if (_zoomIndex > 0) _zoomIndex--;
}

void RadarRenderer::zoomOut() {
    if (_zoomIndex < NUM_ZOOM_LEVELS - 1) _zoomIndex++;
}

void RadarRenderer::drawBalls(LGFX_Sprite& canvas, int cx, int cy, int r, const GpsFix& player, const std::vector<Ball>& balls, float rotation) {
    if (!player.hasPosition) return;

    for (const auto& ball : balls) {
        if (ball.collected) continue;

        double dist = CoordMath::haversineDistance(player.lat, player.lon, ball.lat, ball.lon);
        if (dist > getZoom()) continue;

        double bearing = CoordMath::calculateBearing(player.lat, player.lon, ball.lat, ball.lon);
        
        float radarDist = (dist / getZoom()) * r;
        float bearingRad = (bearing - 90.0f - rotation) * (M_PI / 180.0f); 
        
        int bx = cx + std::cos(bearingRad) * radarDist;
        int by = cy + std::sin(bearingRad) * radarDist;
        
        RadarBackground::blendBlip(static_cast<uint16_t*>(canvas.getBuffer()), canvas.width(), canvas.height(), bx, by);

        // Draw ball number if zoom is 1km or less, clear of the glow
        if (getZoom() <= 1000.0f) {
            canvas.setTextColor(RADAR_BALL);
            canvas.setTextSize(1);
            canvas.setCursor(bx + RadarBackground::BLIP_RADIUS_PX, by - RadarBackground::BLIP_RADIUS_PX);
            canvas.print(ball.id);
        }
    }
}

void RadarRenderer::drawNearestBallIndicator(LGFX_Sprite& canvas, int cx, int cy, int r, const GpsFix& player, const std::vector<Ball>& balls, float rotation) {
    if (!player.hasPosition) return;
    
    // Check if ANY uncollected ball is visible on screen
    bool anyVisible = false;
    double nearestDist = 1e12;
    double nearestBearing = 0;
    
    for (const auto& ball : balls) {
        if (ball.collected) continue;
        
        double dist = CoordMath::haversineDistance(player.lat, player.lon, ball.lat, ball.lon);
        
        if (dist <= getZoom()) {
            anyVisible = true;
            break; // At least one ball is on screen, no indicator needed
        }
        
        // Track nearest off-screen ball
        if (dist < nearestDist) {
            nearestDist = dist;
            nearestBearing = CoordMath::calculateBearing(player.lat, player.lon, ball.lat, ball.lon);
        }
    }
    
    // Only show indicator when ALL balls are off-screen
    if (anyVisible || nearestDist > 1e11) return;
    
    // Position the indicator triangle on the inner edge of the radar circle
    float bearingRad = (nearestBearing - 90.0f - rotation) * (M_PI / 180.0f);
    int edgeOffset = 8; // pixels inward from the edge
    float edgeR = r - edgeOffset;
    int tx = cx + std::cos(bearingRad) * edgeR;
    int ty = cy + std::sin(bearingRad) * edgeR;
    
    // Draw equilateral triangle (size 5) pointing toward the ball (outward)
    int triSize = 5;
    int x0 = tx + std::cos(bearingRad) * triSize;
    int y0 = ty + std::sin(bearingRad) * triSize;
    int x1 = tx + std::cos(bearingRad + 2.094f) * triSize;
    int y1 = ty + std::sin(bearingRad + 2.094f) * triSize;
    int x2 = tx + std::cos(bearingRad - 2.094f) * triSize;
    int y2 = ty + std::sin(bearingRad - 2.094f) * triSize;
    
    canvas.fillTriangle(x0, y0, x1, y1, x2, y2, RADAR_BALL); // Yellow

    // Draw distance text next to the triangle
    char distBuf[16];
    if (nearestDist >= 1000.0) sprintf(distBuf, "%.1fkm", nearestDist / 1000.0);
    else sprintf(distBuf, "%.0fm", (float)nearestDist);

    canvas.setTextDatum(MC_DATUM);
    canvas.setTextColor(RADAR_BALL);
    canvas.setTextSize(1);
    // Position text slightly inward from the triangle
    int tx_text = tx - std::cos(bearingRad) * 20;
    int ty_text = ty - std::sin(bearingRad) * 20;
    canvas.drawString(distBuf, tx_text, ty_text);
}

void RadarRenderer::drawDragonBall(LGFX_Sprite& canvas, int x, int y, int r, uint8_t stars, bool collected) {
    uint16_t ballColor = collected ? 0xFDA0 : canvas.color565(243, 147, 7);
    uint16_t starColor = collected ? 0x8000 : TFT_RED;
    uint16_t borderColor = collected ? TFT_BLACK : 0xFDA0;
    
    canvas.fillCircle(x, y, r, ballColor);
    canvas.drawCircle(x, y, r, borderColor);
    
    if (stars < 1 || stars > 7) return;

    int sr = r / 5;
    if (sr < 1) sr = 1;

    switch (stars) {
        case 1:
            canvas.fillCircle(x, y, sr, starColor);
            break;
        case 2:
            canvas.fillCircle(x, y - r/3, sr, starColor);
            canvas.fillCircle(x, y + r/3, sr, starColor);
            break;
        case 3:
            canvas.fillCircle(x, y - r/3, sr, starColor);
            canvas.fillCircle(x - r/3, y + r/4, sr, starColor);
            canvas.fillCircle(x + r/3, y + r/4, sr, starColor);
            break;
        case 4:
            canvas.fillCircle(x - r/3, y - r/3, sr, starColor);
            canvas.fillCircle(x + r/3, y - r/3, sr, starColor);
            canvas.fillCircle(x - r/3, y + r/3, sr, starColor);
            canvas.fillCircle(x + r/3, y + r/3, sr, starColor);
            break;
        case 5:
            canvas.fillCircle(x, y, sr, starColor);
            canvas.fillCircle(x - r/2.5, y - r/2.5, sr, starColor);
            canvas.fillCircle(x + r/2.5, y - r/2.5, sr, starColor);
            canvas.fillCircle(x - r/2.5, y + r/2.5, sr, starColor);
            canvas.fillCircle(x + r/2.5, y + r/2.5, sr, starColor);
            break;
        case 6:
            for (int i = 0; i < 3; i++) {
                canvas.fillCircle(x - r/3, y - r/2 + i*r/2, sr, starColor);
                canvas.fillCircle(x + r/3, y - r/2 + i*r/2, sr, starColor);
            }
            break;
        case 7:
            canvas.fillCircle(x, y, sr, starColor);
            for (int i = 0; i < 6; i++) {
                float a = i * 60 * (M_PI/180.0);
                canvas.fillCircle(x + cos(a)*r/2, y + sin(a)*r/2, sr, starColor);
            }
            break;
    }
}
