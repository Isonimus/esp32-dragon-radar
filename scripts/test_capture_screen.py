"""Tests for capture_screen.py (slice 0011). Run: python3 -m unittest discover -s scripts"""
import io
import os
import tempfile
import unittest
from unittest import mock

from PIL import Image

import capture_screen
from capture_screen import FrameError, capture_burst, decode_rgb565_be, read_frame, upscale

RED_BE = bytes([0xF8, 0x00])
GREEN_BE = bytes([0x07, 0xE0])
BLUE_BE = bytes([0x00, 0x1F])
WHITE_BE = bytes([0xFF, 0xFF])
BLACK_BE = bytes([0x00, 0x00])


def frame_stream(width, height, pixels, declared_bytes=None, trailer=b"DREND\n", preamble=b""):
    """The bytes the firmware sends for one frame, optionally with log lines ahead of it."""
    declared = len(pixels) if declared_bytes is None else declared_bytes
    header = f"DRSCREEN {width} {height} {declared}\n".encode()
    return io.BytesIO(preamble + header + pixels + trailer)


class DecodeTest(unittest.TestCase):
    def test_primaries_white_and_black_expand_to_full_8_bit_channels(self):
        pixels = RED_BE + GREEN_BE + BLUE_BE + WHITE_BE + BLACK_BE
        self.assertEqual(
            decode_rgb565_be(pixels),
            bytes([255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0]),
        )

    def test_the_high_byte_comes_first(self):
        # Read little-endian, 0xF8 0x00 would be a dark blue-green, not red.
        self.assertEqual(decode_rgb565_be(bytes([0xF8, 0x00])), bytes([255, 0, 0]))
        self.assertEqual(decode_rgb565_be(bytes([0x00, 0xF8])), bytes([0, 28, 198]))

    def test_an_odd_byte_count_is_refused(self):
        with self.assertRaisesRegex(FrameError, "odd"):
            decode_rgb565_be(bytes([0xF8]))


class ReadFrameTest(unittest.TestCase):
    def test_reads_the_frame_after_skipping_log_lines(self):
        pixels = RED_BE + GREEN_BE
        stream = frame_stream(2, 1, pixels, preamble=b"[GPS cap] chars=12 sats=3\r\nZoom In\n")
        self.assertEqual(read_frame(stream), (2, 1, pixels))

    def test_a_frame_that_ends_early_is_refused(self):
        stream = frame_stream(2, 1, RED_BE, declared_bytes=4, trailer=b"")
        with self.assertRaisesRegex(FrameError, "2 of 4 bytes"):
            read_frame(stream)

    def test_a_frame_without_its_end_line_is_refused(self):
        # A log line written into the middle of the frame pushes its tail where DREND belongs.
        stream = frame_stream(2, 1, RED_BE + GREEN_BE, trailer=b"\xe0Zoom In\n")
        with self.assertRaisesRegex(FrameError, "DREND"):
            read_frame(stream)

    def test_a_byte_count_that_is_not_two_per_pixel_is_refused(self):
        # An 8-bit canvas would declare one byte per pixel.
        stream = frame_stream(2, 1, RED_BE, declared_bytes=2)
        with self.assertRaisesRegex(FrameError, "expected 4"):
            read_frame(stream)

    def test_no_header_before_the_port_goes_quiet_is_refused(self):
        with self.assertRaisesRegex(FrameError, "no DRSCREEN header"):
            read_frame(io.BytesIO(b"Setup: Canvas OK\n"))

    def test_a_board_that_logs_but_never_answers_is_refused(self):
        # Firmware without capture keeps printing GPS lines; the wait must still end.
        chatter = b"[GPS cap] chars=12 sats=3\n" * 25
        stream = frame_stream(2, 1, RED_BE, preamble=chatter)
        with self.assertRaisesRegex(FrameError, "no DRSCREEN header"):
            read_frame(stream)


class ResetTest(unittest.TestCase):
    def test_a_reset_on_open_is_named(self):
        stream = io.BytesIO(b"ets Jul 29 2019 12:21:46\n\nrst:0x1 (POWERON_RESET),boot:0x13\n")
        with self.assertRaisesRegex(FrameError, "reset"):
            read_frame(stream)


class RecordingSerial:
    """Stands in for serial.Serial and records the line states it is opened with."""

    def __init__(self):
        self.dtr = True
        self.rts = True
        self.opened_with = None

    def open(self):
        self.opened_with = (self.dtr, self.rts)


class OpenPortTest(unittest.TestCase):
    def test_the_port_opens_with_dtr_and_rts_high(self):
        # Measured on the Cardputer ADV on 2026-10-15: opening with both lines low reset
        # it (rst:0x15 USB_UART_CHIP_RESET) every time; with both high, 3 of 3 opens did not.
        with mock.patch.object(capture_screen.serial, "Serial", RecordingSerial):
            port = capture_screen.open_port("/dev/null", 1.0)
        self.assertEqual(port.opened_with, (True, True))


class FakeBoard:
    """Answers each S with a 2x1 frame whose first pixel is red, then blue, then green..."""

    COLOURS = [RED_BE, BLUE_BE, GREEN_BE]

    def __init__(self):
        self.reply = io.BytesIO()
        self.requests = 0

    def reset_input_buffer(self):
        self.reply = io.BytesIO()

    def write(self, data):
        assert data == b"S"
        pixels = self.COLOURS[self.requests % len(self.COLOURS)] + BLACK_BE
        self.requests += 1
        self.reply = frame_stream(2, 1, pixels, preamble=b"Zoom In\n")

    def readline(self):
        return self.reply.readline()

    def read(self, size):
        return self.reply.read(size)


class CaptureBurstTest(unittest.TestCase):
    def test_writes_one_numbered_scaled_png_per_frame(self):
        with tempfile.TemporaryDirectory() as out_dir:
            paths = list(capture_burst(FakeBoard(), 3, 2, out_dir, "stamp"))
            self.assertEqual(
                [os.path.basename(path) for path in paths],
                ["screen-stamp-01.png", "screen-stamp-02.png", "screen-stamp-03.png"],
            )
            firsts = []
            for path in paths:
                with Image.open(path) as image:
                    self.assertEqual(image.size, (4, 2))
                    firsts.append(image.convert("RGB").getpixel((1, 1)))
            self.assertEqual(firsts, [(255, 0, 0), (0, 0, 255), (0, 255, 0)])


class SilentPort:
    """A port whose every readline times out, as pyserial reports it: with b""."""

    def __init__(self):
        self.waits = 0

    def readline(self):
        self.waits += 1
        return b""


class SilentPortTest(unittest.TestCase):
    def test_a_silent_port_is_given_up_on_after_one_timeout(self):
        # Each readline on a live port waits out the full --timeout; twenty would be minutes.
        port = SilentPort()
        with self.assertRaisesRegex(FrameError, "no DRSCREEN header"):
            read_frame(port)
        self.assertEqual(port.waits, 1)


class UpscaleTest(unittest.TestCase):
    def test_each_pixel_becomes_a_solid_block(self):
        red, blue = bytes([255, 0, 0]), bytes([0, 0, 255])
        image = upscale(2, 1, red + blue, 3)
        self.assertEqual(image.size, (6, 3))
        for y in range(3):
            for x in range(3):
                self.assertEqual(image.getpixel((x, y)), (255, 0, 0))
                self.assertEqual(image.getpixel((x + 3, y)), (0, 0, 255))


if __name__ == "__main__":
    unittest.main()
