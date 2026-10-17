#include "es8311.h"
#include "cardputer_i2c.h"

static constexpr uint8_t ES8311_ADDRESS = 0x18;

struct RegisterWrite {
    uint8_t reg;
    uint8_t value;
};

// Sequence copied unchanged from M5Unified's _speaker_enabled_cb_cardputer_adv.
static constexpr RegisterWrite SPEAKER_ENABLE_SEQUENCE[] = {
    {0x00, 0x80},  // RESET: chip state machine on
    {0x01, 0xB5},  // CLK_MANAGER1: MCLK taken from BCLK (the ADV has no MCLK line)
    // CLK_MANAGER2: multiply BCLK ×8 to reach the 256·fs MCLK the DAC expects.
    // Only right for 16-bit stereo slots (BCLK = 32·fs), which CardputerAudio sends.
    {0x02, 0x18},
    {0x0D, 0x01},  // SYSTEM: power up analog circuitry
    {0x12, 0x00},  // SYSTEM: power up the DAC
    {0x13, 0x10},  // SYSTEM: enable the output driver feeding the speaker amp
    {0x32, 0xBF},  // DAC volume: 0 dB; loudness is set in software
    {0x37, 0x08},  // DAC: bypass the equaliser
};

bool es8311BeginSpeaker() {
    if (!cardputer_i2c::begin()) {
        Serial.println("[es8311] I2C bus failed to start");
        return false;
    }
    for (const RegisterWrite& write : SPEAKER_ENABLE_SEQUENCE) {
        if (!cardputer_i2c::writeRegister(ES8311_ADDRESS, write.reg, write.value)) {
            Serial.printf("[es8311] write to register 0x%02X failed\n", write.reg);
            return false;
        }
    }
    return true;
}
