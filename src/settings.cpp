#include <ArduinoJson.h>
#include <WebServer.h>

#include "settings.h"


const char *TEMP_CALIBRATION = "t_cal";
const char *HUMIDITY_CALIBRATION = "h_cal";
const char *TEXT_ANIMATION_DELAY = "t_anim_delay";
const char *TEXT_LOOP_DELAY = "t_loop_delay";
const char *WIFI_MAX_CONNECT_ATTEMPTS = "wifi_max_attempts";
const char *SENSOR_UPDATE_INTERVAL = "upd_interval";
const char *SENSOR_SEND_INTERVAL = "send_int";
const char *SCREEN_ROTATION = "s_rot";
const char *SCREEN_BRIGHTNESS = "s_brt";
const char *SOUND_INDICATION = "snd";
const char *ALERT_TEMPERATURE = "alert_temp";
const char *ALERT_HUMIDITY = "alert_hum";
const char *ALERT_SEND_LAT = "alert_lat";
const char *ALERT_ENABLED = "enabled";
const char *ALERT_INTERVAL = "int";
const char *ALERT_MIN = "min";
const char *ALERT_MAX = "max";

volatile boolean Settings::_initialized = false;

void Settings::begin() {
    if (!Settings::_initialized) {
        Settings::_initialized = true;
        auto success = EEPROM.begin(sizeof(SettingsEntry) + 8);

#ifdef DEBUG
        if (success) Serial.println("EEPROM initialized");
        else Serial.println("EEPROM initialization failed");
#endif
    } else {
#ifdef DEBUG
        Serial.println("EEPROM already initialized");
#endif
    }

    EEPROM.get(Settings::_offset, _data);
    if (_data.version != SETTINGS_VERSION || _data.header != SETTINGS_HEADER) {
#ifdef DEBUG
        Serial.print("Stored settings version: ");
        Serial.print(_data.version);
        Serial.print(" header:");
        Serial.println(_data.header, HEX);

        Serial.println("Create new settings...");
#endif

        _data = SettingsEntry();
    }
}

void Settings::update_settings(update_fn fn) {
    fn(_data);
    commit();
}

JsonObject write_alert(JsonObject obj, const AlertEntry &entry) {
    obj[ALERT_ENABLED] = entry.enabled;
    obj[ALERT_INTERVAL] = entry.alert_interval;
    obj[ALERT_MIN] = entry.min;
    obj[ALERT_MAX] = entry.max;

    return obj;
}

String Settings::json() const {
    StaticJsonDocument<1024> doc;

    doc[TEMP_CALIBRATION] = _data.temperature_calibration;
    doc[HUMIDITY_CALIBRATION] = _data.humidity_calibration;
    doc[TEXT_ANIMATION_DELAY] = _data.text_animation_delay;
    doc[TEXT_LOOP_DELAY] = _data.text_loop_delay;
    doc[WIFI_MAX_CONNECT_ATTEMPTS] = _data.wifi_max_connect_attempts;
    doc[SENSOR_UPDATE_INTERVAL] = _data.sensor_update_interval;
    doc[SENSOR_SEND_INTERVAL] = _data.sensor_send_interval;
    doc[SCREEN_ROTATION] = _data.screen_rotation;
    doc[SCREEN_BRIGHTNESS] = _data.screen_brightness;
    doc[SOUND_INDICATION] = _data.sound_indication;

    write_alert(doc.createNestedObject(ALERT_TEMPERATURE), _data.alert_temperature);
    write_alert(doc.createNestedObject(ALERT_HUMIDITY), _data.alert_humidity);
    write_alert(doc.createNestedObject(ALERT_SEND_LAT), _data.alert_latency);

    String result;
    serializeJson(doc, result);

    return result;
}

boolean updateFieldFromRequest(WebServer &server, const char *fieldName, float &target) {
    if (server.hasArg(fieldName)) {
        target = server.arg(fieldName).toFloat();
        return true;
    }

    return false;
}

boolean updateFieldFromRequest(WebServer &server, const char *fieldName, boolean &target) {
    if (server.hasArg(fieldName)) {
        target = server.arg(fieldName).toInt() == 1;
        return true;
    }

    return false;
}

template<typename T, typename = std::enable_if<std::is_integral<T>::value>>
boolean updateFieldFromRequest(WebServer &server, const char *fieldName, T &target) {
    if (server.hasArg(fieldName)) {
        target = server.arg(fieldName).toInt();
        return true;
    }

    return false;
}

boolean readAlert(WebServer &server, AlertEntry &entry) {
    boolean ret = false;

    ret = updateFieldFromRequest(server, ALERT_ENABLED, entry.enabled) || ret;
    ret = updateFieldFromRequest(server, ALERT_INTERVAL, entry.alert_interval) || ret;
    ret = updateFieldFromRequest(server, ALERT_MIN, entry.min) || ret;
    ret = updateFieldFromRequest(server, ALERT_MAX, entry.max) || ret;

    return ret;
}

bool Settings::update_settings(WebServer &server) {
    boolean ret = false;

    ret = updateFieldFromRequest(server, TEMP_CALIBRATION, _data.temperature_calibration) || ret;
    ret = updateFieldFromRequest(server, HUMIDITY_CALIBRATION, _data.humidity_calibration) || ret;
    ret = updateFieldFromRequest(server, TEXT_ANIMATION_DELAY, _data.text_animation_delay) || ret;
    ret = updateFieldFromRequest(server, TEXT_LOOP_DELAY, _data.text_loop_delay) || ret;
    ret = updateFieldFromRequest(server, WIFI_MAX_CONNECT_ATTEMPTS, _data.wifi_max_connect_attempts) || ret;
    ret = updateFieldFromRequest(server, SENSOR_UPDATE_INTERVAL, _data.sensor_update_interval) || ret;
    ret = updateFieldFromRequest(server, SENSOR_SEND_INTERVAL, _data.sensor_send_interval) || ret;
    ret = updateFieldFromRequest(server, SCREEN_ROTATION, _data.screen_rotation) || ret;
    ret = updateFieldFromRequest(server, SCREEN_BRIGHTNESS, _data.screen_brightness) || ret;
    ret = updateFieldFromRequest(server, SOUND_INDICATION, _data.sound_indication) || ret;

    if (server.hasArg(ALERT_TEMPERATURE)) {
        ret = readAlert(server, _data.alert_temperature) || ret;
    }

    if (server.hasArg(ALERT_HUMIDITY)) {
        ret = readAlert(server, _data.alert_humidity) || ret;
    }

    if (server.hasArg(ALERT_SEND_LAT)) {
        ret = readAlert(server, _data.alert_latency) || ret;
    }

    if (ret) commit();
    return ret;
}

void Settings::reset() {
    update_settings([](SettingsEntry &data) { data = SettingsEntry(); });
}


void Settings::commit() {
    EEPROM.put(Settings::_offset, _data);
    auto success = EEPROM.commit();

#ifdef DEBUG
    if (success) Serial.println("EEPROM committed");
    else Serial.println("EEPROM commit failed");
#endif
}