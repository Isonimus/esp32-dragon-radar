#include "screen_capture.h"

namespace {
constexpr int CAPTURE_REQUEST = 'S';
}

void serviceScreenCapture(LGFX_Sprite& frame, Stream& port) {
    // Counted and reported once: a line per stray byte would let line noise or a
    // terminal flood the console, and push a queued request's reply past the host's
    // limit on log lines ahead of a frame.
    unsigned ignoredBytes = 0;
    while (port.available() > 0) {
        if (port.read() != CAPTURE_REQUEST) {
            ignoredBytes++;
            continue;
        }
        // The sprite's own length, not width x height x 2: the host checks the two agree,
        // so a canvas that stopped being packed 16-bit is refused there, not misdecoded.
        const uint32_t length = frame.bufferLength();
        port.printf("DRSCREEN %d %d %lu\n", frame.width(), frame.height(), (unsigned long)length);
        const size_t sent = port.write(static_cast<const uint8_t*>(frame.getBuffer()), length);
        if (sent != length) {
            // The native USB drops what the host does not read within its TX timeout. No
            // DREND, so the host cannot mistake the remains for a whole frame.
            port.printf("\n[capture] only %u of %lu bytes sent; frame abandoned\n",
                        (unsigned)sent, (unsigned long)length);
            continue;
        }
        port.print("DREND\n");
    }
    if (ignoredBytes > 0) {
        port.printf("[capture] ignored %u byte(s); send 'S' for a screenshot\n", ignoredBytes);
    }
}
