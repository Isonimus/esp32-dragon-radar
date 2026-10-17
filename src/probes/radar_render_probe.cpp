// Radar render probe: what a smooth radar background costs on each board, measured
// before the radar redesign picks how to draw it. It is compiled only into the probe
// build (probes/radar-render.ini); every game build compiles this file to nothing.
//
// It times, per zoom level:
//   radar-frame    RadarRenderer::render, the whole frame, with seven balls in range
//   aliased-grid   background and grid alone, plain drawLine (the look before slice 0013)
//   smooth-grid    the same geometry with LovyanGFX's anti-aliased lines
//   shaded-grid    background and grid shaded per pixel, in float
//   fixed-grid     the production shading (radar_background), from a table built once per boot
// and, once: building that table, copying a screen-sized image from flash, and pushing the
// canvas to the panel. It reports free heap with the canvas and with the radar's cached
// background. The first three grids are ADR-0012's rejected alternatives, kept to
// re-measure them; their palette is the one that ADR measured.
//
// Afterwards the panel shows one variant; send '1' to '5' to switch and 'S' to capture it
// with scripts/capture_screen.py.
#ifdef RADAR_RENDER_PROBE

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <vector>

#include "../hal/hal.h"
#include "../render/radar_renderer.h"
#include "../render/radar_background.h"
#include "../debug/screen_capture.h"

