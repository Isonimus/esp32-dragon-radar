---
id: '0014'
title: "Screens lay out in a safe area: the panel on rectangular boards, the inscribed square on the round one"
type: architecture
status: accepted
date: 2026-10-16
supersedes: []
superseded_by: []
---

## Context

The custom board's GC9A01 panel is a 240 px circle. Every screen except the radar's cached
background places its content from the canvas's width and height and from corner offsets
tuned on the 240 × 135 panels of the Cardputer and the StickC. Measured from the drawing
code's coordinates, the share of each element's pixels that falls outside the circle:

| Element | Outside the circle |
|---|---|
| Radar HUD "Sats" and "Balls", the zoom legend | 96 to 100% |
| Inline title logo (settings, GPS wait, scattering) | 48% |
| "No Fix (Seek Clear Sky)", "SCATTERING BALLS..." | 45 to 47% |
| "3-STAR BALL FOUND!", "Press to continue..." | 42% |
| "SHENRON SUMMONED!" | 25% |
| Stacked title logo (menu) | 11%, mostly its transparent corners |
| Menu items, settings lines, splash capsule | 0% |

Three ways were weighed:

- **Hand-tuned round branches per screen.** Two sets of coordinates on every screen, and
  nothing stops the next screen from being cut.
- **A chord-aware layout** that also knows the circle's width at each row, so text in the
  top and bottom caps can use it. More logic and tests, for a few rows.
- **One safe area for every screen.** Chosen.

## Decision

- Every screen lays out inside a **safe area** computed by a pure function from the
  panel's size and `DisplayHAL::isRound()`:
  - on a rectangular panel, the whole panel; the existing case margins are offsets inside it;
  - on a round panel, the largest square whose pixels all lie inside the circle: 168 × 168
    from (36, 36) on the 240 px panel.
- Content is positioned from the safe area, never from the raw panel size. The exceptions
  are full-panel decoration (backgrounds, the menu's footer band, the radar's cached layer
  and what is drawn on its glass, such as blips and the edge markers), which may run to the
  panel's edge and be clipped.
- **Fit is decided by measure, not by board.** The title uses the inline logo when it fits
  the safe area's width and the stacked one otherwise; the version string sits beside the
  logo when that fits and under it otherwise; a line of text too wide for the safe area is
  word-wrapped and centred.
- On the round panel the radar HUD's "Sats" and "Balls" sit at the safe area's top
  corners, over the glass, and the zoom legend at its bottom centre.

## Consequences

- One code path serves every board, and a new screen is safe on the round panel as long as
  it positions from the safe area. This becomes standing invariant #7 in `CLAUDE.md`.
- The round panel's top and bottom caps stay empty except for decoration. If a screen ever
  runs short of room there, the chord-aware layout is the next step, as a new decision.
- On the round panel every screen gets the stacked logo, which is 64 px tall against the
  inline one's 30, so the bodies below it scale their graphics to the room left.
