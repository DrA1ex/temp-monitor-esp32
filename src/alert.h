#pragma once

#include <Arduino.h>

#include "settings.h"

#define ALERT_HUMIDITY 0u
#define ALERT_TEMPERATURE 1u
#define ALERT_SENDING 2u

struct Alert {
    const int key;
    const AlertEntry &entry_prop;
    const volatile float &value;

    const char *name;
    const char *unit;
};

static unsigned long alert_time[] = {0ul, 0ul, 0ul};

bool alert(const unsigned int key, const unsigned int alert_interval, float value, float min, float max) {
    const auto last_alert = alert_time[key];

    if ((value < min || value > max) && (last_alert == 0ul || millis() - last_alert > alert_interval)) {
        alert_time[key] = millis();
        return true;
    }

    return false;
}

bool alert(const unsigned int key, float value, const AlertEntry &entry) {
    if (entry.enabled) return alert(key, entry.alert_interval, value, entry.min, entry.max);
    return false;
}