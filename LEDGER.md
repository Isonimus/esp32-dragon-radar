# Ledger

The single mutable file in this repo (stele:ADR-0001). Everything else is
immutable or generated. Open work, deferrals, and known defects all live here — there is
no second tracking file, because two files require manual sync and manual sync does not
happen.

**Closing an item means deleting its line here.** Do not annotate the source ADR; the
ADR's claim ("at the time of this decision we deferred X") stays true forever and needs
no update. Single writer, one direction.

An item opened to outside contributors moves to a GitHub issue, and its line here is
deleted in the same change; one item never lives in both (ADR-0016).

Format: `- [type] description (ADR-NNNN)` — type is `bug` | `feature` | `deferred` |
`audit`. Cite the source ADR where one exists; rule 8 checks that the citation resolves.
A decision that lives in **another** repo is cited `<repo>:ADR-NNNN` — the linter cannot
open that corpus, so it skips qualified references (stele:ADR-0009).

## Open

<!-- One line per open item. Delete the line to close it; the git log is the done-record. -->

- [audit] Walk-test radar rotation on the M5StickC with a GPS fix: it should turn with the direction of travel and hold when stopped (ADR-0002)
- [audit] Watch the M5StickC for phantom or dead button presses now that GPIO 37/39 use `INPUT` (external pull-ups) instead of `INPUT_PULLUP` (ADR-0001)
- [audit] On the M5StickC, confirm the radar beep, menu theme and fanfares sound as before now that they run through `ToneSequencer` without blocking (ADR-0004)
- [deferred] The ADV's BMI270 IMU is not driven (ADR-0003)
- [deferred] Menu theme fidelity: against channels 1-2 of `data/recdbz-clean.mid` (removed before the public release; still in the `archive/pre-public-squash` tag) the table lacks two lead-in A#4s, a G#4 after the C#5 B4 A4 C#5 fill and the chordal closing section (beats 103-122), note 26 lasts 239 ms where the MIDI has 119, and its flat 800 ms rests drift phrases up to 3.9 s off the beat. A MIDI-generated table fixing all of that (rests cut to <=2 beats) sounded worse on the ADV on 2026-10-13 and was reverted; revisit by ear, one change at a time (ADR-0004)
- [audit] The m5stickc env no longer sets `board_build.variant`; confirm the `esp32dev` variant matches the Plus 2's flash and PSRAM
- [audit] Cardputer ADV GPS: confirm a fix on the Grove unit (the cap got one outdoors on 2026-10-13) (ADR-0005)
- [audit] The custom board's GPS module is a GY-NEO6MV2 breakout with a clone chip, not a u-blox NEO-6M: it reports `SW=URANUS5` and `$GN` sentences (a GPS + BeiDou clone that takes CASIC `$PCAS` commands), and outdoors it heard 14 satellites at only 13-29 dB-Hz, so its first fix on 2026-10-16 took over 20 minutes. Try a better antenna and measure the time to first fix
- [audit] Cardputer ADV GPS: watch `failed` in the `[GPS cap]` line on the radar screen. Both UARTs use the default 256-byte RX buffer, about 22 ms at 115200, and the loop drains them once per frame; GPS_WAIT showed `failed=0` over 60 s, but radar redraws are heavier (ADR-0005)
- [audit] On the custom board's round panel, capture the scattering, radar (HUD at the safe area's top corners, zoom legend and LOST FIX at its bottom centre), ball found and win screens once it has a GPS fix; slice 0015 verified only the menu, settings and GPS wait screens there, since there was no fix indoors (ADR-0014)
- [audit] Check the M5StickC Plus2's case margins: whether its case hides any panel rows or columns the safe area (the whole panel on rectangular boards) lets screens use (ADR-0014)
- [audit] The custom board (ESP32-C3) had 31,904 bytes of free heap at its lowest in play once the radar's background was cached (slice 0013). Re-measure with the `[radar] min_free_heap` log whenever a change allocates at runtime; if the margin gets tight, store only the circle's pixels per row on the round panel, which saves about 25 KB (ADR-0012)
- [deferred] Define a static allocation policy for core game systems (`main.cpp` still `new`s the canvas and renderer)
- [audit] Flash the `v0.1.0` factory image at `0x0` on the Cardputer ADV and the custom board and check the title shows `v0.1.0`; the StickC's booted and showed it on 2026-10-17 (ADR-0010)
- [audit] After flashing the StickC's `v0.1.0` factory image on 2026-10-17 the menu offered only New Game, so the image looks to overwrite NVS and erase the saved game and settings. Confirm it with a known save; if so, say it in the README's "Installing a release" and give the `-app.bin` offset for an upgrade that keeps them (ADR-0008)

## Resolved

Entries move out of "Open" by deletion. A narrative of what merely *happened* belongs in
the git log; what belongs on an open item is evidence it cannot be re-derived later — a
measured result, a reproduction, a count the eventual decision turns on. Keep that with
the item while it is open, and move it into the ADR that closes it. What this file is not
is a record of completed work.
