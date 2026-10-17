---
id: '0013'
title: "Radar redesign: an anti-aliased lit screen in a bezel, with blinking glowing blips"
type: slice
status: accepted
date: 2026-10-16
supersedes: []
superseded_by: []
---

## Goal

Make the radar screen look like the anime device. Today it is a flat green disc with a
1-px aliased grid and a rotating sweep line. The reference images show five differences:

- the blips blink and glow, and nothing sweeps;
- the screen is a brighter green, lit towards the centre;
- the grid's two centre lines form a brighter cross;
- the glass has a highlight;
- the case is a light grey bezel with a bevel.

The grid's barrel bulge stays, as do the ball numbers at the closer zooms (≤ 1 km).

Today's radar also looks rough next to the title logo, and the reason is anti-aliasing
(ADR-0012). This slice adopts ADR-0012: the static layer is shaded once per zoom change
into a RAM cache, and each frame draws only what moves.

ADR-0012 makes the C3's remaining heap this slice's gate: 48.6 KB is left once the cache is
allocated. The slice also answers the ledger item to retake the README's radar screenshot
with a live fix, since the README must show the new screen anyway. It fixes the README's
Features table, which promises "concentric range rings" that the radar has never drawn.

## Definition of Done

- **Given** the radar screen at any zoom level, on any board
- **When** the background is shaded
- **Then** every pixel is within 8/255 per channel of a float reference shading of the same layer, the grid lines curve with the barrel bulge, and the screen's rim is anti-aliased

- **Given** a background shaded for one panel size and zoom level
- **When** it is cached as row strips and copied into the canvas
- **Then** the canvas holds exactly the pixels a single-buffer shading would have produced

- **Given** the radar screen on the Cardputer or the StickC (rectangular panels)
- **When** it is drawn
- **Then** the screen sits in a light grey, slightly green bezel with a dark outer outline, a dark inner ring and a bevel lit from the upper right, and the glass shows a soft highlight in its upper right

- **Given** the radar screen on the custom board's round panel
- **When** it is drawn
- **Then** no bezel is drawn, because the panel's own edge is the screen's rim, and the glass highlight still is

- **Given** the radar screen
- **When** it is drawn
- **Then** the grid's two centre lines are brighter than the other grid lines, the red player triangle sits on their crossing, and there is no sweep line

- **Given** a ball within the zoom range
- **When** the blink cycle is in its lit phase
- **Then** the ball shows as a glowing blip (a soft halo, a pale yellow core and a white centre), with its number beside it at zooms of 1 km or less; in the dark phase it is not drawn

- **Given** a blip at the edge of the screen or partly outside the canvas
- **When** it is drawn
- **Then** only pixels inside the canvas are written

- **Given** the radar screen with sound on
- **When** each blink cycle enters its lit phase
- **Then** the radar beep plays once, and not in the dark phase; no beep plays with sound off or on another screen

- **Given** the zoom level changes
- **When** the next frame is drawn
- **Then** the background is reshaded once for the new grid density, and the frames after it only copy the cache

- **Given** each of the three boards
- **When** the probe times the radar frame before and after this slice
- **Then** the new frame, blips included, takes no longer than today's frame at the same zoom level

- **Given** the custom board (ESP32-C3) in a session that cycles through all five zoom levels and saves the game
- **When** the game logs its lowest free heap, before and after this slice
- **Then** no allocation fails, and the lowest free heap after this slice is no lower than before by more than the cache and the radial table

- **Given** the cache cannot be allocated at boot
- **When** setup runs
- **Then** boot stops with a message on the screen, as it does for the canvas

## Design

**Static layer: `src/render/radar_background.{h,cpp}`**, free of Arduino and LovyanGFX so
the native env tests it. It writes big-endian RGB565 into plain `uint16_t` rows, the byte
order of a 16-bit sprite (slice 0011). It holds:

- **The radial table.** The inverse barrel map, the gradient colour and the line-width
  scale depend only on the normalised distance from the centre, not on the panel size.
  That makes one table serve every board. It is built once at boot. Its size (512 or 1024
  entries) is chosen by the accuracy test against the float reference, and its RAM is
  counted in the heap gate.
