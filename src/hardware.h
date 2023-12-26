#pragma once

#include <esp32-hal-ledc.h>
#include <HardwareSerial.h>
#include <Wire.h>

#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <MHZ19.h>

#define PIN_MATRIX_CS 5
#define PIN_SPEAKER 15
#define PIN_FAN_PWM 32
#define PIN_HUMIDIFIER_PWM 33
#define PIN_BME_SDA 25
#define PIN_BME_SCL 26

#define UART_CO2 2

#define PWM_CHANNEL_FAN 5
#define PWM_CHANNEL_HUMIDIFIER 6

const unsigned int BME_ADDRESS = 0x76;

const unsigned long FAN_PWM_BITS = 8;
const unsigned long HUMIDIFIER_PWM_BITS = 8;

const int numberOfHorizontalDisplays = 1;
const int numberOfVerticalDisplays = 1;

const int spacer = 1;
const int width = 5 + spacer;
const int end_spacer = width * 2;
const int height = 8;

static Max72xxPanel matrix = Max72xxPanel(PIN_MATRIX_CS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

static TwoWire bmeWire(0);
static Adafruit_BME280 bme;

static MHZ19 Mhz19;
static HardwareSerial co2Uart(UART_CO2);
