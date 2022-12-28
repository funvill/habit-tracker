#include <Arduino.h>
#include "FastLED.h"
#include <WiFi.h>

#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "EEPROM.h"

#include "globals.h"
#include "database.h"

static uint8_t gMode;

const int timeZone = -8; // Pacific Standard Time (USA)
const long utcOffsetInSeconds = 8 * 60 * 60;
const char *NTP_SERVER = "pool.ntp.org";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, utcOffsetInSeconds);

void flipTheStatusLED();
void checkInputs();
void getCurrentTime();
time_t getNtpTime();

// Modes
void modeCalendar();

// Helpers
void loadingAnimation();
void showWinningAnimation();

// NeoPixel
CRGB leds[PIXELS_COUNT];

#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h" // Used for displaying webpages
#include <AsyncElegantOTA.h>   // Used for Update via webpage. https://github.com/ayushsharma82/AsyncElegantOTA

// Set up the web server.
AsyncWebServer webServer(HTTP_PORT);

// DNS server used for captive portal
#include <DNSServer.h>
DNSServer dnsServer;

#include <ESPConnect.h>

void httpIndex(AsyncWebServerRequest *request)
{
  String html = "<!DOCTYPE html><html><head><title>EveryDayCalendar</title></head><body><h1>EveryDayCalendar</h1><p>Version 0.1.2 (2022-Dec-27)</p><p><a href='/update'>Firmware Update</a></p>";

  html += "<p><strong>WiFi SSID</strong>: " + WiFi.SSID() + "</p>";
  html += "<p><strong>WiFi RSSI</strong>: " + String(WiFi.RSSI()) + " dB</p>";
  html += "<p><strong>WiFi IP</strong>: " + WiFi.localIP().toString() + "</p>";
  html += "<p><strong>WiFi Gateway</strong>: " + WiFi.gatewayIP().toString() + "</p>";
  html += "<p><strong>WiFi Subnet</strong>: " + WiFi.subnetMask().toString() + "</p>";
  html += "<p><strong>WiFi MAC</strong>: " + WiFi.macAddress() + "</p>";
  html += "<p><strong>WiFi BSSID</strong>: " + WiFi.BSSIDstr() + "</p>";

  html += "<p><strong>Mode</strong>: " + String(gMode) + "</p>";
  html += "<p><strong>LED_BRIGHTNESS</strong>: " + String(LED_BRIGHTNESS) + "</p>";
  html += "<p><strong>HTTP_PORT</strong>: " + String(HTTP_PORT) + "</p>";

  html += "<p><strong>Timezone</strong>: " + String(timeZone) + "</p>";
  html += "<p><strong>UTC Offset</strong>: " + String(utcOffsetInSeconds) + "</p>";
  html += "<p><strong>NTP Server</strong>: " + String(NTP_SERVER) + "</p>";
  html += "<p><strong>Current Epoch Time</strong>: " + String(timeClient.getEpochTime()) + "</p>";
  
  // digital clock display of the time
  html += "<p><strong>TimeLib</strong>: ";
  html += (String)year();
  html += "/";
  html += (String)month();
  html += "/";
  html += (String)day();
  html += " ";
  html += (String)hour();
  html += ":";
  html += (String)minute();
  html += ":";
  html += (String)second();
  html += "</p>";

  html += "<h2>Database: </h2><p>";

  for (int dayOffset = 0; dayOffset < DAYS_IN_YEAR; dayOffset++)
  {
    uint8_t value = DatabaseGetOffsetRaw(dayOffset);

    if (value > 0)
    {
      // Base on the day of the year, print the month and day
      uint8_t month = 1;
      uint16_t dayOfTheYear = dayOffset;
      while (dayOfTheYear > monthDays(month, year()))
      {
        dayOfTheYear -= monthDays(month, year());
        month++;
      }
      uint8_t day = dayOfTheYear;

      html += (String)(dayOffset);
      html += " [";
      html += (String)(month);
      html += "/";
      html += (String)(day);
      html += "] = ";
      html += (String)(value);
      html += "[";
      for (int bitOffset = 0; bitOffset < 7; bitOffset++)
      {
        if (value & (1 << bitOffset))
        {
          html += "1";
        }
        else
        {
          html += "0";
        }
        html += ",";
      }
      html += "],  <br />";
    }
  }
  html += "</p>";

  html += "</body></html>";

  request->send_P(200, "text/html", html.c_str());
}

