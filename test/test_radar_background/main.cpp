#include <unity.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "../../src/render/radar_background.h"
#include "../../src/render/radar_background.cpp"

using namespace RadarBackground;

// One RGB565 step of the coarsest channels, in 8-bit units: ADR-0012's measured bound.
static const int MAX_CHANNEL_ERROR = 8;
static const int ROUND_SIZE = 240;
static const int RECT_WIDTH = 240;
static const int RECT_HEIGHT = 135;

static RadialTable table;

void setUp() {}
void tearDown() {}

struct Channels {
    int r, g, b;
};

static Channels unpack(uint16_t bigEndian) {
    uint16_t rgb565 = uint16_t((bigEndian >> 8) | (bigEndian << 8));
    return {(rgb565 >> 11) << 3, ((rgb565 >> 5) & 0x3F) << 2, (rgb565 & 0x1F) << 3};
}

static std::vector<uint16_t> shadeWhole(const Layout& layout, int zoom) {
    std::vector<uint16_t> pixels(size_t(layout.width) * layout.height);
    shadeRows(pixels.data(), 0, layout.height, layout, zoom, table);
    return pixels;
}

// --- The float reference: the layer as specified, with no table and no fixed point. ---

namespace FloatReference {

struct Colour {
    float r, g, b;
};

static Colour colourOf(Rgb rgb) {
    return {float(rgb.r), float(rgb.g), float(rgb.b)};
}

static Colour mix(Colour from, Colour to, float amount) {
    return {from.r + (to.r - from.r) * amount, from.g + (to.g - from.g) * amount,
            from.b + (to.b - from.b) * amount};
}

static float clamp01(float value) {
    return value < 0 ? 0 : (value > 1 ? 1 : value);
}

// Undoes the barrel map s = f(1 + K f^2) by Newton's method.
static float flatRadiusOf(float screenRadius) {
    float flat = screenRadius;
    for (int i = 0; i < 6; i++) {
        flat -= (flat * (1 + BARREL_K * flat * flat) - screenRadius) / (1 + 3 * BARREL_K * flat * flat);
    }
    return flat;
}

static float pxFromGridLine(float flat, float step, float extend, float pxPerUnit) {
    float cell = (flat + extend) / step;
    return fabsf(cell - roundf(cell)) * step * pxPerUnit;
}

static Colour referenceGlass(const Layout& layout, int zoom, float u, float v) {
    float range = float(layout.rangeRadius);
    float s = sqrtf(u * u + v * v);
    float flat = flatRadiusOf(s);
    float toFlat = s > 0 ? flat / s : 1;
    float pxPerUnit = range * (1 + BARREL_K * flat * flat);
    float extend = 1 / (1 + BARREL_K);
    float step = 2 * extend / GRID_CELLS[zoom];
    float fu = u * toFlat, fv = v * toFlat;
    float gridPx = fminf(pxFromGridLine(fu, step, extend, pxPerUnit), pxFromGridLine(fv, step, extend, pxPerUnit));
    // Each arm of the cross is a straight segment through the centre, with square ends.
    float armPx = CENTRE_CROSS_ARM * range;
    float upPx = fabsf(v) * range, acrossPx = fabsf(u) * range;
    float verticalArmPx = fmaxf(acrossPx, upPx - armPx);
    float horizontalArmPx = fmaxf(upPx, acrossPx - armPx);
    float crossPx = fminf(verticalArmPx, horizontalArmPx);

    Colour glass = mix(colourOf(SCREEN_CENTRE), colourOf(SCREEN_EDGE), fminf(s, 1));
    glass = mix(glass, colourOf(GRID_LINE), clamp01(1 - gridPx));
    glass = mix(glass, colourOf(CENTRE_CROSS), clamp01(1 - crossPx));
    float p = u - HIGHLIGHT_CENTRE_X, w = v - HIGHLIGHT_CENTRE_Y;
    float along = (p + w) / sqrtf(2), across = (p - w) / sqrtf(2);
    float q = along * along / (HIGHLIGHT_ALONG_RIM * HIGHLIGHT_ALONG_RIM) +
              across * across / (HIGHLIGHT_ACROSS_RIM * HIGHLIGHT_ACROSS_RIM);
    if (q < 1) glass = mix(glass, colourOf(HIGHLIGHT), HIGHLIGHT_STRENGTH * (1 - q) * (1 - q));
    return glass;
}

static float coverageBeyond(float distance, float boundary) {
    return clamp01(distance - boundary + 0.5f);
}

// Pixels from the marker's nearest edge, negative inside: the farthest of the distances
// beyond each edge's line, measured from the triangle's vertices.
static float markerEdgeDistance(float x, float y) {
    float radius = MARKER_CIRCUMRADIUS_PX;
    float vx[3] = {0, radius * sqrtf(3) / 2, -radius * sqrtf(3) / 2};
    float vy[3] = {-radius, radius / 2, radius / 2};
    float farthest = -1e9f;
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;
        float ex = vx[j] - vx[i], ey = vy[j] - vy[i];
        float length = sqrtf(ex * ex + ey * ey);
        // Clockwise on the screen, so the outward normal is (ey, -ex).
        farthest = fmaxf(farthest, ((x - vx[i]) * ey - (y - vy[i]) * ex) / length);
    }
    return farthest;
}

