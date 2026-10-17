#include "radar_background.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace RadarBackground {

namespace {

constexpr int32_t Q8 = 1 << 8;
constexpr int32_t Q15 = 1 << 15;
constexpr uint64_t Q32 = uint64_t(1) << 32;

float clamp01(float value) {
    return value < 0 ? 0 : (value > 1 ? 1 : value);
}

uint8_t clampChannel(float value) {
    return value <= 0 ? 0 : (value >= 255 ? 255 : uint8_t(value));
}

// Undoes the barrel map s = f(1 + K f^2) by Newton's method.
float flatRadiusOf(float screenRadius) {
    float flat = screenRadius;
    for (int i = 0; i < 6; i++) {
        flat -= (flat * (1 + BARREL_K * flat * flat) - screenRadius) / (1 + 3 * BARREL_K * flat * flat);
    }
    return flat;
}

uint16_t packBigEndian(int r, int g, int b) {
    uint16_t rgb565 = uint16_t(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    return uint16_t((rgb565 >> 8) | (rgb565 << 8));
}

Rgb unpackBigEndian(uint16_t pixel) {
    uint16_t rgb565 = uint16_t((pixel >> 8) | (pixel << 8));
    uint8_t r5 = rgb565 >> 11, g6 = (rgb565 >> 5) & 0x3F, b5 = rgb565 & 0x1F;
    return {uint8_t((r5 << 3) | (r5 >> 2)), uint8_t((g6 << 2) | (g6 >> 4)), uint8_t((b5 << 3) | (b5 >> 2))};
}

// Moves `channel` towards `target` by amountQ8 / 256, rounded.
int32_t mixQ8(int32_t channel, int32_t target, int32_t amountQ8) {
    return channel + (((target - channel) * amountQ8 + Q8 / 2) >> 8);
}

struct ChannelsQ {
    int32_t r, g, b;
};

void mixInto(ChannelsQ& colour, Rgb target, int32_t amountQ8) {
    colour.r = mixQ8(colour.r, target.r, amountQ8);
    colour.g = mixQ8(colour.g, target.g, amountQ8);
    colour.b = mixQ8(colour.b, target.b, amountQ8);
}

struct Colour {
    float r, g, b;
};

Colour colourOf(Rgb rgb) {
    return {float(rgb.r), float(rgb.g), float(rgb.b)};
}

Colour mix(Colour from, Colour to, float amount) {
    return {from.r + (to.r - from.r) * amount, from.g + (to.g - from.g) * amount,
            from.b + (to.b - from.b) * amount};
}

// The fraction of a pixel, centred `distance` from the centre, that lies beyond a circle
// of radius `boundary`: the anti-aliasing of every round edge.
float coverageBeyond(float distance, float boundary) {
    return clamp01(distance - boundary + 0.5f);
}

// Everything shadeRows derives once from the layout and zoom. Distances are in
// half-pixels, so a pixel's centre lies on whole numbers either side of the centre's.
struct Shading {
    uint64_t indexScale;  // squared distance to table index, Q32
    int32_t gridOffsetQ8;
    int32_t gridStepQ8;
    int32_t crossArmQ8;
    int32_t solidSq;      // within this, a pixel is wholly glass
    int32_t outerSq;      // beyond this, a pixel is wholly black
    int32_t highlightX, highlightY, highlightReach;
    uint64_t highlightAlongScale, highlightAcrossScale;  // Q32
};

Shading shadingFor(const Layout& layout, int zoom) {
    float rangeHalf = 2.0f * layout.rangeRadius;
    float extend = 1.0f / (1.0f + BARREL_K);
    float reachHalf = RADIAL_TABLE_REACH * rangeHalf;
    float alongHalf = HIGHLIGHT_ALONG_RIM * rangeHalf;
    float acrossHalf = HIGHLIGHT_ACROSS_RIM * rangeHalf;
    int32_t solid = 2 * layout.screenRadius - 1;
    int32_t outer = 2 * (layout.screenRadius + (layout.bezel ? BEZEL_PX : 0)) + 1;

    Shading shading;
    shading.indexScale = uint64_t(double(RADIAL_STEPS - 1) * double(Q32) / (double(reachHalf) * reachHalf));
    shading.gridOffsetQ8 = int32_t(lroundf(rangeHalf * extend * Q8));
    shading.gridStepQ8 = int32_t(lroundf(rangeHalf * 2.0f * extend / GRID_CELLS[zoom] * Q8));
    shading.crossArmQ8 = int32_t(lroundf(CENTRE_CROSS_ARM * rangeHalf * Q8));
    shading.solidSq = solid * solid;
    shading.outerSq = outer * outer;
    shading.highlightX = int32_t(lroundf(HIGHLIGHT_CENTRE_X * rangeHalf));
    shading.highlightY = int32_t(lroundf(HIGHLIGHT_CENTRE_Y * rangeHalf));
    shading.highlightReach = int32_t(ceilf(alongHalf));
    // The ellipse's axes run at 45 degrees, so its two terms are (p + w)^2 / 2a^2 and
    // (p - w)^2 / 2b^2 for an offset (p, w) from its centre.
    shading.highlightAlongScale = uint64_t(double(Q32) / (2.0 * alongHalf * alongHalf));
    shading.highlightAcrossScale = uint64_t(double(Q32) / (2.0 * acrossHalf * acrossHalf));
    return shading;
}

// Q8 screen pixels from a flat coordinate (Q8 half-pixels) to the nearest grid line.
int32_t gridLineDistanceQ8(int32_t flatQ8, const Shading& shading, uint16_t pxPerFlatQ15) {
    int32_t into = (flatQ8 + shading.gridOffsetQ8) % shading.gridStepQ8;
    if (into < 0) into += shading.gridStepQ8;
    int32_t fromLine = into < shading.gridStepQ8 - into ? into : shading.gridStepQ8 - into;
    // >> 15 for Q15, >> 1 for half-pixels to pixels.
    return int32_t((int64_t(fromLine) * pxPerFlatQ15) >> 16);
}

int32_t coverageQ8(int32_t distanceQ8) {
    return distanceQ8 >= Q8 ? 0 : Q8 - distanceQ8;
}

int32_t highlightQ8(int32_t du, int32_t dv, const Shading& shading) {
    int32_t p = du - shading.highlightX, w = dv - shading.highlightY;
    if (abs(p) > shading.highlightReach || abs(w) > shading.highlightReach) return 0;
    uint64_t q = uint64_t(int64_t(p + w) * (p + w)) * shading.highlightAlongScale +
                 uint64_t(int64_t(p - w) * (p - w)) * shading.highlightAcrossScale;
    if (q >= Q32) return 0;
    uint64_t insideQ16 = (Q32 - q) >> 16;
    constexpr uint64_t STRENGTH_Q8 = uint64_t(HIGHLIGHT_STRENGTH * Q8 + 0.5f);
    return int32_t((STRENGTH_Q8 * insideQ16 * insideQ16 + (Q32 >> 1)) >> 32);
}

// The glass at a pixel inside the table's reach, in 8-bit channels.
ChannelsQ shadeGlass(int32_t du, int32_t dv, int32_t distanceSq, const Shading& shading, const RadialTable& table) {
    uint32_t index = uint32_t((uint64_t(distanceSq) * shading.indexScale + (Q32 >> 1)) >> 32);
    const RadialEntry& entry = table.entries[index];
    int32_t flatU = (du * entry.toFlatQ15) >> 7;  // Q15 to Q8
    int32_t flatV = (dv * entry.toFlatQ15) >> 7;
    int32_t gridU = gridLineDistanceQ8(flatU, shading, entry.pxPerFlatQ15);
    int32_t gridV = gridLineDistanceQ8(flatV, shading, entry.pxPerFlatQ15);
    // The cross is short and near the centre, where the bulge is negligible, so it is
    // measured on the screen: each arm a straight segment with square ends.
    int32_t acrossQ8 = abs(du) * Q8, upQ8 = abs(dv) * Q8;
    int32_t fromVerticalArm = acrossQ8 > upQ8 - shading.crossArmQ8 ? acrossQ8 : upQ8 - shading.crossArmQ8;
    int32_t fromHorizontalArm = upQ8 > acrossQ8 - shading.crossArmQ8 ? upQ8 : acrossQ8 - shading.crossArmQ8;
    int32_t fromCross = fromVerticalArm < fromHorizontalArm ? fromVerticalArm : fromHorizontalArm;
    int32_t crossQ8 = fromCross / 2;  // half-pixels to pixels

    ChannelsQ colour = {entry.gradient.r, entry.gradient.g, entry.gradient.b};
    mixInto(colour, GRID_LINE, coverageQ8(gridU < gridV ? gridU : gridV));
    mixInto(colour, CENTRE_CROSS, coverageQ8(crossQ8));
    mixInto(colour, HIGHLIGHT, highlightQ8(du, dv, shading));
    return colour;
}

// Pixels from the centre beyond which the marker, its outline and its shadow cannot reach.
constexpr int MARKER_REACH_PX = int(MARKER_CIRCUMRADIUS_PX + MARKER_OUTLINE_PX + MARKER_SHADOW_OFFSET_PX) + 2;

// Pixels from the marker's edge, negative inside. The farthest of the three edges' lines
// keeps the corners sharp.
float markerEdgeDistance(float x, float y) {
    constexpr float SIN_60 = 0.8660254f;
    constexpr float INRADIUS_PX = MARKER_CIRCUMRADIUS_PX / 2;
    float beyondBase = y;
    float beyondLeft = -SIN_60 * x - 0.5f * y;
    float beyondRight = SIN_60 * x - 0.5f * y;
    return fmaxf(beyondBase, fmaxf(beyondLeft, beyondRight)) - INRADIUS_PX;
}

// Lays the marker, `dx`, `dy` pixels from the centre, over the glass. It covers a few
// hundred pixels once per zoom, so float costs nothing that matters even without an FPU.
void shadeMarker(ChannelsQ& glass, int dx, int dy) {
    float shadowEdge = markerEdgeDistance(dx + MARKER_SHADOW_OFFSET_PX, dy - MARKER_SHADOW_OFFSET_PX);
    float edge = markerEdgeDistance(float(dx), float(dy));
    Colour black = {0, 0, 0};
    Colour pixel = {float(glass.r), float(glass.g), float(glass.b)};
    pixel = mix(pixel, black, MARKER_SHADOW_STRENGTH * clamp01(MARKER_OUTLINE_PX + 0.5f - shadowEdge));
    pixel = mix(pixel, colourOf(GRID_LINE), clamp01(MARKER_OUTLINE_PX + 0.5f - edge));
    pixel = mix(pixel, colourOf(MARKER), clamp01(0.5f - edge));
    glass = {clampChannel(pixel.r + 0.5f), clampChannel(pixel.g + 0.5f), clampChannel(pixel.b + 0.5f)};
}

// The bezel's body, a rounded ring whose outer slope faces the light where the ring
// faces the upper right, and whose inner slope faces it on the opposite side.
Colour bezelBody(float distance, float bodyStart, float facingLight) {
    float across = clamp01((distance - bodyStart) / BEZEL_BODY_PX);
    float light = 1 + BEVEL_STRENGTH * -cosf(float(M_PI) * across) * facingLight;
    Colour body = colourOf(BEZEL_BODY);
    return {fminf(255, body.r * light), fminf(255, body.g * light), fminf(255, body.b * light)};
}

// A pixel on or beyond the glass's edge: the rim anti-aliased into black, or into the
// bezel's rings. Only these pixels take a square root (ADR-0012).
uint16_t shadeEdge(int32_t du, int32_t dv, int32_t distanceSq, const Layout& layout, const Shading& shading,
                   const RadialTable& table) {
    float distance = sqrtf(float(distanceSq)) / 2;
    float rim = float(layout.screenRadius);
    Colour black = {0, 0, 0};
    Colour pixel = black;
    if (distance < rim + 0.5f) {
        ChannelsQ glass = shadeGlass(du, dv, distanceSq, shading, table);
        pixel = {float(glass.r), float(glass.g), float(glass.b)};
    }
    if (!layout.bezel) {
        pixel = mix(pixel, black, coverageBeyond(distance, rim));
    } else {
        float bodyStart = rim + BEZEL_INNER_RING_PX;
        float outlineStart = bodyStart + BEZEL_BODY_PX;
        // How squarely this point of the ring faces the light in the upper right.
        float facingLight = distance > 0 ? float(du - dv) / (2 * sqrtf(2) * distance) : 0;
        pixel = mix(pixel, colourOf(BEZEL_INNER_RING), coverageBeyond(distance, rim));
        pixel = mix(pixel, bezelBody(distance, bodyStart, facingLight), coverageBeyond(distance, bodyStart));
        pixel = mix(pixel, colourOf(BEZEL_OUTLINE), coverageBeyond(distance, outlineStart));
        pixel = mix(pixel, black, coverageBeyond(distance, outlineStart + BEZEL_OUTLINE_PX));
    }
    return packBigEndian(clampChannel(pixel.r), clampChannel(pixel.g), clampChannel(pixel.b));
}

}  // namespace

Layout layoutFor(int width, int height, bool roundPanel) {
    int cx = width / 2;
    int cy = height / 2;
    int rangeRadius = height / 2 - BEZEL_PX;
    if (roundPanel) return {width, height, cx, cy, rangeRadius, height / 2, false};
    return {width, height, cx, cy, rangeRadius, rangeRadius, true};
}

void buildRadialTable(RadialTable& table) {
    for (int i = 0; i < RADIAL_STEPS; i++) {
        float screenRadius = sqrtf(float(i) / (RADIAL_STEPS - 1)) * RADIAL_TABLE_REACH;
        float flatRadius = flatRadiusOf(screenRadius);
        RadialEntry& entry = table.entries[i];
        entry.toFlatQ15 = uint16_t(lroundf((screenRadius > 0 ? flatRadius / screenRadius : 1.0f) * Q15));
        entry.pxPerFlatQ15 = uint16_t(lroundf((1.0f + BARREL_K * flatRadius * flatRadius) * Q15));
        Colour gradient = mix(colourOf(SCREEN_CENTRE), colourOf(SCREEN_EDGE), fminf(screenRadius, 1.0f));
        entry.gradient = {uint8_t(lroundf(gradient.r)), uint8_t(lroundf(gradient.g)), uint8_t(lroundf(gradient.b))};
    }
}

void shadeRows(uint16_t* rows, int firstRow, int endRow, const Layout& layout, int zoom, const RadialTable& table) {
    Shading shading = shadingFor(layout, zoom);
    for (int y = firstRow; y < endRow; y++) {
        int32_t dv = 2 * (y - layout.cy);
        uint16_t* row = rows + size_t(y - firstRow) * layout.width;
        bool rowCrossesMarker = abs(y - layout.cy) <= MARKER_REACH_PX;
        for (int x = 0; x < layout.width; x++) {
            int32_t du = 2 * (x - layout.cx);
            int32_t distanceSq = du * du + dv * dv;
            if (distanceSq >= shading.outerSq) {
                row[x] = 0;
            } else if (distanceSq > shading.solidSq) {
                row[x] = shadeEdge(du, dv, distanceSq, layout, shading, table);
            } else {
                ChannelsQ glass = shadeGlass(du, dv, distanceSq, shading, table);
                // The marker lies well inside the glass, so the edge pixels never need it.
                if (rowCrossesMarker && abs(x - layout.cx) <= MARKER_REACH_PX) {
                    shadeMarker(glass, x - layout.cx, y - layout.cy);
                }
                row[x] = packBigEndian(glass.r, glass.g, glass.b);
            }
        }
    }
}

StripCache::~StripCache() {
    for (uint16_t* strip : _strips) free(strip);
}

bool StripCache::allocate(int width, int height) {
    _width = width;
    for (int i = 0; i <= STRIP_COUNT; i++) _firstRow[i] = height * i / STRIP_COUNT;
    for (int i = 0; i < STRIP_COUNT; i++) {
        free(_strips[i]);
        size_t bytes = size_t(width) * (_firstRow[i + 1] - _firstRow[i]) * sizeof(uint16_t);
        _strips[i] = static_cast<uint16_t*>(malloc(bytes));
        if (!_strips[i]) return false;
    }
    return true;
}

void StripCache::shade(const Layout& layout, int zoom, const RadialTable& table) {
    for (int i = 0; i < STRIP_COUNT; i++) shadeRows(_strips[i], _firstRow[i], _firstRow[i + 1], layout, zoom, table);
}

void StripCache::copyInto(uint16_t* target) const {
    for (int i = 0; i < STRIP_COUNT; i++) {
        size_t rows = size_t(_firstRow[i + 1] - _firstRow[i]);
        memcpy(target + size_t(_width) * _firstRow[i], _strips[i], size_t(_width) * rows * sizeof(uint16_t));
    }
}

namespace {

constexpr Rgb BLIP_CENTRE = {255, 255, 255};
constexpr Rgb BLIP_CORE = {255, 245, 150};
constexpr Rgb BLIP_HALO = {255, 215, 60};
// Within this many pixels the blip is opaque; beyond it the halo fades out.
constexpr float BLIP_SOLID_PX = 2.5f;

struct BlipShade {
    Rgb colour;
    int32_t alphaQ8;
};

// The blip's look at each whole squared distance from its centre.
struct BlipProfile {
    BlipShade bySquaredDistance[BLIP_RADIUS_PX * BLIP_RADIUS_PX + 1];

