---
id: '0004'
title: "Board audio: one shared tone sequencer; the Cardputer ADV synthesises square waves over I2S"
type: architecture
status: accepted
date: 2026-10-13
supersedes: []
superseded_by: []
---

## Context

The Cardputer port played a single MP3 beep through ESP32-audioI2S and left every melody
a no-op. The first flash of a Cardputer ADV (2026-10-13) showed the speaker could never
work that way:

```
audioI2S requires PSRAM!
OOM: failed to allocate 294912 bytes for m_samplesBuff48K
Setup: Audio OK
```

ESP32-audioI2S 3.x needs PSRAM for its decode buffers, and the ADV booted with none, so
no setting makes MP3 playback work on this board. (The "Audio OK" is the unchecked
`begin()` gap ADR-0001 already records.)

The M5StickC already played every tune as square-wave tones on its buzzer, through
`BuzzerManager`. That class mixed the tune data, the timing rules and the pin driving,
blocked the game loop with `delay()` for up to ~2.5 s during the fanfares, and declared the
menu theme 69 notes long when its tables held 68, reading one past the end of both.

The ×4 ES8311 clock multiplier in the first ADV driver was derived from ESP32-audioI2S's
32-bit slots and never observed on hardware; with that library gone it has no basis.

## Decision

- Tunes are data: `src/audio/melodies` holds every note table once, with a per-melody
  legato so repeated notes stay distinct. Lengths are computed from the tables.
- `ToneSequencer` (`src/audio/`) is the single, pure, non-blocking source of "which
  frequency sounds now": the radar beep never interrupts a melody or restarts itself, a
  melody cuts off whatever is playing, and notes advance on their own timeline so a slow
  frame does not stretch the tune. Each board only turns the frequency into a signal.
- M5StickC: `tone()`/`noTone()` on the buzzer pin, called only when the pitch changes.
  `BuzzerManager` is deleted.
- Cardputer ADV: `SquareWave` (`src/audio/`, pure) renders 16-bit stereo PCM at 48 kHz,
  and a FreeRTOS task above the loop's priority writes it through the core's `ESP_I2S`
  driver in 5 ms blocks. 16-bit slots at 48 kHz are what M5Unified sends this codec, so the
  ES8311 sequence is copied from it without change (×8).
- ESP32-audioI2S, `AudioManager`, `data/radar_beep.mp3` and the Cardputer LittleFS image are
  removed; flashing no longer needs `uploadfs`.

## Consequences

- Every board with sound plays the same tunes with the same rules, and those rules are
  host-tested (`test/test_tone_sequencer`, `test/test_square_wave`).
- The Cardputer gains the melodies it never had, and no board blocks the loop to play one.
- Sound is square-wave only; sampled audio would need PSRAM or a new decision.
- The ADV's loudness (one eighth of full scale) is a starting point, not a measurement;
  it and audible output on the ADV are open in `LEDGER.md`.
