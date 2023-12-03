#include "main.h"

const char *API_URL = "https://192.227.150.106:21219/sensor";

extern const char SSL_CERT[]  asm("_binary_certs_api_pem_start");
extern const char WEB_INDEX[] asm("_binary_html_index_html_start");

const unsigned int connection_timeout = 1000;
const unsigned int tcp_timeout = 1000;

const int numberOfHorizontalDisplays = 1;
const int numberOfVerticalDisplays = 1;
const int spacer = 1;
const int width = 5 + spacer;
const int height = 8;

Max72xxPanel matrix = Max72xxPanel(PIN_MATRIX_CS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
DHT dht(PIN_DHT, DHT11);

WiFiClientSecure client;
HTTPClient http;
WebServer server(80);

unsigned long last_sensor_update = 0ul;
unsigned long last_sensor_sent = 0ul;

TaskHandle_t UiTask;
TaskHandle_t DataUpdateTask;
TaskHandle_t WebTask;

const TickType_t mutex_wait_time = portMAX_DELAY;
SemaphoreHandle_t wifi_connection_mutex = xSemaphoreCreateMutex();

SensorData sensor_data;

const Alert Alerts[] = {
        {ALERT_HUMIDITY,    settings.get().alert_humidity,    sensor_data.humidity},
        {ALERT_TEMPERATURE, settings.get().alert_temperature, sensor_data.temperature},
};

String sensor_data_string = "";
int current_letter_index = 0;

void setup() {
#ifdef DEBUG
    Serial.begin(9600);
    Serial.println("Initializing");
#endif

    settings.begin();

    const auto &config = settings.get();
    matrix.setRotation(config.screen_rotation);
    matrix.setIntensity(config.screen_brightness);

    client.setCACert(SSL_CERT);

    wifi_connect();
    play_sound(SOUND_WIFI_ON);

    dht.begin();

    xTaskCreatePinnedToCore(ui_loop, "UI", 10240, nullptr, 1, &UiTask, 0);
    xTaskCreatePinnedToCore(data_loop, "Data", 10240, nullptr, 1, &DataUpdateTask, 1);
    xTaskCreatePinnedToCore(web_loop, "Web", 10240, nullptr, 1, &WebTask, 1);

    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(DataUpdateTask);
}

__attribute__((unused)) void loop() {}

void wifi_connect() {
    while (xSemaphoreTake(wifi_connection_mutex, mutex_wait_time) != pdTRUE) {
#ifdef DEBUG
        Serial.println("Can't take (wifi_connect)");
#endif
    }

    matrix.fillScreen(LOW);
    matrix.write();
    matrix.drawChar(0, 0, 'W', HIGH, LOW, 1);
    matrix.write();

    WiFi.disconnect(true);

    WiFiClass::mode(WIFI_STA);
    WiFi.setSleep(WIFI_PS_NONE);
    WiFi.begin(ssid, password);

    unsigned int attempt = 0;
    while (WiFiClass::status() != WL_CONNECTED && attempt < settings.get().wifi_max_connect_attempts) {
        matrix.fillScreen(LOW);
        matrix.drawChar((int16_t) (attempt % width - spacer), 0, '.', HIGH, LOW, 1);
        matrix.write();

        delay(100);
        ++attempt;
    }

    if (WiFiClass::status() != WL_CONNECTED) {
        matrix.fillScreen(LOW);
        matrix.drawChar(0, 0, 'F', HIGH, LOW, 1);
        matrix.write();

        play_sound(SOUND_WIFI_FAIL);
        xSemaphoreGive(wifi_connection_mutex);

        ESP.restart();
        return;
    }

    matrix.fillScreen(LOW);
    matrix.drawChar(0, 0, 'K', HIGH, LOW, 1);
    matrix.write();

    xSemaphoreGive(wifi_connection_mutex);

#ifdef DEBUG
    Serial.print("Connected to WiFi with ");
    Serial.print(attempt);
    Serial.println(" attempts");
#endif
}

void update_sensor_data() {
    const auto &config = settings.get();
    if (last_sensor_update == 0ul || (millis() - last_sensor_update) > config.sensor_update_interval) {
        sensor_data.humidity = dht.readHumidity() + config.humidity_calibration;
        sensor_data.temperature = dht.readTemperature() + config.temperature_calibration;

        sensor_data_string = String(sensor_data.temperature, 1) + " C" + "   "
                             + String(sensor_data.humidity, 0) + " %  ";
        last_sensor_update = millis();

#ifdef DEBUG
        Serial.print(sensor_data.temperature);
        Serial.print("С ");
        Serial.print(sensor_data.humidity);
        Serial.println("%");
#endif
    }
}

[[noreturn]] void data_loop(void *) {
    for (;;) {
        esp_task_wdt_reset();

        update_sensor_data();
        process_alerts();

        if (WiFiClass::status() != WL_CONNECTED) {
#ifdef DEBUG
            Serial.println("WiFi lost connection");
#endif
            wifi_connect();
        }

        send_sensor_data();

        delay(1000);
    }
}

[[noreturn]] void ui_loop(void *) {
    for (;;) {
        while (xSemaphoreTake(wifi_connection_mutex, mutex_wait_time) != pdTRUE) {
#ifdef DEBUG
            Serial.println("Can't take (ui_loop)");
#endif
        }
        const auto &config = settings.get();
        matrix.setIntensity(config.screen_brightness);
        matrix.setRotation(config.screen_rotation);
        matrix.fillScreen(LOW);

        if (sensor_data_string.length() == 0 || current_letter_index > width * sensor_data_string.length() - spacer) {
            current_letter_index = 0;
            xSemaphoreGive(wifi_connection_mutex);

            delay(config.text_loop_delay);
            continue;
        }

        int letter = current_letter_index / width;
        int x = (matrix.width() - 1) - current_letter_index % width;
        int y = (matrix.height() - height) / 2;

        while (x + width - spacer >= 0 && letter >= 0) {
            matrix.drawChar((int16_t) x, (int16_t) y, sensor_data_string[letter], HIGH, LOW, 1);

            letter--;
            x -= width;
        }

        ++current_letter_index;

        matrix.write();
        delay(config.text_animation_delay);

        xSemaphoreGive(wifi_connection_mutex);
    }
}

void send_sensor_data() {
    const auto &config = settings.get();
    if (last_sensor_sent == 0ul || (millis() - last_sensor_sent) > config.sensor_send_interval) {
        last_sensor_sent = millis();

        String result;
        StaticJsonDocument<64> doc;
        if (!isnan(sensor_data.temperature)) doc["Tamb"] = sensor_data.temperature;
        if (!isnan(sensor_data.temperature)) doc["Hum"] = sensor_data.humidity;
        serializeJson(doc, result);

        http.setConnectTimeout(connection_timeout);
        http.setTimeout(tcp_timeout);

        http.begin(client, API_URL);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("API-Key", API_KEY);

        const auto httpResponseCode = http.POST(result);
        http.end();

#ifdef DEBUG
        Serial.print("API Response: ");
        Serial.println(httpResponseCode);
#endif
    }
}

void process_alerts() {
    const int count = sizeof(Alerts) / sizeof Alerts[0];
    for (int i = 0; i < count; ++i) {
        const auto &alert_value = Alerts[0];
        alert(alert_value.key, alert_value.value, alert_value.entry_prop);
    }
}

[[noreturn]] void web_loop(void *) {
    server.on("/", [] { server.send(200, "text/html", WEB_INDEX); });
    server.on("/settings", HTTPMethod::HTTP_GET, [] {
        server.send(200, "application/json", settings.json());
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
        yield();
    }
}