#pragma once

#include <EEPROM.h>

#include "debug.h"

#define SETTINGS_HEADER (int) 0xffaabbcc
#define SETTINGS_VERSION (int) 2

class WebServer;

struct AlertEntry {
    boolean enabled;
    unsigned long alert_interval;
    float min;
    float max;
};

struct SettingsEntry {
    int header = SETTINGS_HEADER;
    int version = SETTINGS_VERSION;

    float temperature_calibration = -0.4f;
    float humidity_calibration = 0.0f;

    unsigned int text_animation_delay = 80;
    unsigned int text_loop_delay = 3000;
    unsigned int wifi_max_connect_attempts = 600;

    unsigned long sensor_update_interval = (unsigned long) 5 * 1000;
    unsigned long sensor_send_interval = (unsigned long) 15 * 1000;

    uint8_t screen_rotation = 1;
    uint8_t screen_brightness = 3;

    boolean sound_indication = true;

    AlertEntry alert_temperature = {true, (unsigned long) 5 * 60 * 1000, 28, 30};
    AlertEntry alert_humidity = {false, (unsigned long) 5 * 60 * 1000, 80, 100};
    AlertEntry alert_latency = {true, (unsigned long) 5 * 60 * 1000, 0, 300};
};

typedef void (*update_fn)(SettingsEntry &data);

class Settings {
    static const int _offset = 0;
    volatile static boolean _initialized;

    SettingsEntry _data;

public:
    void begin();

    inline const SettingsEntry &get() const { return _data; }

    String json() const;

    boolean update_settings(WebServer &server);

    void update_settings(update_fn fn);

    void reset();

private:
    void commit();
};

static Settings settings;