- **`shadeRows(rows, firstRow, endRow, layout, zoom, table)`** shades the screen gradient,
  the barrel grid with its brighter centre cross, the rim's anti-aliasing, the glass
  highlight, and, if `layout.bezel`, the bezel. All of it is integer maths, apart from a
  square root on the rim's partly covered pixels and the bezel's ring (ADR-0012).
  - **The highlight is a soft white ellipse in the upper right**, blended at low strength.
    The bezel's bevel is lit from the same upper right, so the two read as one light
    source. The reference images light the case from above.
  - **The bezel** sits outside today's screen radius (`r = height/2 - 5`), where the grey
    `drawCircle` rings sit today. The rectangular panels have only 5 px above and below
    the circle, so there is no crown or button. The round panel draws none, decided by
    `DisplayHAL::isRound()`, not a board `#ifdef` (invariant #1).
- **The strip cache** (eight row strips, ADR-0012) lives in `RadarRenderer`, which shades
  it when the zoom level differs from the cached one and copies it into the canvas each
  frame. It is allocated at boot next to the canvas; failure halts on screen through
  `haltOnScreen`, like the canvas.

**Dynamic layer**, still in `RadarRenderer`, drawn over the copied cache each frame:

- **Blips.** `blendBlip(rows, width, height, x, y)` in `radar_background.cpp` alpha-blends a
  radial glow of about 6 px radius into the buffer: a halo fading to the background, a
  pale yellow core and a white centre. It is clipped to the buffer. This is a few hundred
  pixels per ball, and only for balls in range. The numbers stay LovyanGFX text, as today.
- **The player triangle, the north and nearest-ball markers, the zoom legend and the HUD**
  keep today's drawing. They move or change, so they are not static (invariant #6). They
  are small, so their aliasing is not what makes the radar look rough.
  *Correction (amendment of 2026-10-16 below): the player triangle never moves or turns, so it is static and now shaded into the cache.*

**Blink and beep: `src/game/blip_blink.{h,cpp}`**, pure logic.

- The radar has one shared blink cycle. It is lit for `BLIP_LIT_MS` and dark for the rest
  of `BLIP_PERIOD_MS`.
- `update(nowMs)` reports whether the blips are lit and whether a lit phase began since the
  last call. `main.cpp` plays `playRadarBeep()` on that onset, which replaces the
  sweep's crossing of north.
- The period starts at today's beep cadence, one sweep turn: 36 frames at 10° a frame, at
  least 1.8 s. That keeps the rhythm players know. It is then tuned by eye on the device.
- The sweep angle, `drawSweep` and the `sweepAngle` parameter of `render` are deleted.

**Instrumentation.** In `STATE_RADAR`, the game logs `[radar] min_free_heap=<n>`
(`esp_get_minimum_free_heap_size`) once per zoom change and every 10 s. It is added and
measured on today's radar first, so the heap gate has its "before" number from the same
session script. The probe gains a `radar-frame` metric that times the new `render()` with
seven balls in range, for the frame-time gate. Its `aliased-frame` numbers in ADR-0012 are
the "before".

**Existing code considered.**

- **The probe's integer shader and strip cache** (`src/probes/radar_render_probe.cpp`)
  are the starting point. They move into `radar_background` as tested production code.
  The probe then calls the production shader, so the two cannot drift.
- **`barrelDistort`** stays the one forward map in the renderer. The table's inverse is
  derived from the same `BARREL_K`, defined once in `radar_background.h`.
- **`render-logo.py`'s pre-rendering** is not reused, for ADR-0012's measured reasons.
- **LovyanGFX's `pushAlphaImage`** could blend a pre-rendered ARGB blip sprite. A blended
  function needs no asset, works at any size, and is the code the native tests already
  exercise, so `blendBlip` is used instead.

**Out of scope.** The README's radar screenshot is retaken with a live fix if one can be
had. Otherwise the ledger item stays, reworded for the new screen.

## Verification

- `test/test_radar_background` (native, run in CI by `pio test -e native`). It covers:
  - **Accuracy:** the integer shading against a float reference, for both panel
    geometries (240 × 240 with r 115, 240 × 135 with r 62) and all five zoom levels.
  - **Outside the screen:** pixels beyond the screen are black on the round layout and
    bezel grey on the rectangular one.
  - **The centre cross:** it is brighter than another grid line at the same distance from
    the centre.
  - **The strip cache:** it reproduces the single-buffer shading.
  - **`blendBlip`:** a white centre, a halo that fades out to the background, and no
    writes outside the buffer, for blips at each edge and fully off it.
- `test/test_blip_blink` (native): lit and dark phases, one onset per period, no onset
  inside a phase, and a first call that lands mid-cycle.
- **Probe** (`probes/radar-render.ini`), on all three boards. The `radar-frame` times sit
  next to ADR-0012's `aliased-frame` times per zoom level, and are recorded in `## As built`.
- **On the custom board:**
  - the `[radar] min_free_heap` log over the same session (all five zooms, one save),
    before and after, recorded in `## As built`;
  - a build with the cache allocation forced to fail, which shows the halt message.
- **Captures** with `scripts/capture_screen.py` of the radar on all three boards, before
  and after. They are the artifact for the look-and-feel scenarios, which only eyes can
  judge. The README's `docs/images/radar.png` is replaced by an after capture.
- All three firmware envs build and the native suite passes (invariant #2).

## As built

Shipped on 2026-10-16. Where the build differs from the Design above, this section says
so and why.

**Differences from the Design.**

- **Two radii, not one.** The layout has a *range radius*, `height/2 - 5` as before, which
  scales the grid, the blips and the markers, and a *screen radius* where the glass ends.
  On the rectangular panels they are equal and the 5 px bezel fills the margin. On the
  round panel the glass runs to the panel's edge (`height/2`). Keeping today's radius there
  would have left a 5 px black ring where the grey rings were, so the "panel edge is the
  rim" scenario would have been false. The markers keep the range radius, clear of the
  round panel's lip.
- **The centre is a pixel's centre.** The probe centred the circle on a pixel boundary;
  a one-pixel cross there renders as two half-bright columns. The shader centres on pixel
  `(width/2, height/2)`, where the triangle and markers are drawn.
- **The centre cross is short.** The first build lit the grid's whole centre lines. On
  the operator's reference image the bright cross reaches about a quarter of the radius
  and the grid's centre lines run on past it, dark like the others. Each arm is now
  `CENTRE_CROSS_ARM = 0.27` range radii, measured on the screen with square ends; a test
  checks the centre column past the arm is an ordinary grid line.
- **The bezel is shaded in float.** It is drawn only on the rectangular panels, whose
  chips (S3, ESP32) have an FPU, and its pixels already take the square root the Design
  allowed. The C3 never runs it.
- **The radial table is normalised.** It spans `RADIAL_TABLE_REACH = 1.1` range radii,
  past the round panel's glass edge, in 1024 entries, so one table serves any panel. 1024
  holds the 8/255 bound on both layouts; 512 was not tried.
- **The probe.** `aliased-frame` went with the old renderer. `copy-strips` and
  `fixed-vs-float` were dropped: the renderer now owns the only cache, a second does not
  fit on the C3, and the native test checks the accuracy. `fixed-grid` and `radar-frame`
  run the production code.
- **The heap log** samples `ESP.getMinFreeHeap()`, the Arduino core's wrapper of
  `esp_get_minimum_free_heap_size`.

**Frame time** (median, seven balls in range, zoom levels 0 to 4). ADR-0012's
`aliased-frame` was timed with no fix and no balls, so it is not the same frame. The
"before" here is the old renderer with the same seven balls, built from the commit before
this slice with only the probe's timing loop changed.

| Board | Before | After | Reshading on a zoom change |
|---|---|---|---|
| Custom (ESP32-C3) | 12.2–20.2 ms | 4.5–4.9 ms | 115 ms |
| Cardputer ADV (ESP32-S3) | 2.84–3.37 ms | 1.73–1.97 ms | 28 ms |
| StickC Plus2 (ESP32) | 3.78–4.36 ms | 2.24–2.68 ms | 32 ms |

Reshading costs more than ADR-0012's 16–18 ms on the rectangular boards because the cross,
the highlight and the bezel are now shaded too.

**Heap on the custom board.** The same session before and after: a new game (which
saves), all five zoom levels in and out, then about 20 s on the radar. No allocation
failed in either.

| | Lowest free heap |
|---|---|
| Before | 160,056 bytes |
| After | 31,904 bytes |
| Drop | 128,152 bytes |
| The cache and the table's data | 123,392 bytes |

By the letter of the scenario, the gate fails by 4,760 bytes. The excess is the
allocator's cost of eight 14 KB blocks in a fragmented heap, which ADR-0012 also measured
(118.4 KB of free heap for 115.2 KB of strips). The scenario should have bounded the drop
by the measured cost of the allocation, not by its data size. The operator accepted the
31.9 KB margin; `LEDGER.md` carries an item to re-measure it when a change allocates at
runtime, with the saving to make if it gets tight. With the radar's background allocated,
the probe reports 211 KB free on the Cardputer and 122 KB on the StickC.

**Other evidence.**

- Native suite: 105 tests pass. The new ones failed against a stub first. Of 12 hand
  mutants of the shader, the blip, the strip cache and the blink, 11 failed a test; the
  survivor rounds the strip boundaries up instead of down, an equivalent partition.
- A build that forced `begin()` to fail halted on the StickC with
  `Setup: No memory for radar background - halting`, and the operator saw the message on
  its screen.
- Captures of the new radar on all three boards were reviewed by the operator; the custom
  board's was taken in play with a live fix. The README's `docs/images/radar.png` is now a
  StickC capture in play with a live fix, which closes the ledger's item to retake it. It
  shows a battery reading of 6.6 V, a separate bug now in the ledger.
- The custom board's heap sessions needed a GPS fix, which exposed that its module is a
  GPS + BeiDou clone (`SW=URANUS5`), not a u-blox NEO-6M, with weak signal; see `LEDGER.md`.

## Amendment, 2026-10-16: the player marker is shaded into the cache

The Design kept the player triangle in the per-frame layer on the claim that it moves or
changes. It does neither: it sits on the centre and always points up, so it is static, and
invariant #6 puts it in the cached background. Moving it there also let it gain what the
anime device shows and `fillTriangle` could not draw: anti-aliased edges and a 1 px
outline in the grid's dark green, which separates the red from the bright centre cross.
It also casts a faint shadow 1 px down and to the left, away from the light the highlight
and bevel share.

The marker is shaded in float (`shadeMarker`), over a box of about 20 px a side, once per
zoom; the C3's soft float costs nothing that matters there. The float reference gained
the marker, so the shader's accuracy test covers it, and three tests (red on the centre,
outlined past the tip, darker to the lower left) failed before the change. A blip drawn
at the very centre now covers the marker, which only happens on top of a ball, where it
is collected.

