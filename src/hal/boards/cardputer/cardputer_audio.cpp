#include "cardputer_audio.h"
#include "es8311.h"

bool CardputerAudio::begin() {
    // I2S starts first so the codec, which derives its clock from BCLK, has
    // a clock as soon as it is powered up.
    _i2s.setPins(I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DATA_PIN);
    if (!_i2s.begin(I2S_MODE_STD, SAMPLE_RATE_HZ, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
        Serial.println("[audio] I2S start FAILED - speaker will stay silent");
        return false;
    }
    if (!es8311BeginSpeaker()) {
        Serial.println("[audio] ES8311 codec init FAILED - speaker will stay silent");
        // Stop I2S too, or its DMA clocks out silence for nothing for the rest of the session.
        _i2s.end();
        return false;
    }
    if (xTaskCreate(synthTask, "tone_synth", SYNTH_TASK_STACK_BYTES, this, SYNTH_TASK_PRIORITY, nullptr) != pdPASS) {
        Serial.println("[audio] synth task creation FAILED - speaker will stay silent");
        _i2s.end();
        return false;
    }
    return true;
}

void CardputerAudio::synthTask(void* self) {
    CardputerAudio& audio = *static_cast<CardputerAudio*>(self);
    const size_t bytesPerWrite = sizeof(audio._frames);
    while (true) {
        audio._wave.setFrequency(audio._targetHz.load());
        audio._wave.fill(audio._frames, FRAMES_PER_WRITE);
        // Blocks until the DMA has room, which paces this loop to the sample rate.
        const size_t written = audio._i2s.write(reinterpret_cast<const uint8_t*>(audio._frames), bytesPerWrite);
        if (written != bytesPerWrite) {
            Serial.printf("[audio] I2S write short: %u of %u bytes\n",
                          static_cast<unsigned>(written), static_cast<unsigned>(bytesPerWrite));
        }
    }
}

void CardputerAudio::update() {
    _targetHz.store(_sequencer.frequencyAt(millis()));
}

void CardputerAudio::playRadarBeep() {
    _sequencer.playRadarBeep(millis());
}

void CardputerAudio::playMenuTheme() {
    _sequencer.playMelody(melodies::MENU_THEME, millis());
}

void CardputerAudio::playCollectionFanfare() {
    _sequencer.playMelody(melodies::COLLECTION_FANFARE, millis());
}

void CardputerAudio::playWinFanfare() {
    _sequencer.playMelody(melodies::WIN_FANFARE, millis());
}

void CardputerAudio::stopMelody() {
    _sequencer.stop();
}

bool CardputerAudio::isMelodyPlaying() {
    return _sequencer.isMelodyPlaying();
}
