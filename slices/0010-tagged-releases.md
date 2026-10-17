---
id: '0010'
title: "Tagged releases: firmware images for every board on a version tag"
type: slice
status: accepted
date: 2026-10-14
supersedes: []
superseded_by: []
---

## Goal

Pushing a `v*` tag publishes a GitHub Release with flashable images for all three boards,
built as ADR-0008 lays out. It also replaces the hardcoded `v1.0.0` on the title screen
with the version the build was given.

## Definition of Done

- **Given** a build outside the release job
- **When** the title screen is drawn
- **Then** the version label reads `dev`

- **Given** a release build with `FIRMWARE_VERSION` set to `v0.1.0`
- **When** `scripts/package-release.mjs` packages it
- **Then** `dist/` holds `dragon-radar-<env>-v0.1.0-factory.bin` and `-app.bin`, the factory image's bootloader header byte is `0x02`, and the bootloader sits at the offset PlatformIO reports for that chip

- **Given** a `firmware.bin` that does not contain the version string it is being released as
- **When** it is packaged
- **Then** packaging fails and names the missing version

- **Given** a partition table with neither a `factory` nor an `ota_0` app partition, or a factory image whose header is not dio
- **When** it is packaged
- **Then** packaging fails rather than guessing an offset or publishing the image

- **Given** a pushed tag `vX.Y.Z`
- **When** the `Release` workflow runs
- **Then** a GitHub Release named `Dragon Radar vX.Y.Z` holds both images for each board and a `SHA256SUMS.txt` covering them

## Design

`release.yml` follows adversary's: build, merge with a pinned `esptool==5.2.0`, check the
dio byte, then `gh release create --generate-notes`. Two departures:

- **The offsets and the chip are not in the workflow.** Adversary hardcodes the S3's
  layout for its single board. Here the three chips differ (the classic ESP32's
  bootloader is at `0x1000`), so `scripts/package-release.mjs` reads them from
  `pio project metadata`, the built `partitions.bin` and the board manifest (ADR-0008).
  Its parsing and checks are pure functions with `node:test` tests; only the `pio` and
  `esptool` calls touch the outside world.
- **Native tests gate the build.** A tag is not proof that its commit passed CI, so the
  release runs `pio test -e native` first and the board builds need it.
- **Only `vMAJOR.MINOR.PATCH` tags trigger it**, and steps read the tag from the
  environment rather than through `${{ }}` inside a script, so a tag name can carry no
  shell text into a job. Both came from the adversarial review at wrap-up.
- **A build job per board, then one publish job.** The board builds run as a matrix and
  upload their images as workflow artifacts. A single job downloads them, writes one
  `SHA256SUMS.txt` and creates the release, so a failed board leaves no half-published
  release.

The version reaches the compiler through `PLATFORMIO_BUILD_FLAGS`, which PlatformIO adds
to every env, and `src/firmware_version.h` defaults it to `dev`. The variable is set for
the whole build job, not only the build step. Measured in the local dry run: running
`pio project metadata` without the flags the build used made PlatformIO wipe all three
build directories, so packaging found no `firmware.bin`. Adversary's stable-named
image copy and Pages redeploy serve its web flasher; both are left out until a web
flasher exists here.

## Verification

- `scripts/package-release.test.mjs` (`node --test scripts/*.test.mjs`, run in CI by slice 0009):
  reads the app offset from a `factory` and an `ota_0` table, rejects a table with neither
  and an erased table, reads the flash-mode byte at the bootloader's offset (`0x0`, and
  `0x1000` behind padding), rejects an image too short for the header, finds and misses
  a version string, and builds the `merge-bin` arguments in offset order for flashing at
  `0x0`. Three hand-made mutations of the script (wrong `ota_0` subtype, no offset sort,
  factory partition ignored) each failed at least one test.
- Local dry run on 2026-10-14: each board built with
  `PLATFORMIO_BUILD_FLAGS='-DFIRMWARE_VERSION=\"v0.1.0\"'` and packaged with
  `node scripts/package-release.mjs`, with the flags exported for both steps as the
  workflow does. Each factory image was checked byte for byte: every part PlatformIO
  lists sits at its reported offset, the bytes before the bootloader are `0xFF`, the
  header byte is `0x02`, the app image holds `v0.1.0`, and `sha256sum -c` passes.
- The `Release` workflow itself runs only on GitHub. Its proof is the `v0.1.0` release,
  tracked in `LEDGER.md` until it has been seen.