namespace {

constexpr int RUNS = 30;
// The float shading takes seconds on the C3 and varies by under 0.01% run to run.
constexpr int SLOW_RUNS = 3;
constexpr int ZOOM_COUNT = RadarBackground::ZOOM_LEVEL_COUNT;
using RadarBackground::GRID_CELLS;
using RadarBackground::BARREL_K;
constexpr int SEGMENTS = 24;
constexpr int SHOWN_ZOOM = 2;

// The palette ADR-0012 measured with: a brighter centre fading to the old flat RADAR_GREEN.
constexpr float CENTRE_RGB[3] = {40, 170, 50};
constexpr float EDGE_RGB[3] = {5, 108, 13};
constexpr float GRID_RGB[3] = {0, 64, 0};
constexpr uint16_t RADAR_GREEN = 0x0361;
constexpr uint16_t RADAR_GRID = 0x0200;
// Option C's per-frame cost: an image the size of the largest panel, read from flash.
constexpr int FLASH_IMAGE_PIXELS = 240 * 240;
const uint16_t FLASH_IMAGE[FLASH_IMAGE_PIXELS] = {1};

LGFX_Sprite* canvas = nullptr;
RadarRenderer* radar = nullptr;
int shownVariant = 1;

struct Geometry {
    int cx, cy, r;
};

Geometry geometryOf(LGFX_Sprite& sprite) {
    return {sprite.width() / 2, sprite.height() / 2, sprite.height() / 2 - 5};
}

void barrelDistort(float& px, float& py, const Geometry& g) {
    float dx = (px - g.cx) / g.r;
    float dy = (py - g.cy) / g.r;
    float scale = 1.0f + BARREL_K * (dx * dx + dy * dy);
    px = g.cx + dx * scale * g.r;
    py = g.cy + dy * scale * g.r;
}

// RadarRenderer::drawGrid's geometry, with the line primitive passed in.
template <typename DrawSegment>
void drawGrid(const Geometry& g, int cells, DrawSegment drawSegment) {
    float extend = 1.0f / (1.0f + BARREL_K);
    float step = 2.0f * extend / cells;
    for (int vertical = 0; vertical < 2; vertical++) {
        for (int i = 0; i <= cells; i++) {
            float fixed = -extend + i * step;
            float prevX = 0, prevY = 0;
            bool prevInside = false;
            for (int j = 0; j <= SEGMENTS; j++) {
                float along = -extend + j * (2.0f * extend / SEGMENTS);
                float px = g.cx + (vertical ? fixed : along) * g.r;
                float py = g.cy + (vertical ? along : fixed) * g.r;
                barrelDistort(px, py, g);
                float ddx = px - g.cx, ddy = py - g.cy;
                bool inside = ddx * ddx + ddy * ddy <= float(g.r * g.r);
                if (inside && prevInside) drawSegment(prevX, prevY, px, py);
                prevX = px;
                prevY = py;
                prevInside = inside;
            }
        }
    }
}

void drawAliasedGrid(LGFX_Sprite& sprite, int zoom) {
    Geometry g = geometryOf(sprite);
    sprite.fillScreen(TFT_BLACK);
    sprite.fillCircle(g.cx, g.cy, g.r, RADAR_GREEN);
    drawGrid(g, GRID_CELLS[zoom], [&](float x0, float y0, float x1, float y1) {
        sprite.drawLine(int(x0), int(y0), int(x1), int(y1), RADAR_GRID);
    });
}

void drawSmoothGrid(LGFX_Sprite& sprite, int zoom) {
    Geometry g = geometryOf(sprite);
    sprite.fillScreen(TFT_BLACK);
    sprite.fillSmoothCircle(g.cx, g.cy, g.r, RADAR_GREEN);
    drawGrid(g, GRID_CELLS[zoom], [&](float x0, float y0, float x1, float y1) {
        sprite.drawSmoothLine(lroundf(x0), lroundf(y0), lroundf(x1), lroundf(y1), RADAR_GRID);
    });
}

float clamp01(float value) {
    return value < 0 ? 0 : (value > 1 ? 1 : value);
}

// Undoes barrelDistort's radial map s = t(1 + K t^2) by Newton's method.
float undistortRadius(float screenRadius) {
    float t = screenRadius;
    for (int i = 0; i < 3; i++) {
        float f = t * (1.0f + BARREL_K * t * t) - screenRadius;
        t -= f / (1.0f + 3.0f * BARREL_K * t * t);
    }
    return t;
}

// Pixels from (u, v), in flat grid units, to the nearest grid line along one axis.
float lineDistancePx(float flat, float step, float extend, float pxPerUnit) {
    float cell = (flat + extend) / step;
    float fromLine = fabsf(cell - roundf(cell)) * step;
    return fromLine * pxPerUnit;
}

void drawShadedGrid(LGFX_Sprite& sprite, int zoom) {
    Geometry g = geometryOf(sprite);
    float extend = 1.0f / (1.0f + BARREL_K);
    float step = 2.0f * extend / GRID_CELLS[zoom];
    uint16_t* pixels = static_cast<uint16_t*>(sprite.getBuffer());
    int width = sprite.width();
    for (int y = 0; y < sprite.height(); y++) {
        for (int x = 0; x < width; x++) {
            float u = (x + 0.5f - g.cx) / g.r;
            float v = (y + 0.5f - g.cy) / g.r;
            float screenRadius = sqrtf(u * u + v * v);
            float inside = clamp01((1.0f - screenRadius) * g.r + 0.5f);
            if (inside <= 0) {
                pixels[y * width + x] = 0;
                continue;
            }
            float flatRadius = undistortRadius(screenRadius);
            float toFlat = screenRadius > 0 ? flatRadius / screenRadius : 1.0f;
            float pxPerUnit = g.r * (1.0f + BARREL_K * flatRadius * flatRadius);
            float distance = std::min(lineDistancePx(u * toFlat, step, extend, pxPerUnit),
                                      lineDistancePx(v * toFlat, step, extend, pxPerUnit));
            float line = clamp01(1.0f - distance);
            float rgb[3];
            for (int c = 0; c < 3; c++) {
                float base = CENTRE_RGB[c] + (EDGE_RGB[c] - CENTRE_RGB[c]) * screenRadius;
                rgb[c] = (base + (GRID_RGB[c] - base) * line) * inside;
            }
            uint16_t rgb565 = (uint16_t(rgb[0]) >> 3 << 11) | (uint16_t(rgb[1]) >> 2 << 5) | (uint16_t(rgb[2]) >> 3);
            // A 16-bit LovyanGFX sprite keeps its pixels big-endian (slice 0011).
            pixels[y * width + x] = __builtin_bswap16(rgb565);
        }
    }
}

RadarBackground::RadialTable* radialTable = nullptr;

// Seven balls spread around the player, inside the range at every zoom level.
constexpr double PROBE_LAT = 40.4168;
constexpr double PROBE_LON = -3.7038;
constexpr double METRES_PER_DEGREE = 111320.0;
constexpr float BALL_RANGE_FRACTION = 0.6f;

GpsFix probeFix() {
    GpsFix fix = {};
    fix.lat = PROBE_LAT;
    fix.lon = PROBE_LON;
    fix.satellites = 8;
    fix.hasPosition = true;
    fix.current = true;
    return fix;
}

std::vector<Ball> ballsInRange(float rangeMetres) {
    std::vector<Ball> balls;
    for (uint8_t id = 1; id <= 7; id++) {
        double bearing = id * (2 * M_PI / 7);
        double metres = rangeMetres * BALL_RANGE_FRACTION;
        balls.push_back({id, PROBE_LAT + metres * cos(bearing) / METRES_PER_DEGREE,
                         PROBE_LON + metres * sin(bearing) / (METRES_PER_DEGREE * cos(PROBE_LAT * M_PI / 180)), false});
    }
    return balls;
}

void setZoom(int zoom) {
    while (radar->getZoom() > 100.0f) radar->zoomIn();
    for (int i = 0; i < zoom; i++) radar->zoomOut();
}

void drawRadarFrame(LGFX_Sprite& sprite, const std::vector<Ball>& balls) {
    radar->render(sprite, probeFix(), balls, true, 0.0f);
}

void shadeFixedGrid(LGFX_Sprite& sprite, int zoom) {
    RadarBackground::shadeRows(static_cast<uint16_t*>(sprite.getBuffer()), 0, sprite.height(), radar->layout(), zoom,
                               *radialTable);
}

template <typename Work>
void timeIt(const char* metric, int zoom, Work work, int runs = RUNS) {
    uint32_t samples[RUNS];
    for (int run = 0; run < runs; run++) {
        int64_t start = esp_timer_get_time();
        work();
        samples[run] = uint32_t(esp_timer_get_time() - start);
    }
    std::sort(samples, samples + runs);
    Serial.printf("PROBE %s %s zoom=%d median_us=%lu min_us=%lu max_us=%lu\n", ESP.getChipModel(), metric,
                  zoom, (unsigned long)samples[runs / 2], (unsigned long)samples[0],
                  (unsigned long)samples[runs - 1]);
}

void reportHeap(const char* when) {
    Serial.printf("PROBE %s heap %s free=%u largest_block=%u psram=%u\n", ESP.getChipModel(), when,
                  heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                  ESP.getPsramSize());
}

void show(int variant) {
    shownVariant = variant;
    LGFX_Sprite& c = *canvas;
    switch (variant) {
        case 1: setZoom(SHOWN_ZOOM); drawRadarFrame(c, ballsInRange(radar->getZoom())); break;
        case 2: drawAliasedGrid(c, SHOWN_ZOOM); break;
        case 3: drawSmoothGrid(c, SHOWN_ZOOM); break;
        case 4: drawShadedGrid(c, SHOWN_ZOOM); break;
        case 5: shadeFixedGrid(c, SHOWN_ZOOM); break;
    }
    c.pushSprite(0, 0);
    Serial.printf("PROBE showing variant %d\n", variant);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1000);
    HardwareHAL& hal = getHAL();
    hal.power.begin();
    if (!hal.display.init()) {
        Serial.println("PROBE display failed");
        while (true) delay(1000);
    }
    auto& tft = hal.display.getDevice();