static Colour withMarker(Colour glass, float dx, float dy) {
    float shadowEdge = markerEdgeDistance(dx + MARKER_SHADOW_OFFSET_PX, dy - MARKER_SHADOW_OFFSET_PX);
    float edge = markerEdgeDistance(dx, dy);
    Colour black = {0, 0, 0};
    glass = mix(glass, black, MARKER_SHADOW_STRENGTH * clamp01(MARKER_OUTLINE_PX + 0.5f - shadowEdge));
    glass = mix(glass, colourOf(GRID_LINE), clamp01(MARKER_OUTLINE_PX + 0.5f - edge));
    return mix(glass, colourOf(MARKER), clamp01(0.5f - edge));
}

static Colour referencePixel(const Layout& layout, int zoom, int x, int y) {
    float dx = float(x - layout.cx), dy = float(y - layout.cy);
    float distance = sqrtf(dx * dx + dy * dy);
    Colour black = {0, 0, 0};
    Colour pixel = distance < layout.screenRadius + 0.5f
        ? withMarker(referenceGlass(layout, zoom, dx / layout.rangeRadius, dy / layout.rangeRadius), dx, dy)
        : black;
    float rim = float(layout.screenRadius);
    if (!layout.bezel) return mix(pixel, black, coverageBeyond(distance, rim));

    float bodyStart = rim + BEZEL_INNER_RING_PX;
    float outlineStart = bodyStart + BEZEL_BODY_PX;
    float across = clamp01((distance - bodyStart) / BEZEL_BODY_PX);
    float facing = distance > 0 ? (dx - dy) / (sqrtf(2) * distance) : 0;
    float light = -cosf(float(M_PI) * across) * facing;
    Colour body = colourOf(BEZEL_BODY);
    body = {fminf(255, body.r * (1 + BEVEL_STRENGTH * light)), fminf(255, body.g * (1 + BEVEL_STRENGTH * light)),
            fminf(255, body.b * (1 + BEVEL_STRENGTH * light))};
    pixel = mix(pixel, colourOf(BEZEL_INNER_RING), coverageBeyond(distance, rim));
    pixel = mix(pixel, body, coverageBeyond(distance, bodyStart));
    pixel = mix(pixel, colourOf(BEZEL_OUTLINE), coverageBeyond(distance, outlineStart));
    return mix(pixel, black, coverageBeyond(distance, outlineStart + BEZEL_OUTLINE_PX));
}

}  // namespace FloatReference


// Truncated to RGB565 the way the shader packs, so only the shading's error remains.
static Channels quantised(FloatReference::Colour colour) {
    return {(int(colour.r) >> 3) << 3, (int(colour.g) >> 2) << 2, (int(colour.b) >> 3) << 3};
}

static void assertMatchesReference(const Layout& layout) {
    for (int zoom = 0; zoom < ZOOM_LEVEL_COUNT; zoom++) {
        std::vector<uint16_t> pixels = shadeWhole(layout, zoom);
        int worst = 0, worstX = 0, worstY = 0;
        for (int y = 0; y < layout.height; y++) {
            for (int x = 0; x < layout.width; x++) {
                Channels shaded = unpack(pixels[size_t(y) * layout.width + x]);
                Channels expected = quantised(FloatReference::referencePixel(layout, zoom, x, y));
                int error = abs(shaded.r - expected.r);
                if (abs(shaded.g - expected.g) > error) error = abs(shaded.g - expected.g);
                if (abs(shaded.b - expected.b) > error) error = abs(shaded.b - expected.b);
                if (error > worst) {
                    worst = error;
                    worstX = x;
                    worstY = y;
                }
            }
        }
        char message[80];
        snprintf(message, sizeof(message), "zoom %d, worst at (%d, %d)", zoom, worstX, worstY);
        TEST_ASSERT_LESS_OR_EQUAL_INT_MESSAGE(MAX_CHANNEL_ERROR, worst, message);
    }
}

