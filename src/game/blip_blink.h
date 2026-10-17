#ifndef BLIP_BLINK_H
#define BLIP_BLINK_H

#include <stdint.h>

// The radar's blips blink together, as on the anime device, and the radar beeps as they
// light. The rhythm is the old sweep's, one turn of 36 frames at 20 Hz, so players keep
// the beat they know.
constexpr uint32_t BLIP_PERIOD_MS = 1800;
constexpr uint32_t BLIP_LIT_MS = 900;

struct BlinkState {
    bool lit;
    // A lit phase began since the previous update: the moment to beep.
    bool onset;
};

/**
 * @brief The blips' shared blink cycle, lit for BLIP_LIT_MS of every BLIP_PERIOD_MS.
 *
 * The cycle follows the clock, not the calls, so a late frame neither stretches it nor
 * adds a beep, and a lit phase whose start a frame missed still beeps once. Only a stall
 * longer than the whole lit phase loses that phase's beep.
 */
class BlipBlink {
public:
    BlinkState update(uint32_t nowMs);

private:
    bool _hasOnset = false;
    uint32_t _lastOnsetCycle = 0;
};

#endif // BLIP_BLINK_H
