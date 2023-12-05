#pragma once

#include "debug.h"

#include <Arduino.h>
#include <esp_task_wdt.h>

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>

#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

#include "alert.h"
#include "credentials.h"
#include "pins.h"
#include "settings.h"
#include "sound.h"

#define WDT_TIMEOUT 60

struct SensorData {
    volatile float humidity = NAN;
    volatile float temperature = NAN;
};

struct Alert {
    const int key;
    const AlertEntry &entry_prop;
    const volatile float &value;
};

[[noreturn]] void data_loop(void *parameter);

[[noreturn]] void ui_loop(void *parameter);

[[noreturn]] void web_loop(void *parameter);

void wifi_connect();

void update_sensor_data();

void send_sensor_data();

void process_alerts();
