#pragma once

#include <ArduinoJson.h>
#include "HTTPClient.h"

#include "alert.h"
#include "credentials.h"
#include "debug.h"
#include "hardware.h"
#include "models.h"
#include "schedule.h"
#include "settings.h"
#include "wifi_control.h"

const unsigned int connection_timeout = 1000;
const unsigned int tcp_timeout = 1000;

enum State : uint8_t {
    WARM_UP,
    DISPLAY_SENSOR,
    PENDING_ALERT,
    DISPLAY_ALERT,
};

volatile static State current_state = WARM_UP;

static SensorData sensor_data;
static String alert_display_string = "";

static const Alert Alerts[] = {
        {ALERT_TEMPERATURE, settings.get().alert_temperature, sensor_data.temperature,  "TEMP",    "C",   1},
        {ALERT_CO2,         settings.get().alert_co2,         sensor_data.co2,          "CO2",     "ppm", 0},
        {ALERT_HUMIDITY,    settings.get().alert_humidity,    sensor_data.humidity,     "HUM",     "%",   0},
        {ALERT_SENDING,     settings.get().alert_latency,     sensor_data.send_latency, "LATENCY", "s",   0},
};

static Schedule Schedules[] = {
#ifdef PIN_FAN_PWM
        {PIN_FAN_PWM, PWM_CHANNEL_FAN, FAN_PWM_BITS,
         (float &) sensor_data.fan_speed, settings.get().fan_schedule},
#endif
#ifdef PIN_HUMIDIFIER_PWM
        {PIN_HUMIDIFIER_PWM, PWM_CHANNEL_HUMIDIFIER, HUMIDIFIER_PWM_BITS,
         (float &) sensor_data.humidifier_power, settings.get().humidifier_schedule},
#endif
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
        }
    }
}

void send_sensor_data() {
    const auto &config = settings.get();
    if (sensor_data.last_send == 0ul || (millis() - sensor_data.last_send) > config.sensor_send_interval) {
#ifdef DEBUG
        Serial.println("Sending sensor data...");
#endif
        String result;
        StaticJsonDocument<128> doc;
        if (!isnan(sensor_data.temperature)) doc["Tamb"] = sensor_data.temperature;
        if (!isnan(sensor_data.co2)) doc["CntR"] = sensor_data.co2;
        if (!isnan(sensor_data.humidity)) doc["Hum"] = sensor_data.humidity;
        if (!isnan(sensor_data.fan_speed)) doc["Fan"] = sensor_data.fan_speed;
        if (!isnan(sensor_data.humidifier_power)) doc["HumR"] = sensor_data.humidifier_power;

        serializeJson(doc, result);

        http.setConnectTimeout(connection_timeout);
        http.setTimeout(tcp_timeout);

        http.begin(client, API_URL);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("API-Key", API_KEY);

        const auto httpResponseCode = http.POST(result);
        if (httpResponseCode == 200) {
            const auto now = millis();
            sensor_data.send_latency = (float) (now - sensor_data.last_send) - (float) config.sensor_send_interval;
            if (sensor_data.send_latency < 0ul) sensor_data.send_latency = NAN;

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
    if (sensor_data.last_update == 0ul || (millis() - sensor_data.last_update) > config.sensor_update_interval) {
        sensor_data.humidity = bme.readHumidity() + config.humidity_calibration;
        sensor_data.temperature = bme.readTemperature() + config.temperature_calibration;

        auto co2 = Mhz19.getCO2(false);
        if (co2 >= 400 && co2 <= 5000) {
            sensor_data.co2 = (float) co2 + config.co2_calibration;
        }

        for (auto &schedule: Schedules) {
            schedule.update(sensor_data);
        }

        sensor_data.last_update = millis();

#ifdef DEBUG
        Serial.print("Sensor Data: ");
        Serial.print(sensor_data.temperature);
        Serial.print(" С ");
        Serial.print(sensor_data.co2);
        Serial.print(" ppm ");
        Serial.print(sensor_data.humidity);
        Serial.println("%");

#ifdef PIN_FAN_PWM
        Serial.print("Fan PWM duty: ");
        Serial.print(sensor_data.fan_speed);
        Serial.println("%");
#endif

#ifdef PIN_HUMIDIFIER_PWM
        Serial.print("Humidifier PWM duty: ");
        Serial.print(sensor_data.humidifier_power);
        Serial.println("%");
#endif
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
        settings.timer().handle_timers();

        delay(100);
    }
}

String get_current_display_string() {
    switch (current_state) {
        case WARM_UP:
            return "Warming up...";

        case DISPLAY_ALERT:
            return alert_display_string;

        case DISPLAY_SENSOR:
        case PENDING_ALERT:
        default:
            return sensor_data.display_string;
    }
}

void next_step() {
    State next;
    switch (current_state) {
        case WARM_UP:
            next = sensor_data.ready() ? DISPLAY_SENSOR : WARM_UP;
            break;

        case PENDING_ALERT:
            next = DISPLAY_ALERT;
            break;

        case DISPLAY_SENSOR:
        case DISPLAY_ALERT:
        default:
            next = DISPLAY_SENSOR;
            break;
    }

    sensor_data.update_string();
    current_state = next;
}