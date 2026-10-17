---
id: '0015'
title: "Round screen layout: every screen fits the custom board's circular panel"
type: slice
status: accepted
date: 2026-10-16
supersedes: []
superseded_by: []
---

## Goal

On the custom board's round panel the menu, loading and radar screens are cut off at the
edges (ledger, seen 2026-10-16). ADR-0014 measures how much of each element is lost and
decides the fix: every screen lays out inside a safe area, the inscribed square on the round
panel. This slice adopts it on every screen and closes the ledger item. Two overlaps on the
rectangular panels go with it, because they come from the same hand-placed coordinates: the
GPS wait screen's ball runs into "Searching for satellites...", and the found screen's
pulsing ball covers its headline.

## Definition of Done

- **Given** a 240 px round panel
- **When** the safe area is computed
- **Then** it is the 168 × 168 square from (36, 36), and every one of its pixels lies inside the circle

- **Given** a rectangular panel
- **When** the safe area is computed
- **Then** it is the whole panel

- **Given** each line of text a screen draws, at the size it draws it, on each panel shape
- **When** it is word-wrapped to the safe area's width
- **Then** every wrapped line fits that width, and no word is split

- **Given** the custom board's round panel
- **When** each screen is shown (menu, settings, GPS wait, scattering, radar with its HUD, ball found, win)
- **Then** no text and no logo is cut by the panel's edge, the radar's "Sats" and "Balls" sit at the safe area's top corners and its zoom legend at the bottom centre

- **Given** the Cardputer or the StickC
- **When** the menu and settings screens are shown
- **Then** they are pixel-identical to before this slice, and the other screens differ only where their content is re-fitted: no element overlaps another that it did not overlap by design (the scattering balls fly through the text)

## Design

`src/render/screen_layout.{h,cpp}`, free of Arduino and LovyanGFX so the native tests run
it:

- `Rect safeArea(int width, int height, bool roundPanel)`. On the round panel the square's
  half side is `floor(R / sqrt(2))` for the panel radius R, which keeps the outer corner of
  the outermost pixel inside the circle.
- `int wrapWords(const char* text, int maxChars, LineSpan* lines, int maxLines)`: breaks
  at spaces. The game's font is LovyanGFX's fixed 6 px glyph, so a width in pixels is a
  count of characters at a given text size.

`main.cpp` gets a helper that draws a wrapped, centred block anchored by its top or its
bottom, and every screen positions from the safe area:

- **Title:** inline logo if it fits the safe area's width, otherwise stacked. The version
  sits beside the logo when it fits the safe area, otherwise under it. It returns the title's
  bottom, from which the menu, settings and bodies start, replacing the hard-coded `55`.
- **GPS wait and scattering:** the status text is anchored to the safe area's bottom; the
  dragon ball fills the space between the title and that text, capped at today's 35 px.
  Scattering starts its balls where the GPS ball was.
- **Ball found and win:** headline wrapped and centred at the top of the safe area, the
  prompt at its bottom, the ball or the ring of balls scaled to the space between.
- **Radar:** the HUD is offset from the safe area's corners. The renderer computes the safe
  area itself and puts the zoom legend at the bottom-left of the radar's bounding square
  on rectangular panels, as today, and at the safe area's bottom centre on the round one.
  "LOST FIX" stays above the legend.

Considered and not reused: `RadarBackground::Layout` already knows the panel's shape, but it
describes the radar's geometry, not where content may go, and the menu screens do not have
a radar. The renderer's text calls stay LovyanGFX calls; only the positions change.

## Verification

- `test/test_screen_layout` (native): the safe area on both shapes, every pixel of the
  round one inside the circle and the square not growable by a pixel; `wrapWords` on
  single words, exact fits, several spaces and the game's own strings on both safe areas.
- **Captures** with `scripts/capture_screen.py` of every screen on the custom board, and
  before and after on a rectangular board. They are the evidence for the two on-device
  scenarios, which only eyes can judge.
- All three firmware envs and the probe envs build, the native suite passes, and cppcheck is
  clean (invariant #2).

## As built

Shipped as designed, on 2026-10-17.

- **Safe area and wrapping:** `test/test_screen_layout` has 11 tests, and the native suite
  passes with 119. The round safe area is 168 × 168 from (36, 36), and growing it by one
  pixel puts a pixel outside the circle. Every string in `ScreenText` fits both safe areas:
  the lines drawn whole fit on one line, and the others wrap with no word lost or split. Two
  of them are exactly the round safe area's 168 px: "Distance: < Medium (1.5km) >" and
  "Continue (7/7)".
- **On the custom board's round panel:** captures, masked to the circle, show the menu,
  settings and GPS wait screens whole.
  - Every screen takes the stacked logo, with the version under it.
  - The settings rows start at the safe area's edge, placed for the widest label.
  - The GPS wait ball shrinks to 28 px.

  The scattering, radar, ball found and win screens need a GPS fix, and there was none
  indoors. Their scenario is only partly met: a ledger audit carries their captures.
- **On the Cardputer:** `main` (492e1c1) and this branch were captured on the same screens.
  - **Menu:** 0 pixels differ above the animated footer.
  - **Settings:** 274 pixels differ, all from the cursor's highlight colour, since the
    cursor persists between visits. Compared as text against background, 0 differ.
  - **GPS wait:** the ball went from 35 to 28 px and now clears "Searching for
    satellites..." by 2 px. Before, it ran 5 px into that text.

  The found and win screens' re-fit (a centred headline and a ball or ring sized to the
  room) was not captured on either panel. It shares the ledger audit.
- **Adversarial pass:** one Sonnet reviewer, given the diff, `CLAUDE.md` and the ADR index,
  found no correctness bugs. Three notes were dismissed:
  - The settings x could leave the safe area only if a settings line were wider than it,
    and the native test holds every line to that width.
  - The GPS ball's and the win ring's radii could go negative only on a panel far smaller
    than any we ship; the smallest radius on the current panels is 28 px.
  - The scattering text overlapping the balls is by design, as the Definition of Done
    states.
- All three firmware envs and the three probe envs build, and cppcheck is clean.
- **Deviation from the Design:** the screen strings moved into `src/render/screen_text.h`,
  so the native test checks the game's real text rather than copies of it.
- **Follow-ups in the ledger:**
  - the round-panel captures that need a fix;
  - the StickC's case margins;
  - a setting to hide the radar's HUD text.
