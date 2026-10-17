#include "blip_blink.h"

BlinkState BlipBlink::update(uint32_t nowMs) {
    uint32_t cycle = nowMs / BLIP_PERIOD_MS;
    bool lit = nowMs % BLIP_PERIOD_MS < BLIP_LIT_MS;
    // Counting the cycle that last beeped, rather than comparing with the previous
    // frame, gives one onset per lit phase however late the frames that see it are.
    bool onset = lit && (!_hasOnset || cycle != _lastOnsetCycle);
    if (onset) {
        _hasOnset = true;
        _lastOnsetCycle = cycle;
    }
    return {lit, onset};
}
