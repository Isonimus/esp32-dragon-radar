---
id: '0001'
title: "Hardware abstraction layer: per-board folders behind abstract interfaces, selected at build time"
type: architecture
status: accepted
date: 2026-10-13
supersedes: []
superseded_by: []
---

## Context

The game started on one board and was being ported to three: a custom ESP32-C3 build
(GC9A01 round display, NEO-6M GPS, LSM303 compass, one KY-040 push button), the M5Stack
Cardputer ADV (ESP32-S3, keyboard, ES8311 codec, no magnetometer) and the M5StickC Plus 2
(ESP32, two buttons, buzzer, MPU6886 IMU). Before the port, board differences lived in
`#ifdef` blocks and per-board pin headers (`pins_config.h`, `pins_config_m5stickc.h`)
spread through `main.cpp` and the drivers, so every new board touched game code.

The boards differ in every subsystem — display geometry (round 240×240 vs 135×240),
input (one button with gestures, two buttons, a full keyboard), audio (MP3 over I2S vs a
passive buzzer vs none), and heading sensors (compass, accelerometer-only IMU, none).

M5Unified/M5GFX was considered as the abstraction, since it already covers both M5 boards.
It was rejected: it ships its own `lgfx` namespace, which conflicts with the LovyanGFX
the custom board's GC9A01 panel depends on, and it does not cover the custom board at all.

## Decision

- Game code (`main.cpp`, `game/`, `render/`) talks only to abstract interfaces in
  `src/hal/`: `DisplayHAL`, `InputHAL`, `AudioHAL`, `GpsHAL`, `ImuHAL`, `PowerHAL`,
  bundled as `HardwareHAL` and obtained through `getHAL()`.
- Each board implements every interface in its own folder, `src/hal/boards/<board>/`,
  with statically allocated instances. A board without a subsystem provides a no-op
  implementation that reports itself unavailable, rather than the game testing for the
  board.
- The board is chosen at **build time**: each PlatformIO env's `build_src_filter`
  compiles exactly one board folder. There is no runtime board detection.
- Input is reduced to abstract `InputAction`s (PRIMARY, SECONDARY, their long-press
  forms, UP, DOWN). Boards with discrete keys share `PressGesture` (`src/hal/`) for
  tap/long-press, so the timing rules live in one host-tested place.
- LovyanGFX is the only graphics library on every board; each board supplies an explicit
  panel configuration. LovyanGFX's autodetect has no Cardputer entry, so it is not relied on.
- `InputHAL::begin()` reports failure, and boot halts with an on-screen error when input
  does not come up: a game that cannot be driven must not appear to work.

## Consequences

- Adding a board means adding one folder and one env; game code does not change.
- Board logic that is pure (key decoding, gestures) is kept free of Arduino calls so the
  native env can test it; hardware access stays in thin wrappers verified on device.
- Only one board's code compiles per env, so a change to an interface must be built in
  every env to be verified — a single-env build proves nothing about the others.
- The other `begin()` methods still return `void` and `setup()` reports them "OK"
  unconditionally; that gap is recorded in `LEDGER.md`.
