#pragma once

#include "ArduinoJson.h"

enum FanMode : uint8_t {
    PWM = 0,
    WINDOW = 1,
    SCHEDULE = 2,
    ON = 3,
    OFF = 4,
};

enum SensorType : uint8_t {
    TEMPERATURE = 0,
    HUMIDITY = 1,
    CO2 = 2
};

struct SensorData {
    volatile float humidity = NAN;
    volatile float temperature = NAN;
    volatile float co2 = NAN;
    volatile float send_latency = NAN;
    volatile unsigned long last_update = 0;
    volatile unsigned long last_send = 0;
    volatile float fan_speed = 0;

    String display_string = "";

    bool ready() {
        return !isnan(humidity) || !isnan(temperature) || !isnan(co2);
    }

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

    float get_sensor_value(SensorType type) {
        switch (type) {
            case SensorType::TEMPERATURE:
                return temperature;

            case SensorType::HUMIDITY:
                return humidity;

            case SensorType::CO2:
                return co2;

            default:
                return NAN;
        }
    }

    void update_string() {
        if (!ready()) {
            display_string = "NO DATA";
            return;
        }

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

struct AlertEntry {
    boolean enabled;
    unsigned long alert_interval;
    float min;
    float max;
};