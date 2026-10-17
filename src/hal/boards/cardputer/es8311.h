#ifndef ES8311_H
#define ES8311_H

/**
 * @brief Power up the Cardputer ADV's ES8311 codec for speaker playback.
 *
 * The ADV routes the speaker through this codec, which stays silent until its
 * DAC is configured over I2C; the original Cardputer drove its amplifier
 * straight from I2S and needed no such step. Call before starting I2S.
 *
 * @return false if the codec did not acknowledge a write.
 */
bool es8311BeginSpeaker();

#endif // ES8311_H
