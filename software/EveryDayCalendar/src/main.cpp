#include <Arduino.h>
#include "FastLED.h"
#include <WiFi.h>

#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const int timeZone = -8; // Pacific Standard Time (USA)
const long utcOffsetInSeconds = 8 * 60 * 60;
const char *NTP_SERVER = "pool.ntp.org";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, utcOffsetInSeconds);

const char *WIFI_SSID = "MagesticCanadianGoose";
const char *WIFI_PASSWORD = "Good-Gray-Sloth";

void flipTheStatusLED();
void checkInputs();
void getCurrentTime();
time_t getNtpTime();
int monthDays(uint8_t month, uint8_t year);
void displayLEDsForMonth();

void loadingAnimation();
void cursorAnimation();

// Pins
// ESP32 Pin out
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

// NeoPixel
const uint8_t PIXELS_COUNT = 42; // 7x6
CRGB leds[PIXELS_COUNT];

const uint8_t LED_BRIGHTNESS = 64; // 0 DARK - 255 BRIGHT
const ulong FRAMES_PER_SECOND = 120;

static uint8_t gHue = 0;

// https://github.com/FastLED/FastLED/wiki/Pixel-reference

const CHSV COLOR_SUCCESS = CHSV(96, 255, 255); // Green
const CHSV COLOR_FAIL = CHSV(0, 255, 128);     // RED
const CHSV COLOR_FUTURE = CHSV(128, 255, 128); // Aqua
const CHSV COLOR_NOT_IN_MONTH = CHSV(0, 0, 0); // Black

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

// Database
// ====================
#define DAYS_IN_YEAR 365
uint8_t database[DAYS_IN_YEAR];

// loads the database from the EEPROM into memory
void loadsDatabase()
{
  // Sets the current database to empty (0xFF)
  for (int i = 0; i < DAYS_IN_YEAR; i++)
  {
    database[i] = 0; // OFF
  }
}

void printDatabase()
{
  Serial.println("Print Database: ");
  for (int offset = 0; offset < DAYS_IN_YEAR; offset++)
  {
    if (database[offset] > 0)
    {
      // Base on the day of the year, print the month and day
      uint8_t month = 1;
      uint16_t dayOfTheYear = offset;
      while (dayOfTheYear > monthDays(month, year()))
      {
        dayOfTheYear -= monthDays(month, year());
        month++;
      }
      uint8_t day = dayOfTheYear;

      Serial.print(offset);
      Serial.print(" [");
      Serial.print(month);
      Serial.print("/");
      Serial.print(day);
      Serial.print("] = ");
      Serial.print(database[offset]);
      Serial.print(",  ");
    }
  }
  Serial.println();
}

// Return the day of the year for the given year, month and day
uint GetDayOfTheYear(uint8_t year, uint8_t month, uint8_t day)
{
  if (month > 12 || day > 31)
  {
    return 0;
  }

  uint dayOfTheYear = 0;
  for (int i = 0; i < month; i++)
  {
    dayOfTheYear += monthDays(i, year);
  }
  dayOfTheYear += day;
  return dayOfTheYear;
}

void DatabaseSet(uint8_t year, uint8_t month, uint8_t day, uint8_t offset, bool success)
{
  uint dayOfTheYear = GetDayOfTheYear(year, month, day);
  database[dayOfTheYear] = bitWrite(database[dayOfTheYear], offset, success);

  Serial.print("DatabaseSet year: ");
  Serial.print(year);
  Serial.print(", month: ");
  Serial.print(month);

  Serial.print(", dayOfTheYear: ");
  Serial.print(dayOfTheYear);
  Serial.print(", offset: ");
  Serial.print(offset);
  Serial.print(", success: ");
  Serial.print(success);
  Serial.print(", value: ");
  Serial.println(database[dayOfTheYear]);
}

bool DatabaseGet(uint8_t year, uint8_t month, uint8_t day, uint8_t offset)
{
  uint dayOfTheYear = GetDayOfTheYear(year, month, day);
  return bitRead(database[dayOfTheYear], offset);
}

// ====================

