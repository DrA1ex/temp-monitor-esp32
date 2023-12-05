#pragma once

#include <Adafruit_GFX.h>
#include <DHT.h>
#include <Max72xxPanel.h>

#define PIN_DHT 4
#define PIN_MATRIX_CS 5
#define PIN_SPEAKER 15

const int numberOfHorizontalDisplays = 1;
const int numberOfVerticalDisplays = 1;
const int spacer = 1;
const int width = 5 + spacer;
const int end_spacer = width * 2;
const int height = 8;

static Max72xxPanel matrix = Max72xxPanel(PIN_MATRIX_CS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
static DHT dht(PIN_DHT, DHT11);