    reportHeap("before-canvas");
    canvas = new LGFX_Sprite(&tft);
    canvas->setColorDepth(16);
    if (!canvas->createSprite(tft.width(), tft.height())) {
        Serial.println("PROBE no memory for the canvas");
        while (true) delay(1000);
    }
    reportHeap("with-canvas");
    radar = new RadarRenderer(hal.display.isRound());
    if (!radar->begin(*canvas)) {
        Serial.println("PROBE no memory for the radar background");
        while (true) delay(1000);
    }
    reportHeap("with-canvas-and-radar");
    radialTable = new RadarBackground::RadialTable;

    LGFX_Sprite& c = *canvas;
    timeIt("build-radial-table", 0, [&] { RadarBackground::buildRadialTable(*radialTable); }, SLOW_RUNS);
    for (int zoom = 0; zoom < ZOOM_COUNT; zoom++) {
        setZoom(zoom);
        std::vector<Ball> balls = ballsInRange(radar->getZoom());
        // The first frame at a zoom level reshades the cache; time the frames after it.
        drawRadarFrame(c, balls);
        timeIt("radar-frame", zoom, [&] { drawRadarFrame(c, balls); });
        timeIt("aliased-grid", zoom, [&] { drawAliasedGrid(c, zoom); });
        timeIt("smooth-grid", zoom, [&] { drawSmoothGrid(c, zoom); }, SLOW_RUNS);
        timeIt("shaded-grid", zoom, [&] { drawShadedGrid(c, zoom); }, SLOW_RUNS);
        timeIt("fixed-grid", zoom, [&] { shadeFixedGrid(c, zoom); });
    }
    timeIt("copy-from-flash", SHOWN_ZOOM, [&] {
        memcpy(c.getBuffer(), FLASH_IMAGE, std::min<size_t>(c.bufferLength(), sizeof(FLASH_IMAGE)));
    });
    timeIt("push-canvas", SHOWN_ZOOM, [&] { c.pushSprite(0, 0); });
    Serial.println("PROBE done");
    show(1);
}

void loop() {
    if (Serial.available()) {
        int request = Serial.peek();
        if (request >= '1' && request <= '5') {
            Serial.read();
            show(request - '0');
        } else {
            serviceScreenCapture(*canvas, Serial);
        }
    }
    delay(10);
}

#endif  // RADAR_RENDER_PROBE
