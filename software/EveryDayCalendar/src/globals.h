/**
 * Globals
 * A list of global variables and constants used in the project
 * Multiple files will include this file
 *
 */

#include <Arduino.h> // Required for ESP32
#include "FastLED.h" // Required for CHSV


#ifndef __GLOBAL_H__
#define __GLOBAL_H__

// The build version is shown on the loading screen.
// the version is shown in the serial prompt and the web page.
const uint8_t BUILD_NUMBER = 5;

// HTTP
#define HTTP_PORT 80

// Pins
// ESP32 Pins
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html
const int PIN_WEEK1 = 4;   // IO-4  or pin 26
const int PIN_WEEK2 = 16;  // IO-16 or pin 27
const int PIN_WEEK3 = 17;  // IO-17 or pin 28
const int PIN_WEEK4 = 5;   // IO-5  or pin 29
const int PIN_WEEK5 = 19;  // IO-19 or pin 31
const int PIN_WEEK6 = 18;  // IO-18 or pin 30
const int PIN_DAY1 = 23;   // IO-23 or pin 37
const int PIN_DAY2 = 22;   // IO-22 or pin 36
const int PIN_DAY3 = 21;   // IO-21 or pin 33
const int PIN_DAY4 = 15;   // IO-15 or pin 23
const int PIN_DAY5 = 12;   // IO-12 or pin 14
const int PIN_DAY6 = 14;   // IO-14 or pin 13
const int PIN_DAY7 = 27;   // IO-27 or pin 12, GPIO27, ADC2_CH7, TOUCH7, RTC_GPIO17, EMAC_RX_DV
const int PIN_WIN1 = 26;   // IO-26 or pin 11
const int PIN_WIN2 = 33;   // IO-33 or pin  9
const int PIN_MODE = 32;   // IO-32 or pin  8
const int PIN_NEXT = 34;   // IO-34 or pin  6, Shares pins with U0RXD (serial). If serial is active you can't use thse pins
const int PIN_PREV = 35;   // IO-35 or pin  7, Shares pins with U0TXD (serial)
const int PIN_ONEWIRE = 2; // IO-0 or pin 24

const int PIN_LED_CALENDAR = 25; // IO-25 or pin 10
const int PIN_LED_STATUS = 13;   // IO-13 or pin 16

// Helpful for debugging when identifiying
// The actual LED on the matrix that is being addressed
const uint8_t DAY1 = 0;
const uint8_t DAY2 = 1;
const uint8_t DAY3 = 2;
const uint8_t DAY4 = 3;
const uint8_t DAY5 = 4;
const uint8_t DAY6 = 5;
const uint8_t DAY7 = 6;
const uint8_t DAY8 = 7;
const uint8_t DAY9 = 8;
const uint8_t DAY10 = 9;
const uint8_t DAY11 = 10;
const uint8_t DAY12 = 11;
const uint8_t DAY13 = 12;
const uint8_t DAY14 = 13;
const uint8_t DAY15 = 14;
const uint8_t DAY16 = 15;
const uint8_t DAY17 = 16;
const uint8_t DAY18 = 17;
const uint8_t DAY19 = 18;
const uint8_t DAY20 = 19;
const uint8_t DAY21 = 20;
const uint8_t DAY22 = 21;
const uint8_t DAY23 = 22;
const uint8_t DAY24 = 23;
const uint8_t DAY25 = 24;
const uint8_t DAY26 = 25;
const uint8_t DAY27 = 26;
const uint8_t DAY28 = 27;
const uint8_t DAY29 = 28;
const uint8_t DAY30 = 29;
const uint8_t DAY31 = 30;
const uint8_t DAY32 = 31;
const uint8_t DAY33 = 32;
const uint8_t DAY34 = 33;
const uint8_t DAY35 = 34;
const uint8_t DAY36 = 35;
const uint8_t DAY37 = 36;
const uint8_t DAY38 = 37;
const uint8_t DAY39 = 38;
const uint8_t DAY40 = 39;
const uint8_t DAY41 = 40;
const uint8_t DAY42 = 41;

// Modes
const uint8_t MODE_CALENDAR = 0;
const uint8_t MODE_BINARY_CLOCK = 1;
const uint8_t MODE_EPOCH_CLOCK = 2;
const uint8_t MODE_BREATHING = 3;
const uint8_t MODE_FADE_POINTS = 4;

const uint8_t MODE_START = MODE_FADE_POINTS;
const uint8_t MODE_MIN = MODE_CALENDAR;
const uint8_t MODE_MAX = MODE_FADE_POINTS;

// LED colors
// https://github.com/FastLED/FastLED/wiki/Pixel-reference
const CHSV COLOR_OFF = CHSV(0, 0, 0);          // Black
const CHSV COLOR_SUCCESS = CHSV(96, 255, 200); // Green
const CHSV COLOR_FAIL = CHSV(0, 255, 128);     // RED
const CHSV COLOR_FUTURE = CHSV(128, 255, 128); // Aqua
const CHSV COLOR_NOT_IN_MONTH = COLOR_OFF;     // Black
const CRGB COLOR_CURSOR = CRGB::Gold;
const CHSV COLOR_FONT = COLOR_SUCCESS;

const uint8_t LED_MATRIX_WIDTH = 7;
const uint8_t LED_MATRIX_HEIGHT = 6;

#define DAYS_IN_YEAR 365

const int timeZone = -8; // Pacific Standard Time (USA)
const long utcOffsetInSeconds = timeZone * 60 * 60;

static const char *MONTHS_SHORT[] = {"XXX", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// 0 = Sunday, 1 = Monday, 2 = Tuesday, 3 = Wednesday, 4 = Thursday, 5 = Friday, 6 = Saturday,
static const char *DAY_SHORT[] = {"Sun", "Mon", "Tue", "Wen", "Thu", "Fri", "Sat"};

// Settings
// ------------------------------------------------------------
const uint8_t PIXELS_COUNT = LED_MATRIX_WIDTH * LED_MATRIX_HEIGHT; // 7x6
const uint8_t LED_BRIGHTNESS = 64;                                 // 0 DARK - 255 BRIGHT
const ulong FRAMES_PER_SECOND = 120;
const uint32_t BAUD_RATE = 115200;

// Globals
// ------------------------------------------------------------
// NeoPixel
extern CRGB leds[PIXELS_COUNT];


static uint8_t gMode;
static uint16_t gCurrentYear;
static uint8_t gCurrentMonth;

#endif // __GLOBAL_H__