#pragma once

#include "Arduino.h"

#include "data.h"
#include "debug.h"
#include "hardware.h"
#include "settings.h"
#include "wifi_control.h"

int current_letter_index = 0;


[[noreturn]] void ui_loop(void *) {
    for (;;) {
        while (xSemaphoreTake(wifi_connection_mutex, mutex_wait_time) != pdTRUE) {
#ifdef DEBUG
            Serial.println("Can't take (ui_loop)");
#endif
        }
        const auto &config = settings.get();
        matrix.setIntensity(config.screen_brightness);
        matrix.setRotation(config.screen_rotation);
        matrix.fillScreen(LOW);

        const auto &display_string = sensor_data.display_string;

        if (display_string.length() == 0 || current_letter_index > width * display_string.length() - spacer) {
            current_letter_index = 0;

            xSemaphoreGive(wifi_connection_mutex);

            delay(config.text_loop_delay);
            continue;
        }

        int letter = current_letter_index / width;
        int x = (matrix.width() - 1) - current_letter_index % width;
        int y = (matrix.height() - height) / 2;

        while (x + width - spacer >= 0 && letter >= 0) {
            matrix.drawChar((int16_t) x, (int16_t) y, display_string[letter], HIGH, LOW, 1);

            letter--;
            x -= width;
        }

        ++current_letter_index;

        matrix.write();
        delay(config.text_animation_delay);

        xSemaphoreGive(wifi_connection_mutex);
    }
}