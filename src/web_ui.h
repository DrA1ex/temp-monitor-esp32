#pragma once

#include <WebServer.h>

#include "settings.h"

extern const char WEB_INDEX[] asm("_binary_html_index_html_start");

static WebServer server(80);

[[noreturn]] void web_loop(void *) {
    server.on("/", [] { server.send(200, "text/html", WEB_INDEX); });
    server.on("/settings", HTTPMethod::HTTP_GET, [] {
        server.send(200, "application/json", settings.json());
    });
    server.on("/sensor", HTTPMethod::HTTP_GET, [] {
        server.send(200, "application/json", sensor_data.json());
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

    server.begin();

    for (;;) {
        server.handleClient();
        settings.timer().handle_timers();
        delay(1);
    }
}