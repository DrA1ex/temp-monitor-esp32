#pragma once

#include "Arduino.h"

#include "data.h"
#include "debug.h"
#include "hardware.h"
#include "sound.h"
#include "settings.h"
#include "wifi_control.h"

static uint16_t current_letter_index = 0;

[[noreturn]] void ui_loop(void *) {
    for (;;) {
        while (xSemaphoreTake(wifi_connection_mutex, mutex_wait_time) != pdTRUE) {
#ifdef DEBUG
            Serial.println("Can't take mutex (ui_loop)");
#endif
        }

        const auto &config = settings.get();
        matrix.setIntensity(config.screen_brightness);
        matrix.setRotation(config.screen_rotation);
        matrix.fillScreen(LOW);

        const auto &display_string = get_current_display_string();
        const uint16_t max_index = width * display_string.length() + end_spacer - spacer;
        if (display_string.length() == 0 || current_letter_index > max_index) {
            current_letter_index = 0;
            next_step();

            xSemaphoreGive(wifi_connection_mutex);
            delay(config.text_loop_delay);
            continue;
        }

        if (current_state == State::WARM_UP && sensor_data.ready()) {
            current_letter_index = 0;
            next_step();

            xSemaphoreGive(wifi_connection_mutex);
            continue;
        }

        if (current_letter_index == 0 && current_state == State::DISPLAY_ALERT) {
            play_sound(SOUND_ALERT);
        }

        int letter = current_letter_index / width;
        int x = (matrix.width() - 1) - current_letter_index % width;
        int y = (matrix.height() - height) / 2;

        while (x + width - spacer >= 0 && letter >= 0) {
            if (letter < display_string.length()) {
                matrix.drawChar((int16_t) x, (int16_t) y, display_string[letter], HIGH, LOW, 1);
            }

            letter--;
            x -= width;
        }

        ++current_letter_index;

        matrix.write();
        delay(config.text_animation_delay);

        xSemaphoreGive(wifi_connection_mutex);
    }
}