void setup()
{
  // Add the LEDS first as we use them for status and loading animations
  FastLED.addLeds<NEOPIXEL, PIN_LED_CALENDAR>(leds, PIXELS_COUNT);

  // Set up the serial port for debugging
  // this prevents the button_next, and button_prev from working
  Serial.begin(115200);

  // Wait for the serial port to be ready before continuing
  while (!Serial)
  {
    loadingAnimation();
    delay(10);
  }
  Serial.println("\n\n\n");
  // Print a message to the serial port
  Serial.println("EveryDayCalendar v0.1.1 (2022-Dec-27)");

  WiFi.mode(WIFI_STA); // Optional
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    loadingAnimation();
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Network information for ");
  Serial.println(WIFI_SSID);

  Serial.println("BSSID : " + WiFi.BSSIDstr());
  Serial.print("Gateway IP : ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Subnet Mask : ");
  Serial.println(WiFi.subnetMask());
  Serial.println((String) "RSSI : " + WiFi.RSSI() + " dB");
  Serial.print("ESP32 IP : ");
  Serial.println(WiFi.localIP());

  Serial.print("Connecting to time server NTP: ");
  Serial.print(NTP_SERVER);
  // Set up the time client
  timeClient.begin();

  // Get the current time from the NTP server
  getNtpTime();

  // Set the time to the current time from the NTP server
  setSyncProvider(getNtpTime);
  setSyncInterval(3600);

  // Print the time status
  getCurrentTime();

  // Set the status led to an output
  pinMode(PIN_LED_STATUS, OUTPUT);

  // Set all the buttons to inputs
  pinMode(PIN_WEEK1, INPUT_PULLUP);
  pinMode(PIN_WEEK2, INPUT_PULLUP);
  pinMode(PIN_WEEK3, INPUT_PULLUP);
  pinMode(PIN_WEEK4, INPUT_PULLUP);
  pinMode(PIN_WEEK5, INPUT_PULLUP);
  pinMode(PIN_WEEK6, INPUT_PULLUP);
  pinMode(PIN_DAY1, INPUT_PULLUP);
  pinMode(PIN_DAY2, INPUT_PULLUP);
  pinMode(PIN_DAY3, INPUT_PULLUP);
  pinMode(PIN_DAY4, INPUT_PULLUP);
  pinMode(PIN_DAY5, INPUT_PULLUP);
  pinMode(PIN_DAY6, INPUT_PULLUP);
  pinMode(PIN_DAY7, INPUT_PULLUP);
  pinMode(PIN_WIN1, INPUT_PULLUP);
  pinMode(PIN_WIN2, INPUT_PULLUP);
  pinMode(PIN_MODE, INPUT_PULLUP);

  // If Serial is active you can't use these pins
  pinMode(PIN_PREV, INPUT_PULLUP);
  pinMode(PIN_NEXT, INPUT_PULLUP);

  // set master brightness control
  FastLED.setBrightness(LED_BRIGHTNESS);

  // Loading is done. Reset the LEDs to black
  FastLED.showColor(CRGB::Black);

  // Loads the database
  loadsDatabase();

  // Debug. Set some success over the last month.
  // DatabaseSet(year(), month(), 3, 0, true);
  // DatabaseSet(year(), month(), 6, 0, true);
  // DatabaseSet(year(), month(), 9, 0, true);
  DatabaseSet(year(), month(), 12, 0, true);
  DatabaseSet(year(), month(), 15, 0, true);

  printDatabase(); // Debug
}

