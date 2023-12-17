#pragma once

#include <esp32-hal-ledc.h>
#include <HardwareSerial.h>

#include <Adafruit_GFX.h>
#include <DHT.h>
#include <Max72xxPanel.h>
#include <MHZ19.h>

#define PIN_DHT 4
#define PIN_MATRIX_CS 5
#define PIN_SPEAKER 15
#define PIN_FAN_PWM 32

#define UART_CO2 2
#define PWM_CHANNEL_FAN 5

const int numberOfHorizontalDisplays = 1;
const int numberOfVerticalDisplays = 1;

const int spacer = 1;
const int width = 5 + spacer;
const int end_spacer = width * 2;
const int height = 8;

static Max72xxPanel matrix = Max72xxPanel(PIN_MATRIX_CS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
static DHT dht(PIN_DHT, DHT11);

static MHZ19 Mhz19;
static HardwareSerial co2Uart(UART_CO2);