void setupServer()
{
  webServer.on("/", HTTP_GET, httpIndex);
}

void setup()
{

  gMode = MODE_CALENDAR;

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

  // WifiManager
  ESPConnect.autoConnect("EveryDayCalendar");
  if (ESPConnect.begin(&webServer))
  {
    Serial.println("Connected to WiFi");
    Serial.println("IPAddress: " + WiFi.localIP().toString());
  }
  else
  {
    Serial.println("Failed to connect to WiFi");
  }

  // Start the DNS server
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Configure the web server endpoints.
  AsyncElegantOTA.begin(&webServer); // Start AsyncElegantOTA
  setupServer();
  webServer.begin();

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Network information for ");
  Serial.println(WiFi.SSID());

  Serial.println("BSSID : " + WiFi.BSSIDstr());
  Serial.print("MAC Address : ");
  Serial.println(WiFi.macAddress());
  Serial.print("Gateway IP : ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Subnet Mask : ");
  Serial.println(WiFi.subnetMask());
  Serial.println((String) "RSSI : " + WiFi.RSSI() + " dB");
  Serial.print("ESP32 IP : ");
  Serial.println(WiFi.localIP());

  // Time server
  Serial.print("Connecting to time server NTP: ");
  Serial.println(NTP_SERVER);
  timeClient.begin();
  getNtpTime(); // Get the current time from the NTP server

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
  // pinMode(PIN_PREV, INPUT_PULLUP);
  // pinMode(PIN_NEXT, INPUT_PULLUP);

  // set master brightness control
  FastLED.setBrightness(LED_BRIGHTNESS);

  // Loading is done. Reset the LEDs to black
  FastLED.showColor(CRGB::Black);

  // Loads the database
  loadsDatabase();
  printDatabase(year()); // Debug
}

// the loop function runs over and over again forever
void loop()
{
  // do some periodic updates
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

  switch (gMode)
  {
  case MODE_CALENDAR:
    modeCalendar();
    break;
  default:
    Serial.print("Unknown mode. gMode: ");
    Serial.println(gMode);
    break;
  }

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

  // Mode selectors
  if (digitalRead(PIN_MODE) == BUTTON_DOWN_STATE)
  {
    // Change the mode
    gMode++;
    if (gMode > MODE_MAX)
    {
      gMode = MODE_MIN;
    }
    leds[21 - 1] = BUTTON_DOWN_COLOR;
  }

  // Win 1
  if (digitalRead(PIN_WIN1) == BUTTON_DOWN_STATE)
  {
    showWinningAnimation();
    DatabaseSet(year(), month(), day(), 0, true);
  }
  // Win 2
  if (digitalRead(PIN_WIN2) == BUTTON_DOWN_STATE)
  {
    showWinningAnimation();
    DatabaseSet(year(), month(), day(), 1, true);
  }

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

void modeCalendar()
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

  // The current day of the month should fade in and out of the cursor color (gold)
  // and the background color showing what values have been set so far.
  leds[day() + dayOfWeek - 1] = breathBetweenToColors(COLOR_FAIL, COLOR_SUCCESS, 2.0);
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

void SetAllLEDs(CHSV color)
{
  for (int i = 0; i < PIXELS_COUNT; i++)
  {
    leds[i] = color;
  }
  FastLED.show();
}

// scroll the words WIN across the display 7x6 pixels
void showWinningAnimation()
{
  for (int offset = 0; offset < 2; offset++)
  {
    SetAllLEDs(COLOR_SUCCESS);
    delay(200);
    SetAllLEDs(COLOR_NOT_IN_MONTH);
    delay(200);
  }
}