// the loop function runs over and over again forever
void loop()
{
  // do some periodic updates
  EVERY_N_MILLISECONDS(50) { gHue += 1; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS(1)
  {
    flipTheStatusLED();
  }

  EVERY_N_MINUTES(1)
  {
    getCurrentTime();
  }

  // Set all pixels to black
  FastLED.showColor(CRGB::Black);

  // Update only the months pixels
  displayLEDsForMonth();

  // Check for user input to see if we need to change modes
  checkInputs();

  // Insert a delay to keep the framerate modest
  // FastLED.delay(1000 / FRAMES_PER_SECOND);
  FastLED.show();
}

void flipTheStatusLED()
{
  static bool statusLEDState = LOW;
  statusLEDState = !statusLEDState;
  digitalWrite(PIN_LED_STATUS, statusLEDState);
}

void checkInputs()
{
  const uint8_t BUTTON_DOWN_STATE = LOW;
  const CRGB BUTTON_DOWN_COLOR = CRGB::Gold;

  // Check to see if the buttons have been pressed.

  // For weeks 1-6, set the entire horazonal row to the button down color
  if (digitalRead(PIN_WEEK1) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY1; offset <= DAY7; offset++)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_WEEK2) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY8; offset <= DAY14; offset++)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_WEEK3) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY15; offset <= DAY21; offset++)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_WEEK4) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY22; offset <= DAY28; offset++)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_WEEK5) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY29; offset <= DAY35; offset++)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_WEEK6) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY36; offset <= DAY42; offset++)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }

  // For days 1-7, set the entire vertical column to the button down color
  if (digitalRead(PIN_DAY1) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY1; offset <= DAY36; offset += 7)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_DAY2) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY2; offset <= DAY37; offset += 7)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_DAY3) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY3; offset <= DAY38; offset += 7)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_DAY4) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY4; offset <= DAY39; offset += 7)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_DAY5) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY5; offset <= DAY40; offset += 7)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_DAY6) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY6; offset <= DAY41; offset += 7)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }
  if (digitalRead(PIN_DAY7) == BUTTON_DOWN_STATE)
  {
    for (uint8_t offset = DAY7; offset <= DAY42; offset += 7)
    {
      leds[offset] = BUTTON_DOWN_COLOR;
    }
  }

  if (digitalRead(PIN_WIN1) == BUTTON_DOWN_STATE)
  {
    leds[7 - 1] = BUTTON_DOWN_COLOR;
  }
  if (digitalRead(PIN_WIN2) == BUTTON_DOWN_STATE)
  {
    leds[14 - 1] = BUTTON_DOWN_COLOR;
  }
  if (digitalRead(PIN_MODE) == BUTTON_DOWN_STATE)
  {
    leds[21 - 1] = BUTTON_DOWN_COLOR;
  }
  // if (digitalRead(PIN_PREV) == BUTTON_DOWN_STATE) { leds[28-1] = BUTTON_DOWN_COLOR; }
  // if (digitalRead(PIN_NEXT) == BUTTON_DOWN_STATE) { leds[35-1] = BUTTON_DOWN_COLOR; }
}

void getCurrentTime()
{
  // Serial.println(timeClient.getFormattedTime());

  Serial.print("Get Current Day: ");
  Serial.println(timeClient.getDay());

  Serial.print("Current Epoch Time: ");
  Serial.println(timeClient.getEpochTime());

  // digital clock display of the time
  Serial.print("TimeLib: ");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();

  // Calulate the Epoch for the start of this month (1st)
  time_t startOfMonth = timeClient.getEpochTime() - (day() * 24 * 60 * 60);
  Serial.print("Start of month (epoch): ");
  Serial.println(startOfMonth);

  // figure out the day of the week for the start of this month
  int dayOfWeek = weekday(startOfMonth);
  Serial.print("Start of the month, day of week: ");
  Serial.println(dayOfWeek);

  // Figure out how many days are in this month (28, 29, 30, 31)
  int daysInMonth = monthDays(month(), year());
  Serial.print("Days in this month: ");
  Serial.println(daysInMonth);

  // Calulate what was the day of the week at the start of this month (1st)
  // 0 = Sunday, 1 = Monday, 2 = Tuesday, 3 = Wednesday, 4 = Thursday, 5 = Friday, 6 = Saturday,
}

// This function returns the number of days in a given month and year
// It takes the month and year as parameters
int monthDays(uint8_t month, uint8_t year)
{
  // February
  if (month == 2)
  {
    // Leap year
    if (year % 4 == 0)
    {
      return 29;
    }
    // Not a leap year
    else
    {
      return 28;
    }
  }
  if (month == 4 || month == 6 || month == 9 || month == 11)
  {
    return 30;
  }

  if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12)
  {
    return 31;
  }

  return 0;
}

