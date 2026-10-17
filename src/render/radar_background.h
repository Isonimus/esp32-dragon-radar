#ifndef RADAR_BACKGROUND_H
#define RADAR_BACKGROUND_H

#include <stddef.h>
#include <stdint.h>

/**
 * The radar's static layer (ADR-0012): the lit screen, its barrel grid and centre cross,
 * the player's marker, the glass highlight and, on rectangular panels, the bezel. It is shaded per pixel in
 * integer maths once per zoom change and cached; each frame only copies it.
 *
 * Free of Arduino and LovyanGFX, so the native tests run it. Pixels are big-endian
 * RGB565, the byte order of a 16-bit LovyanGFX sprite (slice 0011).
 */
namespace RadarBackground {

constexpr int ZOOM_LEVEL_COUNT = 5;
// Grid cells across the screen per zoom level: sparse zoomed in, dense zoomed out.
constexpr int GRID_CELLS[ZOOM_LEVEL_COUNT] = {4, 6, 8, 10, 12};

// The grid bulges like a CRT: a flat grid radius f lands at screen radius f(1 + K f^2),
// in units of the range radius.
constexpr float BARREL_K = -0.08f;

// The bezel's rings, outwards from the screen's edge, in pixels. Together they fill the
// 5 px the rectangular panels leave above and below the screen.
constexpr float BEZEL_INNER_RING_PX = 1.0f;
constexpr float BEZEL_BODY_PX = 3.0f;
constexpr float BEZEL_OUTLINE_PX = 1.0f;
constexpr int BEZEL_PX = 5;
// How strongly the bevel's slopes catch or lose the light, as a fraction of the body's
// colour.
constexpr float BEVEL_STRENGTH = 0.35f;

// The glass highlight: an ellipse in the upper right, its long axis along the rim, in
// units of the range radius. The bevel is lit from the same direction, so the two read
// as one light source.
constexpr float HIGHLIGHT_CENTRE_X = 0.40f;
constexpr float HIGHLIGHT_CENTRE_Y = -0.48f;
constexpr float HIGHLIGHT_ALONG_RIM = 0.36f;
constexpr float HIGHLIGHT_ACROSS_RIM = 0.12f;
constexpr float HIGHLIGHT_STRENGTH = 0.25f;

// Each arm of the bright centre cross, from the centre, in units of the range radius. As
// on the anime device it is short; the grid's own centre lines run on past it.
constexpr float CENTRE_CROSS_ARM = 0.27f;

// The player's marker on the cross: a triangle pointing up, outlined in the grid's dark
// green as on the anime device. Its shadow falls down and to the left, away from the
// light in the upper right. It never moves or turns, so it is shaded here (invariant #6).
constexpr float MARKER_CIRCUMRADIUS_PX = 6.0f;
constexpr float MARKER_OUTLINE_PX = 1.0f;
constexpr float MARKER_SHADOW_OFFSET_PX = 1.0f;
// How far the shadow darkens the glass towards black.
constexpr float MARKER_SHADOW_STRENGTH = 0.35f;

struct Rgb {
    uint8_t r, g, b;
};
constexpr Rgb SCREEN_CENTRE = {60, 190, 60};
constexpr Rgb SCREEN_EDGE = {10, 110, 20};
constexpr Rgb GRID_LINE = {0, 70, 10};
constexpr Rgb CENTRE_CROSS = {150, 235, 140};
constexpr Rgb HIGHLIGHT = {255, 255, 255};
constexpr Rgb MARKER = {255, 0, 0};
constexpr Rgb BEZEL_INNER_RING = {40, 46, 40};
// Light grey, slightly green, like the anime device's case.
constexpr Rgb BEZEL_BODY = {196, 204, 192};
constexpr Rgb BEZEL_OUTLINE = {24, 26, 24};

// Where the radar sits on a panel. Pixel (cx, cy) is the centre.
struct Layout {
    int width;
    int height;
    int cx;
    int cy;
    // Pixels from the centre to the radar's range: the grid, the blips and the markers
    // scale with it.
    int rangeRadius;
    // Pixels from the centre to the glass's edge.
    int screenRadius;
    bool bezel;
};

// A rectangular panel frames the screen in a bezel. A round panel's own edge is the
// screen's rim, so the glass runs out to it and no bezel is drawn.
Layout layoutFor(int width, int height, bool roundPanel);

// Everything the shading needs that depends only on the distance from the centre,
// indexed by the squared distance, normalised to the range radius.
constexpr int RADIAL_STEPS = 1024;
// The table reaches this far, in range radii: past the round panel's glass edge.
constexpr float RADIAL_TABLE_REACH = 1.1f;
struct RadialEntry {
    uint16_t toFlatQ15;     // flat grid radius / screen radius
    uint16_t pxPerFlatQ15;  // screen pixels per flat pixel across a grid line
    Rgb gradient;
};
struct RadialTable {
    RadialEntry entries[RADIAL_STEPS];
};
void buildRadialTable(RadialTable& table);

// Shades rows [firstRow, endRow) into `rows`, whose first row is firstRow.
void shadeRows(uint16_t* rows, int firstRow, int endRow, const Layout& layout, int zoom,
               const RadialTable& table);

// The shaded layer as horizontal strips: on the custom board, a second screen-sized
// buffer does not fit in one block next to the canvas, but eight strips do (ADR-0012).
class StripCache {
public:
    static constexpr int STRIP_COUNT = 8;

    StripCache() = default;
    StripCache(const StripCache&) = delete;
    StripCache& operator=(const StripCache&) = delete;
    ~StripCache();

    // False if any strip cannot be allocated; the cache is then unusable, and the caller
    // must not shade or copy it.
    bool allocate(int width, int height);
    void shade(const Layout& layout, int zoom, const RadialTable& table);
    void copyInto(uint16_t* target) const;

private:
    uint16_t* _strips[STRIP_COUNT] = {};
    int _firstRow[STRIP_COUNT + 1] = {};
    int _width = 0;
};

// Pixels from a blip's centre to the edge of its glow.
constexpr int BLIP_RADIUS_PX = 6;

// Blends a glowing blip centred on pixel (x, y) into a width x height buffer: a halo that
// fades into what is underneath, a pale yellow core and a white centre. Only pixels
// inside the buffer are written.
void blendBlip(uint16_t* pixels, int width, int height, int x, int y);

}  // namespace RadarBackground

#endif  // RADAR_BACKGROUND_H
