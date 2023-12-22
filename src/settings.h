#pragma once

#include <EEPROM.h>

#include "debug.h"
#include "models.h"
#include "timer.h"

#define SETTINGS_HEADER (int) 0xffaabbcc
#define SETTINGS_VERSION (int) 8

class WebServer;

struct SettingsEntry {
    int header = SETTINGS_HEADER;
    int version = SETTINGS_VERSION;

    float temperature_calibration = 0.0f;
    float humidity_calibration = 0.0f;
    float co2_calibration = 0.0f;

    unsigned int text_animation_delay = 80;
    unsigned int text_loop_delay = 3000;
    unsigned int wifi_max_connect_attempts = 600;

    unsigned long sensor_update_interval = (unsigned long) 5 * 1000;
    unsigned long sensor_send_interval = (unsigned long) 15 * 1000;

    unsigned long settings_save_interval = 15000;

    uint8_t screen_rotation = 3;
    uint8_t screen_brightness = 5;

    boolean sound_indication = true;

    AlertEntry alert_temperature = {true, (unsigned long) 5 * 60 * 1000, 22, 24};
    AlertEntry alert_co2 = {true, (unsigned long) 5 * 60 * 1000, 400, 1500};
    AlertEntry alert_humidity = {true, (unsigned long) 5 * 60 * 1000, 80, 100};
    AlertEntry alert_latency = {true, (unsigned long) 5 * 60 * 1000, 0, 60000};

    ScheduleEntry fan_schedule = {ScheduleMode::PWM, SensorType::CO2, 500, 1000, 480, 3600, 26000, 0, 1};
    ScheduleEntry humidifier_schedule = {ScheduleMode::PWM, SensorType::HUMIDITY, 100, 80, 480, 3600, 26000, 0, 1};
};

typedef void (*update_fn)(SettingsEntry &data);

class Settings {
    static const int _offset = 0;
    volatile static boolean _initialized;

    SettingsEntry _data;
    Timer &_timer;

    long _save_timer_id = -1;

public:
    Settings(Timer& timer);

    inline Timer &timer() { return _timer; }

    void begin();

    inline const SettingsEntry &get() const { return _data; }

    String json() const;

    boolean update_settings(WebServer &server);

    void update_settings(update_fn fn);

    void reset();

    inline bool is_pending_commit() const { return _save_timer_id != -1; }

    void force_save();

private:
    void _commit();
};

static Settings settings(shared_timer);