---
id: '0008'
title: "Releases: the git tag is the version, offsets come from the build, libraries are pinned"
type: architecture
status: accepted
date: 2026-10-14
supersedes: []
superseded_by: []
---

## Context

The repo is about to go public with its first release, and nothing about a release was
defined yet:

- The title screen drew a hardcoded `VERSION_LABEL = "v1.0.0"` (`src/main.cpp`). No tag
  existed, so the label described nothing, and a tag would have had to be copied into the
  source by hand on every release.
- The three boards use three chips, and the flash layout differs between them. Measured
  on 2026-10-14 with `pio project metadata --json-output` and the built partition tables:

  | env | chip | bootloader | partitions | boot_app0 | app (`ota_0`) | bootloader header |
  |---|---|---|---|---|---|---|
  | `custom` | ESP32-C3 | `0x0` | `0x8000` | `0xe000` | `0x10000` | `0x02` (dio) |
  | `cardputer` | ESP32-S3 | `0x0` | `0x8000` | `0xe000` | `0x10000` | `0x02` (dio) |
  | `m5stickc-plus2` | ESP32 | `0x1000` | `0x8000` | `0xe000` | `0x10000` | `0x02` (dio) |

  A factory image built from hardcoded S3 offsets would put the StickC's bootloader in
  the wrong place.
- The adversary project shipped a factory image whose bootloader header said qio. The S3
  ROM reads the second-stage bootloader in the header's mode before quad mode is enabled,
  so the device reset-looped; `esptool image_info` did not catch it. Its release job now
  forces dio and checks the header byte before publishing.
- Libraries were declared with caret ranges. `lovyan03/LovyanGFX @ ^1.1.16` already
  resolved to 1.2.19 and `mikalhart/TinyGPSPlus @ ^1.0.3` to 1.1.0, so the version a
  release was built against depended on when the build machine last fetched.

## Decision

- **The git tag is the version.** Tags are `vMAJOR.MINOR.PATCH`; the first is `v0.1.0`.
  The release job passes the tag into the build as `FIRMWARE_VERSION`
  (`src/firmware_version.h`), and the title screen draws it. Every other build is a
  development build and draws `dev`. Packaging refuses a `firmware.bin` that does not
  contain the tag string, so a release whose flag went missing cannot ship as `dev`.
- **Each board ships two images**, named `dragon-radar-<env>-<version>-factory.bin` and
  `dragon-radar-<env>-<version>-app.bin`, plus one `SHA256SUMS.txt` for the release. The
  factory image is the whole flash from `0x0` (bootloader, partition table, boot_app0 and
  the app) for esptool or a web flasher. The app image is `firmware.bin` unchanged, for an
  app loader such as M5Launcher.
- **Offsets come from the build, never from a table in CI.** The bootloader, partition
  table and boot_app0 offsets are read from `pio project metadata`, the app offset from
  the built partition table (`factory`, else `ota_0`), and the chip from the board
  manifest. `scripts/package-release.mjs` does this, so a board or partition change
  cannot leave a stale offset behind.
- **The factory image is written dio and checked.** Packaging passes `--flash-mode dio`
  and `--target-offset 0x0` to `esptool merge-bin`, and fails unless the flash-mode byte
  of the bootloader header is `0x02`. That byte is read at the bootloader's offset, not
  at the start of the file: on the classic ESP32 the image starts with `0x1000` bytes of
  `0xFF` padding, and a first version of the check read that padding and refused a good
  StickC image.
- **Libraries are pinned to exact versions** in `platformio.ini`, at the versions
  resolved above (and `pololu/LSM303 @ 3.0.1`). Bumping one is a deliberate change that
  is built in every env, as invariant #2 already requires of shared code.
- A browser-based web flasher is deferred (see `LEDGER.md`). The factory image is the
  format it would serve, so adding one later needs no change to these artifacts.

## Consequences

- A tag push is the whole release procedure; there is no version to bump in the source.
- Builds outside the release job show `dev`, so a photo of the title screen tells a
  release from a local build.
- A new board needs no release-side change beyond its row in the release matrix.
- Library updates no longer arrive silently; they have to be picked up by hand.
