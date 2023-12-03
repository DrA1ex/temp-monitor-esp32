
#include <Arduino.h>
#include <esp_task_wdt.h>

#include "debug.h"
#include "data.h"
#include "display.h"
#include "hardware.h"
#include "settings.h"
#include "sound.h"
#include "web_ui.h"
#include "wifi_control.h"

#define WDT_TIMEOUT 60

TaskHandle_t UiTask;
TaskHandle_t DataUpdateTask;
TaskHandle_t WebTask;

void setup() {
#ifdef DEBUG
    Serial.begin(9600);
    Serial.println("Initializing");
#endif

    settings.begin();

    const auto &config = settings.get();
    matrix.setRotation(config.screen_rotation);
    matrix.setIntensity(config.screen_brightness);

    client.setCACert(SSL_CERT);

    wifi_connect();
    play_sound(SOUND_WIFI_ON);

    dht.begin();

    xTaskCreatePinnedToCore(ui_loop, "UI", 10240, nullptr, 1, &UiTask, 0);
    xTaskCreatePinnedToCore(data_loop, "Data", 10240, nullptr, 1, &DataUpdateTask, 1);
    xTaskCreatePinnedToCore(web_loop, "Web", 10240, nullptr, 1, &WebTask, 1);

    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(DataUpdateTask);
}

__attribute__((unused)) void loop() {}