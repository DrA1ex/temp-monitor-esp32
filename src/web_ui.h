#pragma once

#include <WebServer.h>

#include "settings.h"

extern const char WEB_INDEX[] asm("_binary_html_index_html_start");

static WebServer server(80);

String status_json() {
    StaticJsonDocument<256> doc;
    doc["temp"] = sensor_data.temperature;
    doc["hum"] = sensor_data.humidity;
    doc["co2"] = sensor_data.co2;
    doc["fan"] = sensor_data.fan_speed;
    doc["humr"] = sensor_data.humidifier_power;
    doc["lat"] = sensor_data.send_latency;

    auto system = doc.createNestedObject("system");
    system["uptime"] = esp_timer_get_time() / 1000000ULL;
    system["wifi"] = WiFi.RSSI();
    system["config_p"] = settings.is_pending_commit();

    String result;
    serializeJson(doc, result);
    return result;
}

[[noreturn]] void web_loop(void *) {
    server.on("/", [] { server.send(200, "text/html", WEB_INDEX); });
    server.on("/settings", HTTPMethod::HTTP_GET, [] {
        server.send(200, "application/json", settings.json());
    });
    server.on("/status", HTTPMethod::HTTP_GET, [] {
        server.send(200, "application/json", status_json());
    });
    server.on("/settings", HTTPMethod::HTTP_POST, [] {
        if (settings.update_settings(server)) {
            server.send(200, "plain/text", "OK");
        } else {
            server.send(400, "plain/text", "Bad Request");
        }
    });
    server.on("/reset", HTTPMethod::HTTP_POST, [] {
        settings.reset();
        server.send(200, "plain/text", "OK");
    });
    server.on("/co2/calibrate", HTTPMethod::HTTP_POST, [] {
        Mhz19.calibrate();
        server.send(200, "plain/text", "OK");
    });

    server.on("/restart", HTTPMethod::HTTP_POST, [] {
        if (settings.is_pending_commit()) {
            settings.force_save();
        }

#ifdef DEBUG
        Serial.println("Restart by use request");
#endif

        server.send(200);
        ESP.restart();
    });

    server.begin();

    for (;;) {
        server.handleClient();
        delay(10);
    }
}