    BlipProfile() {
        for (int squared = 0; squared <= BLIP_RADIUS_PX * BLIP_RADIUS_PX; squared++) {
            float distance = sqrtf(float(squared));
            Colour colour = mix(colourOf(BLIP_CENTRE), colourOf(BLIP_CORE), clamp01((distance - 0.5f) / 1.5f));
            colour = mix(colour, colourOf(BLIP_HALO), clamp01((distance - 2.0f) / 2.0f));
            float fade = 1 - clamp01((distance - BLIP_SOLID_PX) / (BLIP_RADIUS_PX + 0.5f - BLIP_SOLID_PX));
            BlipShade& shade = bySquaredDistance[squared];
            shade.colour = {clampChannel(colour.r + 0.5f), clampChannel(colour.g + 0.5f), clampChannel(colour.b + 0.5f)};
            shade.alphaQ8 = int32_t(lroundf(fade * fade * Q8));
        }
    }
};

}  // namespace

void blendBlip(uint16_t* pixels, int width, int height, int x, int y) {
    static const BlipProfile profile;
    int top = y - BLIP_RADIUS_PX < 0 ? 0 : y - BLIP_RADIUS_PX;
    int bottom = y + BLIP_RADIUS_PX >= height ? height - 1 : y + BLIP_RADIUS_PX;
    int left = x - BLIP_RADIUS_PX < 0 ? 0 : x - BLIP_RADIUS_PX;
    int right = x + BLIP_RADIUS_PX >= width ? width - 1 : x + BLIP_RADIUS_PX;
    for (int py = top; py <= bottom; py++) {
        for (int px = left; px <= right; px++) {
            int squared = (px - x) * (px - x) + (py - y) * (py - y);
            if (squared > BLIP_RADIUS_PX * BLIP_RADIUS_PX) continue;
            const BlipShade& shade = profile.bySquaredDistance[squared];
            uint16_t& pixel = pixels[size_t(py) * width + px];
            Rgb below = unpackBigEndian(pixel);
            ChannelsQ under = {below.r, below.g, below.b};
            mixInto(under, shade.colour, shade.alphaQ8);
            pixel = packBigEndian(under.r, under.g, under.b);
        }
    }
}

}  // namespace RadarBackground
