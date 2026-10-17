---
id: '0006'
title: "Title logo: pre-rendered from an Apache-licensed font into an ARGB8888 header"
type: architecture
status: accepted
date: 2026-10-13
supersedes: []
superseded_by: []
---

## Context

The menu, settings and loading screens titled the game with LovyanGFX's built-in
6×8 font at size 3. The operator asked for lettering closer to the Dragon Ball Z title
card: yellow "DRAGON", red "RADAR", a thick black outline, bouncing hand-drawn capitals
and a dragon ball in place of an O.

That lettering is custom artwork, not a typeface, and the trademark of its owners. The
closest fan-made font, Saiyan Sans, is only listed by an aggregator that names no
designer and links no licence file, so it cannot be vendored with confidence. Six
openly licensed display fonts were rendered with the same styling and compared on the
menu background; Luckiest Guy (Astigmatic, Apache 2.0, from the Google Fonts repository)
was the closest and the operator chose it.

Every supported screen is 240 px wide. The settings, GPS-wait and scattering screens start
their content ~36 px from the top, which leaves room for one line of lettering only. The
main menu's first item starts ~70 px down, which fits the title card's two stacked lines.

## Decision

- `scripts/render-logo.py` (Python 3 + Pillow, run by hand) draws the title from
  `data/fonts/LuckiestGuy-Regular.ttf` and writes `src/render/dragon_radar_logo.h`, two
  row-major `0xAARRGGBB` arrays: a 232×30 one-line title for the settings and loading
  screens, and a 157×64 stacked title ("RADAR" set in under "DRAGON") for the main menu.
  The header is generated: it is never edited by hand, only regenerated.
- The firmware blends them with `pushAlphaImage` as `lgfx::argb8888_t`, so each asset
  works on any background colour and every board, and no font rasteriser ships on the
  device.
- The font's licence ships beside it as `data/fonts/LuckiestGuy-LICENSE.txt`.

## Consequences

- The two logos cost ~68 KB of flash and no RAM (the arrays are `const`).
- Changing the title's text, colours or styling means editing and re-running the
  script; the generated header and the script change in the same commit.
- Rebuilding the header needs Pillow on the developer's machine; building the firmware
  does not.
- The styling still evokes a trademarked logo. That is fine for a personal build and
  would need a second look before the firmware is distributed publicly.
