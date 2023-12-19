#pragma once

#include "hardware.h"
#include "models.h"
#include "settings.h"
#include "window.h"

#define FAN_WINDOW_CHUNK_SIZE 60

static unsigned long fan_pwm_freq = 0;

static bool fan_window_on = false;

static unsigned long fan_last_update = 0;
static bool fan_window_can_be_active = true;
static Window fan_window(0, FAN_WINDOW_CHUNK_SIZE);

float map_value(float value, float src_from, float src_to, float dst_from, float dst_to) {
    const float src_range = src_to - src_from;
    const float dst_range = dst_to - dst_from;

    const float k = dst_range / src_range;

    return max(dst_from, min(dst_to, dst_from + (value - src_from) * k));
}

void update_fan_speed(SensorData &sensor_data) {
#ifndef PIN_FAN_PWM
    return;
#endif

    const auto &config = settings.get();

    if (fan_pwm_freq != config.fan_pwm_frequency) {
        ledcSetup(PWM_CHANNEL_FAN, config.fan_pwm_frequency, FAN_PWM_BITS);
        ledcAttachPin(PIN_FAN_PWM, PWM_CHANNEL_FAN);

#ifdef DEBUG
        Serial.print("Reconfigure Fan PWM  frequency: ");
        Serial.print(fan_pwm_freq);
        Serial.print(" >> ");
        Serial.println(config.fan_pwm_frequency);
#endif

        fan_pwm_freq = config.fan_pwm_frequency;
    }

    fan_window.resize((long) config.fan_active_time_window);
    if (fan_window_can_be_active && fan_window.accumulated_time() >= config.fan_max_active_time) {
        fan_window_can_be_active = false;
    } else if (!fan_window_can_be_active && fan_window.accumulated_time() == 0) {
        fan_window_can_be_active = true;
    }

#ifdef DEBUG
    Serial.print("Fan update, active time: ");
    Serial.print(fan_window.accumulated_time());
    Serial.print(" / ");
    Serial.print(fan_window.window_size());
    Serial.print(" (");
    Serial.print(fan_window_can_be_active ? "can be active" : "out of time");
    Serial.println(")");

    fan_window.print_debug();
#endif

    float fan_duty = NAN;
    float value = sensor_data.get_sensor_value(config.fan_sensor);
    switch (config.fan_mode) {
        case PWM:
            if (fan_window_can_be_active && value >= config.fan_min_sensor_value) {
                fan_duty = map_value(value, config.fan_min_sensor_value, config.fan_max_sensor_value,
                                     config.fan_min_duty, config.fan_max_duty);
            } else {
                fan_duty = 0.0f;
            }
            break;

        case WINDOW:
            if (!fan_window_on && fan_window_can_be_active && value > config.fan_max_sensor_value) {
                fan_window_on = true;
            } else if (fan_window_on && (!fan_window_can_be_active || value < config.fan_min_sensor_value)) {
                fan_window_on = false;
            }

            fan_duty = fan_window_on ? config.fan_max_duty : config.fan_min_duty;
            break;

        case SCHEDULE:
            fan_duty = fan_window_can_be_active ? config.fan_max_duty : config.fan_min_duty;
            break;

        case ON:
            fan_duty = config.fan_max_duty;
            break;

        case OFF:
        default:
            fan_duty = 0.0f;
            break;
    }


    if (isnan(fan_duty)) fan_duty = 0.0f;
    ledcWrite(PWM_CHANNEL_FAN, (uint32_t) (FAN_PWM_RESOLUTION * fan_duty));
    sensor_data.fan_speed = fan_duty * 100;

    const auto current_time_sec = millis() / 1000ul;
    fan_window.update(fan_duty > 0 ? (window_t) (current_time_sec - fan_last_update) : 0);
    fan_last_update = current_time_sec;
}