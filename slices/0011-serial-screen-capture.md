---
id: '0011'
title: "Serial screen capture: the frame on the display, saved as a PNG on the host"
type: slice
status: accepted
date: 2026-10-14
supersedes: []
superseded_by: []
---

## Goal

The README shows only the title logo. Photos of a small TFT come out with glare,
moiré and blown-out colour, so screenshots should come from the firmware itself. Every
screen is drawn into one 16-bit canvas and then pushed to the panel, so after any frame
that canvas holds exactly what is on the display. This slice sends it over the USB
serial link on request, and a host script turns it into a PNG. The first use is three
README shots taken before `v0.1.0`: the main menu, the satellite-search screen with all
seven stars lit, and the radar.

## Definition of Done

- **Given** any firmware build, release builds included
- **When** the host sends the byte `S` over the serial console
- **Then** the board answers with a `DRSCREEN <width> <height> <bytes>` line, the canvas's raw pixels and a `DREND` line, and the game carries on from where it was

- **Given** the firmware receives bytes other than `S`
- **When** the loop reads them
- **Then** it sends no frame for them and logs one line counting them, not one per byte

- **Given** the board cannot send the whole frame (its USB TX timed out on a slow host)
- **When** the write comes up short
- **Then** it logs how many bytes went out, sends no `DREND`, and the host refuses the frame

- **Given** a frame of big-endian RGB565 pixels, the order LovyanGFX keeps a 16-bit sprite in
- **When** the host decodes it
- **Then** full red, green, blue and white come out as 8-bit channels at 255 and black at 0, and a decoder that read the bytes the other way round fails the test

- **Given** boot or game log lines on the port ahead of the frame
- **When** the host reads a frame
- **Then** it skips them and still decodes the frame

- **Given** a frame that ends early, is not followed by `DREND`, or declares a byte count other than width × height × 2
- **When** the host reads it
- **Then** it stops with an error naming which of these went wrong and writes no PNG

- **Given** `--count N` and `--scale K`
- **When** the capture runs
- **Then** it writes N numbered PNGs, each pixel drawn as a K × K block with no smoothing

- **Given** a board that is already running a game
- **When** the host opens the port to capture
- **Then** the board does not reset, and if it does, the host stops and says the board reset rather than timing out

## Design

**Firmware: `src/debug/screen_capture.{h,cpp}`**, one function, `serviceScreenCapture`,
called at the top of `loop()` with the canvas and `Serial`. At that point the canvas holds
the frame the previous loop pushed, complete, because rendering is synchronous.

- **Compiled into every build**, not behind a flag. It costs one function, and every CI
  build compiles it. A flagged tool is compiled only when someone needs it, and that is
  when it is found broken. Nothing else reads serial input. The frame contains only what
  the screen shows, and no screen shows coordinates.
- **Serial, not the HAL.** `main.cpp` already logs to `Serial` on every board, so the
  console is common ground rather than board hardware, and invariant #1 is not touched.
  Taking a `Stream&` keeps the function free of which port that is.
- **The protocol is a header line, the bytes, and an end line.** The host validates it
  all, so the firmware stays dumb. The header carries the sprite's real
  `bufferLength()`, not a computed one. If the canvas ever stopped being a packed 16-bit
  buffer, the host would refuse it instead of decoding garbage. The `DREND` line
  catches any log line another task writes into the middle of the frame: the inserted
  bytes push the frame's tail into the place where `DREND` should be.
- **A short write abandons the frame.** On the S3 and C3, `HWCDC::write` drops what the
  host does not read within its 100 ms TX timeout and returns a short count. The firmware
  then logs the count instead of `DREND`, so the remains are never taken for a frame. Stray
  input bytes are counted and logged once per loop. A line per byte would let line noise
  flood the console and push a queued request's reply past the host's 20-line limit.
