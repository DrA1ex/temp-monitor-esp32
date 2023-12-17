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

static bool fan_window_on = false;

struct SensorData {
    volatile float humidity = NAN;
    volatile float temperature = NAN;
    volatile float co2 = NAN;
    volatile float send_latency = NAN;
    volatile unsigned long last_update = 0;
    volatile unsigned long last_send = 0;
    volatile float fan_speed = 0;

    String display_string = "";

    String json() const {
        StaticJsonDocument<256> doc;
        doc["temp"] = temperature;
        doc["hum"] = humidity;
        doc["co2"] = co2;
        doc["fan"] = fan_speed;
        doc["lat"] = send_latency;

        auto system = doc.createNestedObject("system");
        system["uptime"] = esp_timer_get_time() / 1000000ULL;
        system["wifi"] = WiFi.RSSI();

        String result;
        serializeJson(doc, result);
        return result;
    }

    void update_string() {
        String co2_formatted;
        if (!isnan(co2) && co2 >= 1000) {
            float k = co2 / 1000.0f;
            unsigned int fraction = 0;
            if (k - floor(k) > 0.06) fraction = 1;

            co2_formatted = String(k, fraction) + "k";
        } else {
            co2_formatted = String(co2, 0);
        }

        display_string = String(temperature, 1) + " C" + "  "
                         + co2_formatted + " ppm" + "  "
                         + String(humidity, 0) + " %";
    }
};

enum State : uint8_t {
    DISPLAY_SENSOR,
    PENDING_ALERT,
    DISPLAY_ALERT,
};

volatile static State current_state = DISPLAY_SENSOR;

static SensorData sensor_data;
static String alert_display_string = "";

static const Alert Alerts[] = {
        {ALERT_TEMPERATURE, settings.get().alert_temperature, sensor_data.temperature,  "TEMP",    "C",   1},
        {ALERT_CO2,         settings.get().alert_co2,         sensor_data.co2,          "CO2",     "ppm", 0},
        {ALERT_HUMIDITY,    settings.get().alert_humidity,    sensor_data.humidity,     "HUM",     "%",   0},
        {ALERT_SENDING,     settings.get().alert_latency,     sensor_data.send_latency, "LATENCY", "s",   0},
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
        StaticJsonDocument<64> doc;
        if (!isnan(sensor_data.temperature)) doc["Tamb"] = sensor_data.temperature;
        if (!isnan(sensor_data.co2)) doc["CntR"] = sensor_data.co2;
        if (!isnan(sensor_data.humidity)) doc["Hum"] = sensor_data.humidity;
        if (!isnan(sensor_data.fan_speed)) doc["Fan"] = sensor_data.fan_speed * 100;

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

float map_value(float value, float src_from, float src_to, float dst_from, float dst_to) {
    const float src_range = src_to - src_from;
    const float dst_range = dst_to - dst_from;

    const float k = dst_range / src_range;

    return max(dst_from, min(dst_to, dst_from + (value - src_from) * k));
}

float get_sensor_value(SensorType type) {
    switch (type) {
        case TEMPERATURE:
            return sensor_data.temperature;

        case HUMIDITY:
            return sensor_data.humidity;

        case CO2:
            return sensor_data.co2;

        default:
            return NAN;
    }
}

void update_fan_speed() {
#ifndef PIN_FAN_PWM
    return;
#endif

    const auto &config = settings.get();

    float fan_duty;
    float value = get_sensor_value(config.fan_sensor);
    switch (config.fan_mode) {
        case PWM:
            fan_duty = map_value(value, config.fan_min_sensor_value,
                                 config.fan_max_sensor_value, 0, 1);
            break;

        case WINDOW:
            if (fan_window_on && value < config.fan_min_sensor_value) fan_window_on = false;
            else if (!fan_window_on && value > config.fan_max_sensor_value) fan_window_on = true;

            fan_duty = fan_window_on ? 1 : 0;
            break;

        case ON:
            fan_duty = 1;
            break;

        case OFF:
        default:
            fan_duty = 0;
            break;
    }


    if (isnan(fan_duty) || fan_duty < config.fan_min_duty) fan_duty = 0.0f;

    ledcWrite(PWM_CHANNEL_FAN, (uint32_t) (255 * fan_duty));;
    sensor_data.fan_speed = fan_duty;
}

void update_sensor_data() {
    const auto &config = settings.get();
    if (sensor_data.last_update == 0ul || (millis() - sensor_data.last_update) > config.sensor_update_interval) {
        sensor_data.humidity = dht.readHumidity() + config.humidity_calibration;
        sensor_data.temperature = dht.readTemperature() + config.temperature_calibration;

        auto co2 = Mhz19.getCO2(false);
        if (co2 >= 400 && co2 <= 5000) {
            sensor_data.co2 = (float) Mhz19.getCO2(false) + config.co2_calibration;
        }

        update_fan_speed();


        sensor_data.last_update = millis();

#ifdef DEBUG
        Serial.print("Sensor Data: ");
        Serial.print(sensor_data.temperature);
        Serial.print("С ");
        Serial.print(sensor_data.humidity);
        Serial.println("% ");
        Serial.print(sensor_data.co2);
        Serial.println("ppm");

#ifdef PIN_FAN_PWM
        Serial.print("Fan PWM duty: ");
        Serial.print(sensor_data.fan_speed);
        Serial.print("%");
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

void next_step() {
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

    sensor_data.update_string();
    current_state = next;
}