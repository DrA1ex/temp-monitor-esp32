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
    volatile float send_latency = 0;
    volatile unsigned long last_send = 0;

    String display_string = "";

    String json() const {
        StaticJsonDocument<128> doc;
        doc["temp"] = temperature;
        doc["hum"] = humidity;
        doc["lat"] = float(millis() - last_send) / 1000;

        String result;
        serializeJson(doc, result);
        return result;
    }
};

enum State : uint8_t {
    DISPLAY_SENSOR,
    PENDING_ALERT,
    DISPLAY_ALERT,
};


static unsigned long last_sensor_update = 0ul;
static unsigned long last_sensor_send_try = 0ul;

volatile static State current_state = DISPLAY_SENSOR;

static SensorData sensor_data;
static String alert_display_string = "";

static const Alert Alerts[] = {
        {ALERT_HUMIDITY,    settings.get().alert_humidity,    sensor_data.humidity,     "HUM",     "%", 0},
        {ALERT_TEMPERATURE, settings.get().alert_temperature, sensor_data.temperature,  "TEMP",    "C", 1},
        {ALERT_SENDING,     settings.get().alert_latency,     sensor_data.send_latency, "LATENCY", "s", 0},
};

static HTTPClient http;
static WiFiClientSecure client;

[[noreturn]] void data_loop(void *);

void process_alerts() {
    if (current_state != DISPLAY_SENSOR) return;

    for (auto config: Alerts) {
        const boolean activated = alert(config.key, config.value, config.entry_prop);
        if (activated) {
            current_state = PENDING_ALERT;
            alert_display_string = String("ALERT ") + config.name + ": "
                                   + String(config.value, config.fraction) + " "
                                   + String(config.unit);

            return;
        };
    }
}

void send_sensor_data() {
    const auto &config = settings.get();
    if (last_sensor_send_try == 0ul || (millis() - last_sensor_send_try) > config.sensor_send_interval) {
#ifdef DEBUG
        Serial.println("Sending sensor data...");
#endif
        last_sensor_send_try = millis();

        String result;
        StaticJsonDocument<64> doc;
        if (!isnan(sensor_data.temperature)) doc["Tamb"] = sensor_data.temperature;
        if (!isnan(sensor_data.humidity)) doc["Hum"] = sensor_data.humidity;
        serializeJson(doc, result);

        http.setConnectTimeout(connection_timeout);
        http.setTimeout(tcp_timeout);

        http.begin(client, API_URL);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("API-Key", API_KEY);

        const auto httpResponseCode = http.POST(result);
        if (httpResponseCode == 200) {
            const auto now = millis();
            sensor_data.send_latency = float(now - sensor_data.last_send) / 1000.0f;
            sensor_data.last_send = now;
        }

        http.end();

#ifdef DEBUG
        if (httpResponseCode == 200) {
            Serial.println("Sensor data sent");
        } else {
            Serial.print("Data API Error: ");
            Serial.println(HTTPClient::errorToString(httpResponseCode));
        }
#endif
    }
}

void update_sensor_data() {
    const auto &config = settings.get();
    if (last_sensor_update == 0ul || (millis() - last_sensor_update) > config.sensor_update_interval) {
        sensor_data.humidity = dht.readHumidity() + config.humidity_calibration;
        sensor_data.temperature = dht.readTemperature() + config.temperature_calibration;

        sensor_data.display_string = String(sensor_data.temperature, 1) + " C" + "  "
                                     + String(sensor_data.humidity, 0) + " %";

        last_sensor_update = millis();

#ifdef DEBUG
        Serial.print("Sensor Data: ");
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

String &get_current_display_string() {
    switch (current_state) {
        case DISPLAY_ALERT:
            return alert_display_string;

        case DISPLAY_SENSOR:
        case PENDING_ALERT:
        default:
            return sensor_data.display_string;
    }
}

void next_status() {
    State next;
    switch (current_state) {
        case PENDING_ALERT:
            next = DISPLAY_ALERT;
            break;

        case DISPLAY_SENSOR:
        case DISPLAY_ALERT:
        default:
            next = DISPLAY_SENSOR;
            break;
    }

    current_state = next;
}