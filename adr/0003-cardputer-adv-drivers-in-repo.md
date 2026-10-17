---
id: '0003'
title: "Cardputer ADV: small in-repo drivers over Wire, not the M5 libraries"
type: architecture
status: accepted
date: 2026-10-13
supersedes: []
superseded_by: []
---

## Context

The Cardputer ADV differs from the original Cardputer where the port's code assumed it
did not:

- Its keyboard sits behind a TCA8418 controller on the internal I2C bus (G8 SDA, G9 SCL,
  INT G11, address 0x34) instead of a 74HC138-scanned GPIO matrix. The ported scan code
  could not work on it, and also indexed past its own 7-row array.
- Its speaker is fed by an ES8311 codec (0x18) that is silent until configured over I2C.
- The I2S pins in the ported code had WS and data swapped (42/43). M5Unified configures
  BCLK 41, WS 43, data 42 for both Cardputer revisions.

M5Stack's `M5Cardputer` and `M5Unified` libraries handle all of this, but ADR-0001 rejected
M5Unified for its `lgfx` conflict, and pulling it in for one board would bring that
conflict back.

## Decision

Write the minimum drivers in the board folder, on Arduino `Wire`, taking register values
from M5Stack's own sources (read on 2026-10-13) rather than from memory:

- `cardputer_i2c` owns the shared internal bus, so the keyboard, codec and IMU do not each
  start it.
- `tca8418_keymap` decodes a FIFO event to the printed 4×14 layout, following
  `TCA8418KeyboardReader::remap` in M5Cardputer. It is pure and host-tested; the property
  test checks that the 56 wired positions land on 56 distinct printed keys.
- `CardputerInput` polls INT (G11) and drains the FIFO only when it is low; each event goes
  straight to its `PressGesture`, so a tap whose press and release share a frame is kept.
  Enter and Backspace mirror the M5StickC's two buttons.
- `es8311` replays M5Unified's speaker-enable sequence unchanged. Its ×8 BCLK multiplier
  assumes 16-bit stereo slots (BCLK = 32·fs), so whatever feeds the codec must send that
  format; ADR-0004 records how sound is produced.
- The display uses an explicit ST7789 configuration copied from M5GFX's Cardputer branch.

## Consequences

- No new library dependency, and the keymap, gesture and heading rules are host-tested.
- The register sequences are copies; if M5Stack changes the hardware or fixes a sequence,
  this repo will not follow automatically.
- The Enter/Backspace positions were confirmed on an ADV on 2026-10-13 (`[kbd]` log: Enter
  row 2 col 13, Backspace row 0 col 13). Audible output is recorded as open in `LEDGER.md`.
- The ADV's BMI270 IMU is not driven; ADR-0002 makes the board playable without it.
