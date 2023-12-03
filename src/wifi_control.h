#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "debug.h"
#include "sound.h"
#include "hardware.h"

const TickType_t mutex_wait_time = portMAX_DELAY;
static SemaphoreHandle_t wifi_connection_mutex = xSemaphoreCreateMutex();

bool is_connected() {
    return WiFiClass::status() == WL_CONNECTED;
}

void wifi_connect() {
    while (xSemaphoreTake(wifi_connection_mutex, mutex_wait_time) != pdTRUE) {
#ifdef DEBUG
        Serial.println("Can't take (wifi_connect)");
#endif
    }

    matrix.fillScreen(LOW);
    matrix.write();
    matrix.drawChar(0, 0, 'W', HIGH, LOW, 1);
    matrix.write();

    WiFi.disconnect(true);

    WiFiClass::mode(WIFI_STA);
    WiFi.setSleep(WIFI_PS_NONE);
    WiFi.begin(ssid, password);

    unsigned int attempt = 0;
    while (WiFiClass::status() != WL_CONNECTED && attempt < settings.get().wifi_max_connect_attempts) {
        matrix.fillScreen(LOW);
        matrix.drawChar((int16_t) (attempt % width - spacer), 0, '.', HIGH, LOW, 1);
        matrix.write();

        delay(100);
        ++attempt;
    }

    if (WiFiClass::status() != WL_CONNECTED) {
        matrix.fillScreen(LOW);
        matrix.drawChar(0, 0, 'F', HIGH, LOW, 1);
        matrix.write();

        play_sound(SOUND_WIFI_FAIL);
        xSemaphoreGive(wifi_connection_mutex);

        ESP.restart();
        return;
    }

    matrix.fillScreen(LOW);
    matrix.drawChar(0, 0, 'K', HIGH, LOW, 1);
    matrix.write();

    xSemaphoreGive(wifi_connection_mutex);

#ifdef DEBUG
    Serial.print("Connected to WiFi with ");
    Serial.print(attempt);
    Serial.println(" attempts");
#endif
}