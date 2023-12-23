#pragma once

#include "debug.h"
#include "models.h"
#include "window.h"

float map_value(float value, float src_from, float src_to, float dst_from, float dst_to) {
    const bool reverse = src_from > src_to;
    if (reverse) std::swap(src_from, src_to);

    const float src_range = src_to - src_from;
    const float dst_range = dst_to - dst_from;

    const float k = dst_range / src_range;
    const auto result = max(dst_from, min(dst_to, dst_from + (value - src_from) * k));

    if (!reverse)
        return result;

    return dst_to - result;
}

class Schedule {
    unsigned long _pwm_freq = 0;
    bool _window_on = false;

    unsigned long _last_update = 0;
    unsigned long _window_next_active_time = 0;
    bool _window_can_be_active = false;
    Window _window;

    const ScheduleEntry &_config;
    float &_dst_member;

    uint8_t _pin;
    uint8_t _channel;
    uint8_t _bits;
    uint32_t _resolution;

public:
    Schedule(uint8_t pin, uint8_t channel, uint8_t bits, float &dst_member,
             const ScheduleEntry &config, long chunk_size = 60l)
            : _pin(pin), _channel(channel), _bits(bits), _resolution((1ul << _bits) - 1),
              _dst_member(dst_member), _config(config), _window(0l, chunk_size) {}

    float update(SensorData &sensor_data) {
        if (_pwm_freq != _config.pwm_frequency) {
            ledcSetup(_channel, _config.pwm_frequency, _bits);
            ledcAttachPin(_pin, _channel);

#ifdef DEBUG
            Serial.print("Reconfigure pin ");
            Serial.print(_pin);
            Serial.print(" PWM frequency: ");
            Serial.print(_pwm_freq);
            Serial.print(" >> ");
            Serial.print(_config.pwm_frequency);
            Serial.print(" (bits: ");
            Serial.print(_bits);
            Serial.print(", resolution: ");
            Serial.print(_resolution);
            Serial.println(")");
#endif

            _pwm_freq = _config.pwm_frequency;
        }

        _window.resize((long) _config.active_time_window);
        if (_window_can_be_active && _window.accumulated_time() >= _config.max_active_time) {
            _window_can_be_active = false;
            _window_next_active_time = 0;
        } else if (!_window_can_be_active && _window.accumulated_time() == 0) {
            _window_can_be_active = true;
            _window_next_active_time = millis() + _config.activation_offset * 1000;
        }

#ifdef DEBUG
        Serial.print("Pin ");
        Serial.print(_pin);
        Serial.print(" update, active time: ");
        Serial.print(_window.accumulated_time());
        Serial.print(" / ");
        Serial.print(_window.window_size());
        Serial.print(" (");
        Serial.print(_window_can_be_active ? "can be active" : "out of time");
        Serial.println(")");

        _window.print_debug();
#endif

        float duty = NAN;
        float value = sensor_data.get_sensor_value(_config.sensor);
        switch (_config.mode) {
            case PWM:
                if (_can_be_active()) {
                    duty = map_value(value, _config.min_sensor_value, _config.max_sensor_value,
                                     _config.min_duty, _config.max_duty);
                } else {
                    duty = 0.0f;
                }
                break;

            case WINDOW: {
                const bool active = map_value(value, _config.min_sensor_value,
                                              _config.max_sensor_value, 0, 1) == 1.0;
                if (!_window_on && _can_be_active() && active) {
                    _window_on = true;
                } else if (_window_on && (!_can_be_active() || !active)) {
                    _window_on = false;
                }

                duty = _window_on ? _config.max_duty : _config.min_duty;
                break;
            }

            case SCHEDULE:
                duty = _can_be_active() ? _config.max_duty : _config.min_duty;
                break;

            case ON:
                duty = _config.max_duty;
                break;

            case OFF:
            default:
                duty = 0.0f;
                break;
        }


        if (isnan(duty)) duty = 0.0f;
        ledcWrite(_channel, (uint32_t) ((float) _resolution * duty));

        const auto current_time_sec = millis() / 1000ul;
        _window.update(duty > 0 ? (window_t) (current_time_sec - _last_update) : 0);
        _last_update = current_time_sec;

        _dst_member = duty * 100;
        return duty;
    }

private:
    inline bool _can_be_active() const {
        return _window_can_be_active && millis() >= _window_next_active_time;
    }
};