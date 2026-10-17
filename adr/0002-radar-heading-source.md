---
id: '0002'
title: "Radar heading: GPS course while moving, compass when still"
type: architecture
status: accepted
date: 2026-10-13
supersedes: []
superseded_by: []
---

## Context

The radar rotates so "up" is the way the player faces. Two sources exist:

- **GPS course over ground** — available on every board, but only meaningful while the
  receiver moves; standing still, it is position jitter.
- **A compass** — only the custom board has one (LSM303). The M5StickC's MPU6886 and the
  Cardputer ADV's BMI270 have no magnetometer and cannot give an absolute heading.

The port's first rotation code used GPS course only inside `if (imu.available())`, so on
a board without a compass the radar stayed north-up even while walking. A regression
test written against that logic failed three cases: moving without a compass (0° instead
of the course), stopping without a compass (snapped to 0° instead of holding), and an
invalid fix whose stale course was still trusted.

## Decision

`RadarHeading::select` (`src/game/radar_heading.*`) picks the heading each frame:

1. Valid fix and speed above 1 m/s (`MIN_COURSE_SPEED_MPS`) → GPS course, remembered.
2. Otherwise, a compass if the board has one → compass heading.
3. Otherwise → the last remembered course; north-up until the first one arrives.

1 m/s is roughly walking pace; below it, course is dominated by jitter.

## Consequences

- Every board rotates with the direction of travel, so a board without a compass is
  playable on foot. This is what makes a Cardputer ADV build worth shipping.
- Turning on the spot without a compass does not rotate the radar until the player walks
  again; that is inherent to GPS, not a defect.
- The rule is covered by `test/test_heading`; changing it means a new ADR.