// --- Tests ---

void test_layouts_fit_the_panels_and_the_table() {
    Layout round = layoutFor(ROUND_SIZE, ROUND_SIZE, true);
    TEST_ASSERT_EQUAL_INT(ROUND_SIZE / 2, round.cx);
    TEST_ASSERT_EQUAL_INT(ROUND_SIZE / 2, round.screenRadius);
    TEST_ASSERT_EQUAL_INT(ROUND_SIZE / 2 - BEZEL_PX, round.rangeRadius);
    TEST_ASSERT_FALSE(round.bezel);

    Layout rect = layoutFor(RECT_WIDTH, RECT_HEIGHT, false);
    TEST_ASSERT_EQUAL_INT(RECT_WIDTH / 2, rect.cx);
    TEST_ASSERT_EQUAL_INT(RECT_HEIGHT / 2, rect.cy);
    TEST_ASSERT_EQUAL_INT(RECT_HEIGHT / 2 - BEZEL_PX, rect.screenRadius);
    TEST_ASSERT_EQUAL_INT(rect.screenRadius, rect.rangeRadius);
    TEST_ASSERT_TRUE(rect.bezel);
    // The bezel ends inside the panel's top and bottom rows.
    TEST_ASSERT_LESS_OR_EQUAL_INT(rect.cy, rect.screenRadius + BEZEL_PX);

    // Every glass pixel the shader looks up lies inside the radial table.
    TEST_ASSERT_TRUE(round.screenRadius + 1 < RADIAL_TABLE_REACH * round.rangeRadius);
}

void test_round_shading_matches_the_float_reference_at_every_zoom() {
    assertMatchesReference(layoutFor(ROUND_SIZE, ROUND_SIZE, true));
}

void test_rect_shading_matches_the_float_reference_at_every_zoom() {
    assertMatchesReference(layoutFor(RECT_WIDTH, RECT_HEIGHT, false));
}

void test_round_panel_is_black_beyond_the_glass() {
    Layout layout = layoutFor(ROUND_SIZE, ROUND_SIZE, true);
    std::vector<uint16_t> pixels = shadeWhole(layout, 0);
    TEST_ASSERT_EQUAL_HEX16(0, pixels[0]);
    // The glass reaches the panel's sides, so past the rim lies only on the diagonals:
    // here, about 1.6 pixels beyond it.
    int diagonal = int(ceilf((layout.screenRadius + 1.5f) / sqrtf(2)));
    TEST_ASSERT_EQUAL_HEX16(0, pixels[size_t(layout.cy + diagonal) * layout.width + layout.cx + diagonal]);
    // Just inside the rim it is lit glass.
    int x = layout.cx - layout.screenRadius + 2;
    TEST_ASSERT_GREATER_THAN_INT(60, unpack(pixels[size_t(layout.cy) * layout.width + x]).g);
}

void test_rect_panel_frames_the_glass_in_a_grey_bezel() {
    Layout layout = layoutFor(RECT_WIDTH, RECT_HEIGHT, false);
    std::vector<uint16_t> pixels = shadeWhole(layout, 0);
    // The middle of the bezel's body, left of the centre: light grey, close to neutral.
    int bodyMiddle = layout.cx - layout.screenRadius - int(BEZEL_INNER_RING_PX + BEZEL_BODY_PX / 2);
    Channels body = unpack(pixels[size_t(layout.cy) * layout.width + bodyMiddle]);
    TEST_ASSERT_GREATER_THAN_INT(140, body.r);
    TEST_ASSERT_GREATER_THAN_INT(140, body.g);
    TEST_ASSERT_GREATER_THAN_INT(140, body.b);
    TEST_ASSERT_LESS_THAN_INT(24, abs(body.g - body.r));
    // Beyond the bezel, black.
    int beyond = layout.cx - layout.screenRadius - BEZEL_PX - 2;
    TEST_ASSERT_EQUAL_HEX16(0, pixels[size_t(layout.cy) * layout.width + beyond]);
}

