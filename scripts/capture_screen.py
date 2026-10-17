#!/usr/bin/env python3
"""Save what a running board's display shows as PNGs, over its serial port (slice 0011).

The firmware answers an `S` byte with `DRSCREEN <width> <height> <bytes>`, the canvas's
pixels as big-endian RGB565, and `DREND`. This script asks, checks the reply and writes
the frame upscaled with sharp pixels.

Usage: python3 scripts/capture_screen.py --port /dev/ttyACM0 [--count 20] [--scale 3] [--out shots]
Requires pyserial (PlatformIO installs it) and Pillow. Run from the repo root.
"""
import argparse
import os
import time

import serial
from PIL import Image

CAPTURE_REQUEST = b"S"
HEADER_TAG = b"DRSCREEN"
END_LINE = b"DREND"
BYTES_PER_PIXEL = 2
# The request is sent right after the input buffer is cleared, so only lines another
# task printed while the board was reading it can come first. More than this means the
# firmware is not answering, which an endless stream of GPS log lines would otherwise hide.
MAX_LINES_BEFORE_HEADER = 20
# The ESP32 ROM's first line after a reset, e.g. "rst:0x1 (POWERON_RESET),boot:0x13".
RESET_BANNER = b"rst:0x"
# Ignored by the native USB on the Cardputer and the custom board; the StickC's
# USB-UART bridge runs at the console speed set in platformio.ini.
CONSOLE_BAUD = 115200


class FrameError(Exception):
    """The board's reply is not a whole, well-formed frame."""


def read_frame(stream):
    """Reads one reply from `stream` (anything with readline and read).

    Returns (width, height, pixel bytes). Log lines ahead of the header are skipped.
    """
    for _ in range(MAX_LINES_BEFORE_HEADER):
        line = stream.readline()
        if not line:
            break
        if line.startswith(HEADER_TAG):
            return _read_body(stream, line)
        if line.startswith(RESET_BANNER):
            raise FrameError(
                "the board reset when the port opened, so its game and GPS fix are gone; "
                "open_port must leave DTR and RTS high"
            )
    raise FrameError(
        "no DRSCREEN header from the board; is it running a firmware with screen capture?"
    )


def _read_body(stream, header):
    fields = header.split()
    if len(fields) != 4 or not all(field.isdigit() for field in fields[1:]):
        raise FrameError(f"malformed header {header!r}")
    width, height, declared = (int(field) for field in fields[1:])
    expected = width * height * BYTES_PER_PIXEL
    if declared != expected:
        raise FrameError(
            f"the board sends {declared} bytes for {width}x{height}, expected {expected}: "
            "the canvas is not a packed 16-bit buffer"
        )
    pixels = stream.read(declared)
    if len(pixels) != declared:
        raise FrameError(f"the frame ended early: got {len(pixels)} of {declared} bytes")
    trailer = stream.readline().strip()
    if trailer != END_LINE:
        raise FrameError(
            f"expected DREND after the pixels, got {trailer[:40]!r}: either a log line "
            "was written into the frame, or the board dropped bytes because this host "
            "read too slowly (it then logs how many it sent)"
        )
    return width, height, pixels


def decode_rgb565_be(pixels):
    """Big-endian RGB565, as LovyanGFX keeps a 16-bit sprite, to packed 8-bit RGB."""
    if len(pixels) % BYTES_PER_PIXEL:
        raise FrameError(f"odd byte count {len(pixels)} for 16-bit pixels")
    rgb = bytearray()
    for i in range(0, len(pixels), BYTES_PER_PIXEL):
        value = (pixels[i] << 8) | pixels[i + 1]
        red5, green6, blue5 = value >> 11, (value >> 5) & 0x3F, value & 0x1F
        # Repeating the top bits in the gap maps full scale to 255, not 248 or 252.
        rgb += bytes(((red5 << 3) | (red5 >> 2), (green6 << 2) | (green6 >> 4), (blue5 << 3) | (blue5 >> 2)))
    return bytes(rgb)


def upscale(width, height, rgb, scale):
    """Each pixel as a scale x scale block; smoothing would blur the panel's pixel art."""
    image = Image.frombytes("RGB", (width, height), rgb)
    return image.resize((width * scale, height * scale), Image.NEAREST)


def open_port(path, timeout):
    port = serial.Serial()
    port.port = path
    port.baudrate = CONSOLE_BAUD
    port.timeout = timeout
    # A reset loses the game and the GPS fix the radar shot needs. The kernel raises DTR and
    # RTS when the port opens; lowering them afterwards passes through DTR low with RTS
    # high, the line state that resets the chip. Measured on the Cardputer ADV on
    # 2026-10-15: both low reset it on every open, both high (pyserial's default, set here
    # so nobody "fixes" it to low) on none of three (slices/0011).
    port.dtr = True
    port.rts = True
    port.open()
    return port


def capture_burst(port, count, scale, out_dir, stamp):
    """Takes `count` frames from an open port, yielding each PNG's path once written."""
    for index in range(1, count + 1):
        port.reset_input_buffer()
        port.write(CAPTURE_REQUEST)
        width, height, pixels = read_frame(port)
        path = os.path.join(out_dir, f"screen-{stamp}-{index:02d}.png")
        upscale(width, height, decode_rgb565_be(pixels), scale).save(path)
        yield path


def positive_int(text):
    value = int(text)
    if value < 1:
        raise argparse.ArgumentTypeError(f"{value} is not at least 1")
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--port", required=True, help="serial port, e.g. /dev/ttyACM0")
    parser.add_argument("--count", type=positive_int, default=1, help="frames to take in a burst")
    parser.add_argument("--scale", type=positive_int, default=3, help="pixel size in the PNG")
    parser.add_argument("--out", default="shots", help="directory for the PNGs")
    parser.add_argument("--timeout", type=float, default=15.0,
                        help="seconds to wait for a reply; the StickC needs about 6 per frame")
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)
    stamp = time.strftime("%Y%m%d-%H%M%S")
    port = open_port(args.port, args.timeout)
    try:
        for path in capture_burst(port, args.count, args.scale, args.out, stamp):
            print(path)
    finally:
        port.close()


if __name__ == "__main__":
    main()
