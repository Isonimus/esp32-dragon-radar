---
id: '0012'
title: "Radar background: shaded per pixel in integer maths once per zoom change, cached in RAM strips"
type: architecture
status: accepted
date: 2026-10-16
supersedes: []
superseded_by: []
---

## Context

The radar redesign brings a radial gradient on the screen, a lit bezel, a centre cross
and a glass highlight. The title logo looks much smoother than today's radar, and the
reason is anti-aliasing. The logo is drawn at 8× and downsampled (ADR-0006), so its edge
pixels hold partial coverage. The radar is drawn with `fillCircle` and `drawLine`, which
set each pixel fully on or off. Its barrel-distorted grid is also built from 24 segments
per line with endpoints truncated to whole pixels. Curves come out as staircases and the
grid lines wobble.

Everything static on the radar changes only with the zoom level (five grid densities).
`drawGrid` ignores its rotation argument. The probe (`src/probes/radar_render_probe.cpp`,
built with `probes/radar-render.ini`) measured four ways to draw the background and grid
on all three boards. Each figure is the median of 30 runs, or of 3 for those marked †. A
range runs from zoom level 0 (4 grid cells) to level 4 (12 cells).

| Measured | Custom (ESP32-C3, 240×240) | Cardputer ADV (ESP32-S3, 240×135) | StickC Plus2 (ESP32, 240×135) |
|---|---|---|---|
| Today's whole radar frame | 8.2–16.5 ms | 1.3–1.9 ms | 1.8–2.5 ms |
| Grid in LovyanGFX's anti-aliased lines, per frame † | 100–316 ms | 5.9–15.6 ms | 6.8–18.4 ms |
| Background and grid shaded per pixel, float † | 3,269–3,279 ms | 65.7–65.9 ms | 65.6–65.8 ms |
| The same in integer maths, from a radial table | 136 ms | 16.1 ms | 18.2 ms |
| Building that table (once per boot) † | 48 ms | 3.5 ms | 3.5 ms |
| Copying a cached background into the canvas, from RAM strips | 0.48 ms | 0.17 ms | 0.17 ms |
| Copying a screen-sized image from flash | 4.4 ms | 2.5 ms | 8.2 ms |
| Pushing the canvas to the panel | 26.1 ms | 13.0 ms | 13.0 ms |
| Free heap with the canvas | 167 KB | 285 KB | 196 KB |
| Free heap with the canvas and a cached background | 48.6 KB (largest block 13.8 KB) | 220 KB | 131 KB |

The game runs at 20 Hz, a 50 ms frame. On the custom board, today's frame plus the push
already takes up to 43 ms of it.

- **Anti-aliased lines every frame** are out. On the C3 they cost up to 6× the whole frame
  budget. LovyanGFX's `drawSmoothLine` and `drawWideLine` also take integer endpoints, so
  the curved grid still kinks at every vertex. The probe captures show the kinks.
- **Float shading** gives clean curves, edge and gradient, but takes 3.3 s on the C3,
  which has no FPU.
- **Integer shading** keeps the result within 8/255 of the float version on every channel,
  about one RGB565 step. That is the largest difference found on each board. The inverse
  barrel map, the gradient colour and the line-width scale depend only on the distance from
  the centre, so they go in a 1024-entry table built once. Each pixel then needs integer
  arithmetic only, plus a square root for the two-pixel rim, where the circle edge is
  partly covered.
- **A single second screen buffer does not fit on the C3.** With the canvas allocated, its
  largest free block is 114,676 bytes, against 115,200 needed. Two halves failed too
  (largest block 55,284 bytes after the first), and so did four quarters (28,660 after
  three). Eight strips of 30 rows allocate.
- **Pre-rendering on the host into flash**, the logo's approach, was the alternative that
  costs no RAM. It is slower per frame than RAM on every board: 9× on the C3, 15× on the
  S3 and 48× on the ESP32. Five zoom levels of 240×240 RGB565 come to 576 KB, which would
  fill about 85% of the custom board's 1.25 MB app partition. The grid geometry would also
  live twice, in Python and in C++, and the blips must land on the same grid.

## Decision

- The radar's static layer is **shaded per pixel in integer maths** from a radial lookup
  table built once at boot, and **cached in RAM as eight row strips**. The static layer is
  the screen, its gradient and the barrel-distorted grid, plus any later static
  decoration. It is reshaded only when the zoom level changes. It writes big-endian RGB565,
  the byte order of a 16-bit LovyanGFX sprite (slice 0011).
- Each frame **copies the cache into the canvas** and then draws only what moves or
  changes on top: the blips, the markers and the text.
- Float shading, anti-aliased line primitives for static geometry, and flash-resident
  pre-rendered backgrounds are not used for the radar.

## Consequences

- **Frame cost falls on every board.** The background costs a 0.17–0.48 ms copy instead
  of today's 0.7–17.6 ms of drawing. A zoom change costs one long frame instead: 136 ms on
  the C3, 16 ms on the S3, 18 ms on the ESP32.
- **The cache costs a second screen of RAM:** 115 KB on the custom board and 65 KB on the
  other two. On the C3 that leaves **48.6 KB of free heap** for the rest of the game, down
  from 167 KB. The slice that adopts this must measure the game's lowest free heap during
  play on the C3. If the margin is not safe, that is a new decision, not a quiet fallback.
- **The table and the shader are shared code**, so a change to them is built in every env
  (invariant #2). The barrel map, its inverse and the gradient must stay in one place,
  so the cached grid and anything drawn over it agree.
- **The probe stays in the repo** to re-measure these numbers. Every game build compiles
  it to nothing; it runs only in the probe envs.
- **No dithering.** RGB565 bands the gradient in rings visible in an upscaled capture, but
  the operator saw no banding on the Cardputer's panel.
- **Unused headroom on the StickC.** The StickC Plus2 has 2 MB of PSRAM, but its env uses
  the generic `esp32dev` board, which does not enable it; the probe reports `psram=0`.
  Nothing needs it under this decision.
