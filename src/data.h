#pragma once

#include <ArduinoJson.h>
#include "HTTPClient.h"

#include "alert.h"
#include "credentials.h"
#include "debug.h"
#include "hardware.h"
#include "settings.h"
#include "wifi_control.h"

const unsigned int connection_timeout = 1000;
const unsigned int tcp_timeout = 1000;

struct SensorData {
    volatile float humidity = NAN;
    volatile float temperature = NAN;

    String display_string = "";
};

static unsigned long last_sensor_update = 0ul;
static unsigned long last_sensor_sent = 0ul;

static SensorData sensor_data;
static const Alert Alerts[] = {
        {ALERT_HUMIDITY,    settings.get().alert_humidity,    sensor_data.humidity},
        {ALERT_TEMPERATURE, settings.get().alert_temperature, sensor_data.temperature},
};

static HTTPClient http;
static WiFiClientSecure client;

void process_alerts() {
    const int count = sizeof(Alerts) / sizeof Alerts[0];
    for (int i = 0; i < count; ++i) {
        const auto &alert_value = Alerts[0];
        alert(alert_value.key, alert_value.value, alert_value.entry_prop);
    }
}

void send_sensor_data() {
    const auto &config = settings.get();
    if (last_sensor_sent == 0ul || (millis() - last_sensor_sent) > config.sensor_send_interval) {
        last_sensor_sent = millis();

        String result;
        StaticJsonDocument<64> doc;
        if (!isnan(sensor_data.temperature)) doc["Tamb"] = sensor_data.temperature;
        if (!isnan(sensor_data.temperature)) doc["Hum"] = sensor_data.humidity;
        serializeJson(doc, result);

        http.setConnectTimeout(connection_timeout);
        http.setTimeout(tcp_timeout);

        http.begin(client, API_URL);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("API-Key", API_KEY);

        const auto httpResponseCode = http.POST(result);
        http.end();

#ifdef DEBUG
        Serial.print("API Response: ");
        Serial.println(httpResponseCode);
#endif
    }
}

void update_sensor_data() {
    const auto &config = settings.get();
    if (last_sensor_update == 0ul || (millis() - last_sensor_update) > config.sensor_update_interval) {
        sensor_data.humidity = dht.readHumidity() + config.humidity_calibration;
        sensor_data.temperature = dht.readTemperature() + config.temperature_calibration;

        sensor_data.display_string = String(sensor_data.temperature, 1) + " C" + "   "
                                     + String(sensor_data.humidity, 0) + " %  ";

        last_sensor_update = millis();

#ifdef DEBUG
        Serial.print(sensor_data.temperature);
        Serial.print("С ");
        Serial.print(sensor_data.humidity);
        Serial.println("%");
#endif
    }
}

[[noreturn]] void data_loop(void *) {
    for (;;) {
        esp_task_wdt_reset();

        update_sensor_data();
        process_alerts();

        if (!is_connected()) {
#ifdef DEBUG
            Serial.println("WiFi lost connection");
#endif
            wifi_connect();
        }

        send_sensor_data();

        delay(1000);
    }
}