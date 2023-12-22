#pragma once

#include "ArduinoJson.h"

enum ScheduleMode : uint8_t {
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
    volatile float humidifier_power = 0;

    String display_string = "";

    bool ready() const {
        return !isnan(humidity) || !isnan(temperature) || !isnan(co2);
    }

    float get_sensor_value(SensorType type) const {
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

struct ScheduleEntry {
    ScheduleMode mode;
    SensorType sensor;

    float min_sensor_value;
    float max_sensor_value;

    unsigned long max_active_time;
    unsigned long active_time_window;

    unsigned long pwm_frequency;

    float min_duty;
    float max_duty;
};