- **Cost while capturing:** the loop blocks while the frame goes out. That takes about
  64 KB for the Cardputer's 240 × 135 frame and 113 KB for the custom board's 240 × 240.
  Over the S3's and C3's native USB that is well under a second. Over the StickC's
  115200-baud USB-UART bridge it is about 6 s. GPS sentences arriving during the dump
  overflow the 256-byte receive buffer and are lost, which TinyGPSPlus counts as failed
  checksums. The fix resumes on the next sentence.

**Host: `scripts/capture_screen.py`**, Python like `render-logo.py`. It uses pyserial
(PlatformIO already installs it) and Pillow (`render-logo.py` already needs it). The name
uses an underscore so its tests can import it.

- `read_frame` reads one frame from anything with `readline` and `read`, so the tests
  drive it with an in-memory stream. `decode_rgb565_be` turns the bytes into RGB. Both
  are pure and tested. Only `main` touches the port and the disk.
- **The port is opened with DTR and RTS set low.** pyserial raises both by default and
  applies DTR before RTS, which passes through RTS high with DTR low. That is the reset
  state of the StickC's auto-reset circuit and of the S3's and C3's USB-serial
  peripheral. A reset would cost the GPS fix the radar shot needs. Whether the kernel's
  own raise of both lines at open still pulses a reset is not settled without hardware,
  so the host also recognises the ROM's `rst:0x` boot line and names the reset. It does
  not wait out a timeout instead.
- **Bursts.** The satellite-search screen lights its stars one at a time, 400 ms each,
  and shows all seven only in the last 400 ms of every 2.8 s cycle. The radar's sweep
  line moves every frame. One capture is a lottery, so `--count` takes a burst and the
  best frame is picked by eye.
- **Upscaling** is nearest-neighbour (`--scale`, default 3), so a 240 × 135 frame becomes
  720 × 405 with sharp pixels instead of a blur.

## Verification

- `scripts/test_capture_screen.py` (`python3 -m unittest discover -s scripts`, run in CI
  by the `host scripts` job). It tests the RGB565 decoding: the primaries, white and
  black, and a byte-order check that a swapped decoder fails. It also tests log lines
  skipped before the header, and the three refusals (short frame, missing `DREND`,
  byte count not width × height × 2). Two tests check that the host gives up rather than
  waiting forever: one for a silent port, after one timeout, and one for a board that
  only logs, after 20 lines. One checks that a reset is named. A fake board drives a
  three-frame burst, checking the numbering, the scale and that each file holds its own
  frame. Another checks that scaling turns each pixel into a K × K block. Hand-made
  mutants of each guard (byte order, `DREND`, length, early exit, line cap, reset banner,
  numbering, scale, nearest-neighbour) each failed at least one test.
- All three firmware envs build with the capture compiled in (CI, slice 0009).
- On hardware: the three README screenshots are taken with the script on the Cardputer
  ADV mid-game. The board did not reset when the port opened, and the game carried on
  after each burst. Until this has been done, it is tracked in `LEDGER.md`.

## Amendment — 2026-10-15: DTR and RTS are held high, not low

The Design above says the port is opened with DTR and RTS set low. That was wrong.
It was measured on the Cardputer ADV when the burst was first run.

- **Both lines low:** the board reset on every open, and the stream showed
  `rst:0x15 (USB_UART_CHIP_RESET)` and a fresh boot.
- **Both lines high** (pyserial's default): none of three opens reset the board, and
  each frame arrived in about 0.4 s.

The kernel raises both lines when the port opens. Lowering them afterwards passes through
DTR low with RTS high, the state that resets the chip, so lowering them causes the very
reset it was meant to prevent. `open_port` now sets both high, and a test checks the
line states the port is opened with.

One probe looked as if it had not reset. It waited 0.5 s before clearing the input
buffer, which flushed the boot banner. The request sent after that was answered by the
new boot, so a frame alone does not prove there was no reset. Only the raw stream
shows it.
