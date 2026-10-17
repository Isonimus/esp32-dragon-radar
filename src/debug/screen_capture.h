#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>

// Answers a host's request for the frame on the display, for README screenshots
// (slice 0011; scripts/capture_screen.py is the host side). Call once per loop, before
// anything draws, so `frame` still holds the whole frame the previous loop pushed.
void serviceScreenCapture(LGFX_Sprite& frame, Stream& port);
