#include "m5stickc_audio.h"
#include <Arduino.h>

bool M5StickCAudio::begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    return true;
}

void M5StickCAudio::update() {
    const uint16_t frequencyHz = _sequencer.frequencyAt(millis());
    // tone() restarts the PWM timer, so only touch it when the pitch changes.
    if (frequencyHz == _soundingHz) return;
    if (frequencyHz == 0) {
        noTone(BUZZER_PIN);
    } else {
        tone(BUZZER_PIN, frequencyHz);
    }
    _soundingHz = frequencyHz;
}

void M5StickCAudio::playRadarBeep() {
    _sequencer.playRadarBeep(millis());
}

void M5StickCAudio::playMenuTheme() {
    _sequencer.playMelody(melodies::MENU_THEME, millis());
}

void M5StickCAudio::playCollectionFanfare() {
    _sequencer.playMelody(melodies::COLLECTION_FANFARE, millis());
}

void M5StickCAudio::playWinFanfare() {
    _sequencer.playMelody(melodies::WIN_FANFARE, millis());
}

void M5StickCAudio::stopMelody() {
    _sequencer.stop();
}

bool M5StickCAudio::isMelodyPlaying() {
    return _sequencer.isMelodyPlaying();
}