void test_bevel_is_lit_from_the_upper_right() {
    Layout layout = layoutFor(RECT_WIDTH, RECT_HEIGHT, false);
    std::vector<uint16_t> pixels = shadeWhole(layout, 0);
    // The outer slope of the body, at 45 degrees up-right and down-left of the centre.
    float outerSlope = layout.screenRadius + BEZEL_INNER_RING_PX + BEZEL_BODY_PX - 0.5f;
    int offset = int(lroundf(outerSlope / sqrtf(2)));
    Channels upperRight = unpack(pixels[size_t(layout.cy - offset) * layout.width + layout.cx + offset]);
    Channels lowerLeft = unpack(pixels[size_t(layout.cy + offset) * layout.width + layout.cx - offset]);
    TEST_ASSERT_GREATER_THAN_INT(lowerLeft.g, upperRight.g);
}

void test_centre_cross_is_brighter_than_the_grid_and_the_screen() {
    Layout layout = layoutFor(ROUND_SIZE, ROUND_SIZE, true);
    for (int zoom = 0; zoom < ZOOM_LEVEL_COUNT; zoom++) {
        std::vector<uint16_t> pixels = shadeWhole(layout, zoom);
        // A row across the cross's lower arm, away from the highlight, crossing grid lines.
        int y = layout.cy + int(CENTRE_CROSS_ARM * layout.rangeRadius / 2);
        const uint16_t* row = &pixels[size_t(y) * layout.width];
        int cross = unpack(row[layout.cx]).g;
        int reach = layout.rangeRadius * 9 / 10;
        for (int x = layout.cx - reach; x <= layout.cx + reach; x++) {
            if (abs(x - layout.cx) <= 1) continue;
            TEST_ASSERT_GREATER_THAN_INT(unpack(row[x]).g, cross);
        }
    }
}

void test_centre_cross_ends_short_of_the_rim() {
    Layout layout = layoutFor(ROUND_SIZE, ROUND_SIZE, true);
    std::vector<uint16_t> pixels = shadeWhole(layout, 0);
    // Past the end of the lower arm, the centre column is an ordinary grid line: darker
    // than the screen beside it.
    int y = layout.cy + int(CENTRE_CROSS_ARM * layout.rangeRadius) + 4;
    const uint16_t* row = &pixels[size_t(y) * layout.width];
    TEST_ASSERT_LESS_THAN_INT(unpack(row[layout.cx + 4]).g, unpack(row[layout.cx]).g);
}

static Channels pixelAt(const std::vector<uint16_t>& pixels, const Layout& layout, int dx, int dy) {
    return unpack(pixels[size_t(layout.cy + dy) * layout.width + layout.cx + dx]);
}

void test_marker_is_red_on_the_centre() {
    for (bool round : {true, false}) {
        Layout layout = round ? layoutFor(ROUND_SIZE, ROUND_SIZE, true) : layoutFor(RECT_WIDTH, RECT_HEIGHT, false);
        Channels centre = pixelAt(shadeWhole(layout, 0), layout, 0, 0);
        TEST_ASSERT_GREATER_OR_EQUAL_INT(MARKER.r - MAX_CHANNEL_ERROR, centre.r);
        TEST_ASSERT_LESS_OR_EQUAL_INT(MAX_CHANNEL_ERROR, centre.g);
    }
}

void test_marker_is_outlined_in_the_grid_colour() {
    Layout layout = layoutFor(RECT_WIDTH, RECT_HEIGHT, false);
    std::vector<uint16_t> pixels = shadeWhole(layout, 0);
    // Just past the tip, which sits on the bright cross, the outline covers the pixel.
    int pastTip = int(MARKER_CIRCUMRADIUS_PX) + 1;
    Channels outline = pixelAt(pixels, layout, 0, -pastTip);
    TEST_ASSERT_INT_WITHIN(MAX_CHANNEL_ERROR, GRID_LINE.r, outline.r);
    TEST_ASSERT_INT_WITHIN(MAX_CHANNEL_ERROR, GRID_LINE.g, outline.g);
    TEST_ASSERT_INT_WITHIN(MAX_CHANNEL_ERROR, GRID_LINE.b, outline.b);
}

