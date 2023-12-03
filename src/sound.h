#pragma once

#include <Arduino.h>
#include "hardware.h"
#include "settings.h"

const unsigned int SOUND_WIFI_ON[] = {
        880, 120,
        1046, 240
};

const unsigned int SOUND_WIFI_FAIL[] = {
        1046, 120,
        880, 120,
        587, 240
};

const unsigned int SOUND_ALERT[] = {
        880, 120,
        698, 120
};

template<unsigned long SIZE>
void play_sound(const unsigned int (&notes)[SIZE]) {
    if (!settings.get().sound_indication) return;

    for (unsigned long i = 0; i < SIZE; i += 2) {
        tone(PIN_SPEAKER, notes[i]);
        delay(notes[i + 1]);
        noTone(PIN_SPEAKER);
    }
}