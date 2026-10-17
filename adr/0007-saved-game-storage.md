---
id: '0007'
title: "Saved game and settings: a StorageHAL with one shared NVS implementation, keeping the existing keys"
type: architecture
status: accepted
date: 2026-10-13
supersedes: []
superseded_by: []
---

## Context

`BallManager` (`src/game/`) opened the ESP32's NVS through Arduino's `Preferences`
inside `#ifdef ARDUINO` blocks, and `main.cpp` did the same for the settings screen's
choices. That was game code reaching hardware outside `src/hal/`,
against ADR-0001, and it left save and load untestable: in the native env every
persistence call compiled to nothing, so `hasSavedGame()` always answered false there.

On the boards, the menu showed "Continue (7/7)" for every saved game. The save was only
loaded when Continue was chosen, and `collectedCount()` was `7 - remainingCount()`, so an
empty ball list read as seven balls found. The fix needs the menu to read the save, and
the tests for it need a save to exist on the host.

Every supported board is an ESP32 running the same Arduino core, so `Preferences`
behaves identically on all three.

## Decision

- Persistence goes through `StorageHAL` (`src/hal/storage_hal.h`): load, save and clear
  the list of balls, and load and save the settings (`GameSettings`), each reporting
  failure. It is part of the `HardwareHAL` bundle; nothing outside it uses `Preferences`.
- One implementation, `NvsStorage` (`src/hal/nvs_storage.{h,cpp}`), serves every board.
  This departs from ADR-0001's "each board implements every interface in its own folder":
  the three copies would be identical, and a fourth board that is not an ESP32 can still
  supply its own. Boards list it in their `*_hal.cpp` like any other part.
- The saved-game format stays as it was: NVS namespace `dragon_radar`, `count` (uint8),
  and per ball `b<id>` (bool, collected), `lat<id>` and `lon<id>` (double), ids from 1.
  Settings stay in their own namespace, `dr_settings`, with `distIdx` (uint8) and
  `soundOn` (bool), so starting a new game, which deletes the saved game, keeps them.
  Saves already on devices keep loading.
- A save missing any ball's key is treated as no save and logged, rather than filled with
  defaults that would put a ball at 0,0. Incomplete settings fall back to the defaults.
- Native tests use an in-memory fake (`test/mocks/fake_storage.h`).

## Consequences

- `BallManager` is free of Arduino calls and its save/load behaviour is host-tested.
- The NVS key mapping itself is only exercised on a device; continuing a game after a
  reboot is its check.
- Renaming the namespace or a key orphans every existing save; a format change needs a
  new ADR and a migration or an explicit decision to drop old saves.