void test_marker_shadow_falls_to_the_lower_left() {
    Layout layout = layoutFor(RECT_WIDTH, RECT_HEIGHT, false);
    std::vector<uint16_t> pixels = shadeWhole(layout, 0);
    // Everything else near the centre is mirror-symmetric, so only the shadow can make
    // the left side darker than the right.
    int reach = int(MARKER_CIRCUMRADIUS_PX + MARKER_OUTLINE_PX + MARKER_SHADOW_OFFSET_PX) + 2;
    int left = 0, right = 0;
    for (int dy = 0; dy <= reach; dy++) {
        for (int dx = 1; dx <= reach; dx++) {
            left += pixelAt(pixels, layout, -dx, dy).g;
            right += pixelAt(pixels, layout, dx, dy).g;
        }
    }
    TEST_ASSERT_LESS_THAN_INT(right, left);
}

void test_highlight_brightens_the_upper_right_of_the_glass() {
    Layout layout = layoutFor(ROUND_SIZE, ROUND_SIZE, true);
    std::vector<uint16_t> pixels = shadeWhole(layout, 0);
    int dx = int(lroundf(HIGHLIGHT_CENTRE_X * layout.rangeRadius));
    int dy = int(lroundf(HIGHLIGHT_CENTRE_Y * layout.rangeRadius));
    // Its mirror image in the lower left sits at the same distance from the centre, so it
    // has the same gradient and differs only by the highlight.
    Channels lit = unpack(pixels[size_t(layout.cy + dy) * layout.width + layout.cx + dx]);
    Channels unlit = unpack(pixels[size_t(layout.cy - dy) * layout.width + layout.cx - dx]);
    TEST_ASSERT_GREATER_THAN_INT(unlit.r + 20, lit.r);
}

void test_strip_cache_reproduces_the_single_buffer_shading() {
    const bool panels[] = {true, false};
    for (bool roundPanel : panels) {
        Layout layout = roundPanel ? layoutFor(ROUND_SIZE, ROUND_SIZE, true) : layoutFor(RECT_WIDTH, RECT_HEIGHT, false);
        std::vector<uint16_t> whole = shadeWhole(layout, 2);
        StripCache cache;
        TEST_ASSERT_TRUE(cache.allocate(layout.width, layout.height));
        cache.shade(layout, 2, table);
        std::vector<uint16_t> copied(whole.size(), 0xA5A5);
        cache.copyInto(copied.data());
        TEST_ASSERT_EQUAL_HEX16_ARRAY(whole.data(), copied.data(), whole.size());
    }
}

// --- blendBlip ---

static const int BLIP_BUFFER_WIDTH = 40;
static const int BLIP_BUFFER_HEIGHT = 30;
static const int GUARD = 64;
static const uint16_t GUARD_VALUE = 0x5A5A;
// The screen's edge green, big-endian.
static const uint16_t BACKGROUND = uint16_t(((0x0361 & 0xFF) << 8) | (0x0361 >> 8));

struct BlipBuffer {
    std::vector<uint16_t> storage;
    BlipBuffer() : storage(GUARD + BLIP_BUFFER_WIDTH * BLIP_BUFFER_HEIGHT + GUARD, GUARD_VALUE) {
        for (int i = 0; i < BLIP_BUFFER_WIDTH * BLIP_BUFFER_HEIGHT; i++) pixels()[i] = BACKGROUND;
    }
    uint16_t* pixels() { return &storage[GUARD]; }
    uint16_t at(int x, int y) { return pixels()[y * BLIP_BUFFER_WIDTH + x]; }
};

static int brightness(uint16_t pixel) {
    Channels c = unpack(pixel);
    return c.r + c.g + c.b;
}

void test_blip_has_a_white_centre() {
    BlipBuffer buffer;
    blendBlip(buffer.pixels(), BLIP_BUFFER_WIDTH, BLIP_BUFFER_HEIGHT, 20, 15);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, buffer.at(20, 15));
}

void test_blip_glow_fades_out_into_the_background() {
    BlipBuffer buffer;
    blendBlip(buffer.pixels(), BLIP_BUFFER_WIDTH, BLIP_BUFFER_HEIGHT, 20, 15);
    int previous = brightness(buffer.at(20, 15));
    for (int distance = 1; distance <= BLIP_RADIUS_PX; distance++) {
        int current = brightness(buffer.at(20 + distance, 15));
        TEST_ASSERT_LESS_OR_EQUAL_INT(previous, current);
        previous = current;
    }
    // The halo is still visible part of the way out, and gone past the radius.
    TEST_ASSERT_GREATER_THAN_INT(brightness(BACKGROUND), brightness(buffer.at(20 + BLIP_RADIUS_PX - 2, 15)));
    TEST_ASSERT_EQUAL_HEX16(BACKGROUND, buffer.at(20 + BLIP_RADIUS_PX + 1, 15));
    TEST_ASSERT_EQUAL_HEX16(BACKGROUND, buffer.at(20, 15 - BLIP_RADIUS_PX - 1));
}