time_t getNtpTime()
{
  timeClient.update();
  return timeClient.getEpochTime();
}

// https://github.com/marmilicious/FastLED_examples/blob/master/breath_effect_v2.ino
CRGB breathBetweenToColors(CHSV start, CHSV end, float pulseSpeed = 2.0)
{
  // static float pulseSpeed = 2.0; // Larger value gives faster pulse.

  // start.value; // Pulse minimum value (Should be less then valueMax).
  // end.value; // Pulse maximum value (Should be larger then valueMin).
  static float delta = (end.value - start.value) / 2.35040238; // Do Not Edit

  float dV = ((exp(sin(pulseSpeed * millis() / 2000.0 * PI)) - 0.36787944) * delta);
  float val = start.value + dV;
  uint8_t hue = map(val, start.value, end.value, start.hue, end.hue); // Map hue based on current val
  uint8_t sat = map(val, start.value, end.value, start.sat, end.sat); // Map sat based on current val

  return CHSV(hue, sat, val);
}

void displayLEDsForMonth()
{

  // Calulate the Epoch for the start of this month (1st)
  time_t startOfMonth = timeClient.getEpochTime() - (day() * 24 * 60 * 60);
  // Figure out the day of the week for the start of this month
  int dayOfWeek = weekday(startOfMonth);
  // Figure out how many days are in this month (28, 29, 30, 31)
  int daysInMonth = monthDays(month(), year());
  // 0 = Sunday, 1 = Monday, 2 = Tuesday, 3 = Wednesday, 4 = Thursday, 5 = Friday, 6 = Saturday,

  // Set any of the LEDS that are before the start of the month to black
  for (int offset_pixel = 0; offset_pixel < dayOfWeek; offset_pixel++)
  {
    leds[offset_pixel] = COLOR_NOT_IN_MONTH;
  }

  // Fill the past days with the rainbow color
  for (int offset_pixel = dayOfWeek; offset_pixel < dayOfWeek + day(); offset_pixel++)
  {
    if (DatabaseGet(year(), month(), offset_pixel - dayOfWeek + 1, 0))
    {
      leds[offset_pixel] = COLOR_SUCCESS; // Success
    }
    else
    {
      leds[offset_pixel] = COLOR_FAIL; // Failure
    }
  }

  // The current day of the month should fade in and out of the cursor color (gold)
  // and the background color showing what values have been set so far.
  leds[day() + dayOfWeek - 1] = breathBetweenToColors(COLOR_FAIL, COLOR_SUCCESS, 2.1);

  // fill the future days with gray color (no data)
  for (int offset_pixel = dayOfWeek + day(); offset_pixel < dayOfWeek + daysInMonth; offset_pixel++)
  {
    // divide the whole hue range across the number of pixels
    // and add the offset to the current pixel
    leds[offset_pixel] = COLOR_FUTURE;
  }

  // Set any of the LEDS after the month to black
  for (int offset_pixel = dayOfWeek + daysInMonth; offset_pixel < PIXELS_COUNT; offset_pixel++)
  {
    leds[offset_pixel] = COLOR_NOT_IN_MONTH;
  }
}

// Starting with the top left, sets leds illumiated in a progress bar.
// Once it reaches the end, it loops back with a new color.
void loadingAnimation()
{
  static uint8_t offset = 0;
  static uint8_t hue = 0;

  // Starting with black. Loads a new LED once every few ms
  const uint8_t LOADING_ANIMATION_SPEED = 20;

  if (offset > PIXELS_COUNT)
  {
    offset = 0;
    hue += 255 / 4;
  }

  EVERY_N_MILLIS_I(LOADING_ANIMATION, LOADING_ANIMATION_SPEED)
  {
    leds[offset] = CHSV(hue, 255, 128);
    FastLED.show();
    offset++;
  }
}

// The current day of the month should fade in and out of the cursor color (gold)
// and the background color showing what values have been set so far.
void cursorAnimation()
{
}