// Every pixel the blip changed lies within its radius of the centre: a blip that ran off
// one side of a row would show up wrapped onto the neighbouring row.
static void assertBlipStaysInside(int x, int y) {
    BlipBuffer buffer;
    blendBlip(buffer.pixels(), BLIP_BUFFER_WIDTH, BLIP_BUFFER_HEIGHT, x, y);
    for (int i = 0; i < GUARD; i++) {
        TEST_ASSERT_EQUAL_HEX16(GUARD_VALUE, buffer.storage[i]);
        TEST_ASSERT_EQUAL_HEX16(GUARD_VALUE, buffer.storage[buffer.storage.size() - 1 - i]);
    }
    for (int py = 0; py < BLIP_BUFFER_HEIGHT; py++) {
        for (int px = 0; px < BLIP_BUFFER_WIDTH; px++) {
            if (buffer.at(px, py) == BACKGROUND) continue;
            TEST_ASSERT_LESS_OR_EQUAL_INT(BLIP_RADIUS_PX * BLIP_RADIUS_PX, (px - x) * (px - x) + (py - y) * (py - y));
        }
    }
}

void test_blip_at_each_edge_writes_only_inside_the_buffer() {
    assertBlipStaysInside(0, 15);
    assertBlipStaysInside(BLIP_BUFFER_WIDTH - 1, 15);
    assertBlipStaysInside(20, 0);
    assertBlipStaysInside(20, BLIP_BUFFER_HEIGHT - 1);
    assertBlipStaysInside(-3, 15);
    assertBlipStaysInside(BLIP_BUFFER_WIDTH + 2, 15);
    assertBlipStaysInside(-2, -2);
}

void test_blip_fully_off_the_buffer_writes_nothing() {
    const int offsets[][2] = {{-BLIP_RADIUS_PX - 1, 15}, {BLIP_BUFFER_WIDTH + BLIP_RADIUS_PX, 15},
                              {20, -BLIP_RADIUS_PX - 1}, {20, BLIP_BUFFER_HEIGHT + BLIP_RADIUS_PX}};
    for (const auto& offset : offsets) {
        BlipBuffer buffer;
        blendBlip(buffer.pixels(), BLIP_BUFFER_WIDTH, BLIP_BUFFER_HEIGHT, offset[0], offset[1]);
        for (int i = 0; i < BLIP_BUFFER_WIDTH * BLIP_BUFFER_HEIGHT; i++) {
            TEST_ASSERT_EQUAL_HEX16(BACKGROUND, buffer.pixels()[i]);
        }
    }
}

int main() {
    buildRadialTable(table);
    UNITY_BEGIN();
    RUN_TEST(test_layouts_fit_the_panels_and_the_table);
    RUN_TEST(test_round_shading_matches_the_float_reference_at_every_zoom);
    RUN_TEST(test_rect_shading_matches_the_float_reference_at_every_zoom);
    RUN_TEST(test_round_panel_is_black_beyond_the_glass);
    RUN_TEST(test_rect_panel_frames_the_glass_in_a_grey_bezel);
    RUN_TEST(test_bevel_is_lit_from_the_upper_right);
    RUN_TEST(test_centre_cross_is_brighter_than_the_grid_and_the_screen);
    RUN_TEST(test_centre_cross_ends_short_of_the_rim);
    RUN_TEST(test_marker_is_red_on_the_centre);
    RUN_TEST(test_marker_is_outlined_in_the_grid_colour);
    RUN_TEST(test_marker_shadow_falls_to_the_lower_left);
    RUN_TEST(test_highlight_brightens_the_upper_right_of_the_glass);
    RUN_TEST(test_strip_cache_reproduces_the_single_buffer_shading);
    RUN_TEST(test_blip_has_a_white_centre);
    RUN_TEST(test_blip_glow_fades_out_into_the_background);
    RUN_TEST(test_blip_at_each_edge_writes_only_inside_the_buffer);
    RUN_TEST(test_blip_fully_off_the_buffer_writes_nothing);
    return UNITY_END();
}
