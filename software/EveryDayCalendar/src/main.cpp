/**
 * EveryDayCalendar
 *
 *
 * ToDo:
 * MVP
 * [x] Database and settings download from web page
 * - Update previouse day in the database. (cursor)
 * [x] Once connected print the IP address to the matrix
 *
 * Nice to have
 * - Synch database to online store
 * - Symbols for each mode instead of text.
 * - Update settings from web page
 *   - Timezone, UTC offset, NTP server, LED brightness, etc.
 *   - Colors
 */

#include <Arduino.h>
#include "FastLED.h"
#include <WiFi.h>

#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "EEPROM.h"

#include "globals.h"
#include "font.h"
#include "database.h"

static uint8_t gMode;
// static uint8_t gCurrsor;
static uint16_t gCurrentYear;
static uint8_t gCurrentMonth;

// The build version is shown on the loading screen.
// the version is shown in the serial prompt and the web page.
const String VERSION = "build 1";
const uint8_t BUILD_NUMBER = 0b00000001;

const int timeZone = -8; // Pacific Standard Time (USA)
const long utcOffsetInSeconds = 8 * 60 * 60;
const char *NTP_SERVER = "pool.ntp.org";

static const char *MONTHS_SHORT[] = {"XXX", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// 0 = Sunday, 1 = Monday, 2 = Tuesday, 3 = Wednesday, 4 = Thursday, 5 = Friday, 6 = Saturday,
static const char *DAY_SHORT[] = {"Sun", "Mon", "Tue", "Wen", "Thu", "Fri", "Sat"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, utcOffsetInSeconds);

void flipTheStatusLED();
void checkInputs();
void getCurrentTime();
time_t getNtpTime();

// Modes
void modeCalendar();
void modeClock();
void modeProgress();
void modeBreathing();

// Helpers
void SetAllLEDs(CHSV color = COLOR_OFF);
void animationNoWifi();
void loadingAnimation();
void ShowGlyphSearchingWiFi();
void ShowGlyph(const uint8_t *glyph, CHSV color = COLOR_SUCCESS, uint8_t xOffset = 0, uint8_t xGlyphOffset = 0);
void ScrollText(String text, CHSV color = COLOR_FONT, uint16_t delayMs = 100);

#include <Time.h>
time_t GetEpochForDate(uint16_t year, uint8_t month, uint8_t day)
{
  tmElements_t tmSet;
  tmSet.Year = year - 1970; // tmElements_t year is offset from 1970
  tmSet.Month = month;      // Jan = 0
  tmSet.Day = day;          // 1st = 0
  tmSet.Hour = 0;
  tmSet.Minute = 0;
  tmSet.Second = 1;
  return makeTime(tmSet);
}

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
  String html = "<!DOCTYPE html><html><head><title>EveryDayCalendar</title></head><body><h1>Every Day Calendar</h1>";
  html += "<p>Version " + VERSION + "</p>";
  html += "<p><a href='/update'>Firmware Update</a> - <a href='/database'>Download Database</a> - <a href='/reset'>Database Reset</a></p>";

  html += "<h2>WIFI</h2>";
  html += "<p><strong>WiFi SSID</strong>: " + WiFi.SSID() + "</p>";
  html += "<p><strong>WiFi RSSI</strong>: " + String(WiFi.RSSI()) + " dB</p>";
  html += "<p><strong>WiFi IP</strong>: " + WiFi.localIP().toString() + "</p>";
  html += "<p><strong>WiFi Gateway</strong>: " + WiFi.gatewayIP().toString() + "</p>";
  html += "<p><strong>WiFi Subnet</strong>: " + WiFi.subnetMask().toString() + "</p>";
  html += "<p><strong>WiFi MAC</strong>: " + WiFi.macAddress() + "</p>";
  html += "<p><strong>WiFi BSSID</strong>: " + WiFi.BSSIDstr() + "</p>";

  html += "<h2>Other</h2>";
  html += "<p><strong>BAUD_RATE</strong>: " + String(BAUD_RATE) + "</p>";
  html += "<p><strong>LED_BRIGHTNESS</strong>: " + String(LED_BRIGHTNESS) + "</p>";
  html += "<p><strong>HTTP_PORT</strong>: " + String(HTTP_PORT) + "</p>";

  html += "<p><strong>Timezone</strong>: " + String(timeZone) + "</p>";
  html += "<p><strong>UTC Offset</strong>: " + String(utcOffsetInSeconds) + "</p>";
  html += "<p><strong>NTP Server</strong>: " + String(NTP_SERVER) + "</p>";
  html += "<p><strong>Current Epoch Time</strong>: " + String(timeClient.getEpochTime()) + "</p>";

  html += "<h2>Display</h2>";
  html += "<p><strong>Mode</strong>: " + String(gMode) + "</p>";
  html += "<p><strong>gCurrentYear</strong>: " + String(gCurrentYear) + "</p>";
  html += "<p><strong>gCurrentMonth</strong>: " + String(gCurrentMonth) + " (" + (String)MONTHS_SHORT[gCurrentMonth] + ")</p>";

  int daysInMonth = monthDays(gCurrentMonth, gCurrentYear);
  int dayOfWeek = weekday(GetEpochForDate(gCurrentYear, gCurrentMonth, 1)) - 1;
  html += "<p><strong>daysInMonth</strong>: " + String(daysInMonth) + "</p>";
  html += "<p><strong>dayOfWeek</strong>: " + String(dayOfWeek) + " (" + DAY_SHORT[dayOfWeek] + ")</p>";

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

void httpDatabase(AsyncWebServerRequest *request)
{
  String html = "<!DOCTYPE html><html><head><title>EveryDayCalendar</title></head><body><h1>Every Day Calendar - Database</h1>";
  html += "<p><a href='/'>Home</a></p>";
  html += "<textarea style='margin: 0 10px; width:80%; height: 400px'>";

  for (int dayOffset = 0; dayOffset < DAYS_IN_YEAR; dayOffset++)
  {
    uint8_t value = DatabaseGetOffsetRaw(dayOffset);
    html += (String)(value);
    html += ",";
  }

  html += "</textarea>";
  html += "</body></html>";
  request->send_P(200, "text/html", html.c_str());
}

// ToDo: alow users to set the calendar via the web interface
void httpSet(AsyncWebServerRequest *request)
{
  String html = "<!DOCTYPE html><html><head><title>EveryDayCalendar</title></head><body><h1>Every Day Calendar - Set</h1>";
  html += "<p><a href='/'>Home</a></p>";
  html += "ToDo: allow users to set the calendar via the web interface";
  html += "</body></html>";
  request->send_P(200, "text/html", html.c_str());
}

void httpReset(AsyncWebServerRequest *request)
{
  ESPConnect.erase();
  resetDatabase();

  String html = "<!DOCTYPE html><html><head><title>EveryDayCalendar</title></head><body><h1>Every Day Calendar - Reset</h1>";
  html += "<p><a href='/'>Home</a></p>";
  html += "<strong>Database has been reset</strong>";

  html += "</body></html>";
  request->send_P(200, "text/html", html.c_str());
  delay(1000);

  // Reset the device
  ESP.restart();
}

void setupServer()
{
  webServer.on("/", HTTP_GET, httpIndex);
  webServer.on("/database", HTTP_GET, httpDatabase);
  webServer.on("/set", HTTP_POST, httpSet);
  webServer.on("/reset", HTTP_GET, httpReset);
}

void setup()
{
  // Set the status led to an output
  // This shows that there is power to the boards
  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS, HIGH);

  // Configure the LEDs
  FastLED.addLeds<NEOPIXEL, PIN_LED_CALENDAR>(leds, PIXELS_COUNT);
  // set master brightness control
  FastLED.setBrightness(LED_BRIGHTNESS);

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

  // Set up the serial port for debugging
  // this prevents the button_next, and button_prev from working
  Serial.begin(BAUD_RATE);

  // Wait for the serial port to be ready before continuing
  // We want to display the loading animation for at least 1 second
  // If the serial port fails to load, only wait up to 3 seconds.
  while ((!Serial && millis() < 1000 * 3) || millis() < 1000 * 1)
  {
    loadingAnimation();
    delay(1);
  }
  Serial.println("\n\n\n");

  // Print a message to the serial port
  Serial.println("EveryDayCalendar v0.1.1 (2022-Dec-27)");

  // Load the settings
  loadsDatabase();

  // WifiManager
  ShowGlyphSearchingWiFi();
  String APSSID = (String) "EveryDayCalendar-" + WiFi.macAddress();
  ESPConnect.autoConnect(APSSID.c_str() );
  if (ESPConnect.begin(&webServer))
  {
    Serial.println("Connected to WiFi");
    Serial.println("IPAddress: " + WiFi.localIP().toString());
    delay (100); // Give the TCP stack a chance to connect to the network
  }
  else
  {
    Serial.println("Failed to connect to WiFi");
    while (true)
    {
      animationNoWifi();
    }
    return;
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

  // Print the IP address to the LEDS
  ScrollText((String) "IP " + WiFi.localIP().toString());

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

  // Set the current year and month to the current time
  // The user can change this using the buttons.
  gCurrentYear = year();
  gCurrentMonth = month();
  gMode = MODE_CALENDAR;

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

  switch (gMode)
  {
  case MODE_CALENDAR:
    modeCalendar();
    break;
  case MODE_CLOCK:
    modeClock();
    break;
  case MODE_PROGRESS:
    modeProgress();
    break;
  case MODE_BREATHING:
    modeBreathing();
    break;
  default:
    gMode = MODE_MIN;
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

    switch (gMode)
    {
    case MODE_CALENDAR:
      ScrollText("D");
      break;
    case MODE_CLOCK:
      ScrollText("T");
      break;
    case MODE_PROGRESS:
      ScrollText("P");
      break;
    case MODE_BREATHING:
      ScrollText("B");
      break;
    default:
      ScrollText("XX");
      gMode = MODE_CALENDAR;
      break;
    }

    Serial.println("Mode changed to: " + (String)gMode);

    leds[21 - 1] = BUTTON_DOWN_COLOR;
  }

  // Previous Month
  if (digitalRead(PIN_WIN1) == BUTTON_DOWN_STATE)
  {
    if (gCurrentMonth <= 1)
    {
      gCurrentMonth = 12;
      gCurrentYear -= 1;

      ScrollText((String)gCurrentYear, COLOR_FONT, 50);
    }
    else
    {
      gCurrentMonth -= 1;
    }

    Serial.println("gCurrentMonth: " + (String)gCurrentMonth + " (" + (String)MONTHS_SHORT[gCurrentMonth] + ") gCurrentYear: " + (String)gCurrentYear);
    Serial.println("GetEpochForDate: " + (String)GetEpochForDate(gCurrentYear, gCurrentMonth, 1));

    ScrollText((String)MONTHS_SHORT[gCurrentMonth], COLOR_FONT, 50);
  }
  // Next Month
  if (digitalRead(PIN_WIN2) == BUTTON_DOWN_STATE)
  {
    gCurrentMonth++;
    if (gCurrentMonth > 12)
    {
      gCurrentMonth = 1;
      gCurrentYear += 1;
      ScrollText((String)gCurrentYear, COLOR_FONT, 50);
    }

    Serial.println("gCurrentMonth: " + (String)gCurrentMonth + " (" + (String)MONTHS_SHORT[gCurrentMonth] + ") gCurrentYear: " + (String)gCurrentYear);
    Serial.println("GetEpochForDate: " + (String)GetEpochForDate(gCurrentYear, gCurrentMonth, 1));

    ScrollText((String)MONTHS_SHORT[gCurrentMonth], COLOR_FONT, 50);
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

  // For days 1-7, use them as win conditions
  if (digitalRead(PIN_DAY1) == BUTTON_DOWN_STATE)
  {
    DatabaseSet(year(), month(), day(), 0, true);
    ScrollText("Win 1", COLOR_FONT, 50);
  }
  if (digitalRead(PIN_DAY2) == BUTTON_DOWN_STATE)
  {
    DatabaseSet(year(), month(), day(), 1, true);
    ScrollText("Win 2", COLOR_FONT, 50);
  }
  if (digitalRead(PIN_DAY3) == BUTTON_DOWN_STATE)
  {
    DatabaseSet(year(), month(), day(), 2, true);
    ScrollText("Win 3", COLOR_FONT, 50);
  }
  if (digitalRead(PIN_DAY4) == BUTTON_DOWN_STATE)
  {
    DatabaseSet(year(), month(), day(), 3, true);
    ScrollText("Win 4", COLOR_FONT, 50);
  }
  if (digitalRead(PIN_DAY5) == BUTTON_DOWN_STATE)
  {
    DatabaseSet(year(), month(), day(), 4, true);
    ScrollText("Win 5", COLOR_FONT, 50);
  }
  if (digitalRead(PIN_DAY6) == BUTTON_DOWN_STATE)
  {
    DatabaseSet(year(), month(), day(), 5, true);
    ScrollText("Win 6", COLOR_FONT, 50);
  }
  if (digitalRead(PIN_DAY7) == BUTTON_DOWN_STATE)
  {
    DatabaseSet(year(), month(), day(), 6, true);
    ScrollText("Win 7", COLOR_FONT, 50);
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
  time_t startOfMonth = GetEpochForDate(gCurrentYear, gCurrentMonth, 1);
  // time_t startOfMonth = timeClient.getEpochTime() - (day() * 24 * 60 * 60);
  // Figure out the day of the week for the start of this month
  int dayOfWeek = weekday(startOfMonth) - 1;
  // Figure out how many days are in this month (28, 29, 30, 31)
  int daysInMonth = monthDays(gCurrentMonth, gCurrentYear);
  // 0 = Sunday, 1 = Monday, 2 = Tuesday, 3 = Wednesday, 4 = Thursday, 5 = Friday, 6 = Saturday,

  // Figure out if this month is in the future or not.
  bool isFuture = false;
  if (gCurrentYear > year())
  {
    isFuture = true;
  }
  else if (gCurrentYear == year())
  {
    if (gCurrentMonth > month())
    {
      isFuture = true;
    }
  }

  // Set all the LEDS to black.
  SetAllLEDs(COLOR_NOT_IN_MONTH);

  if (!isFuture)
  {
    // Fill in all the days with the data from the database
    for (int offset_pixel = dayOfWeek; offset_pixel < dayOfWeek + daysInMonth; offset_pixel++)
    {
      if (DatabaseGet(gCurrentYear, gCurrentMonth, offset_pixel - dayOfWeek + 1, 0))
      {
        leds[offset_pixel] = COLOR_SUCCESS; // Success
      }
      else
      {
        leds[offset_pixel] = COLOR_FAIL; // Failure
      }
    }
  }

  // If we are in the current year, and month.. Then show the future dates as COLOR_FUTURE
  if (gCurrentYear == year() && gCurrentMonth == month())
  {
    // fill the future days with gray color (no data)
    for (int offset_pixel = dayOfWeek + day(); offset_pixel < dayOfWeek + daysInMonth; offset_pixel++)
    {
      // divide the whole hue range across the number of pixels
      // and add the offset to the current pixel
      leds[offset_pixel] = COLOR_FUTURE;
    }

    // The current day of the month should fade in and out of the cursor color (gold)
    // and the background color showing what values have been set so far.
    leds[day() + dayOfWeek - 1] = breathBetweenToColors(COLOR_FAIL, COLOR_SUCCESS, 2.0);
  }
  else if (isFuture)
  {
    // If this month is in the future, then show the whole month as COLOR_FUTURE
    for (int offset_pixel = dayOfWeek; offset_pixel < dayOfWeek + daysInMonth; offset_pixel++)
    {
      leds[offset_pixel] = COLOR_FUTURE;
    }
  }
}

// Binary clock
#define BitVal(data, y) ((data >> y) & 1) /** Return Data.Y value   **/
void modeClock()
{
  // Get the current time in seconds
  time_t now = timeClient.getEpochTime();

  uint8_t y = year(now);
  uint8_t m = month(now);
  uint8_t d = day(now);
  uint8_t hr = hour(now);
  uint8_t min = minute(now);
  uint8_t sec = second(now);

  // Set all the LEDS to black.
  SetAllLEDs(COLOR_OFF);

  for (int offset_pixel = DAY1; offset_pixel < DAY7; offset_pixel++)
  {
    if (bitRead(y, (offset_pixel - DAY1)))
    {
      leds[offset_pixel] = CHSV(0, 255, 128);
    }
  }
  for (int offset_pixel = DAY8; offset_pixel < DAY14; offset_pixel++)
  {
    if (bitRead(m, (offset_pixel - DAY8)))
    {
      leds[offset_pixel] = CHSV(42, 255, 128);
    }
  }
  for (int offset_pixel = DAY15; offset_pixel < DAY21; offset_pixel++)
  {
    if (bitRead(d, (offset_pixel - DAY15)))
    {
      leds[offset_pixel] = CHSV(84, 255, 128);
    }
  }
  for (int offset_pixel = DAY22; offset_pixel < DAY28; offset_pixel++)
  {
    if (bitRead(hr, (offset_pixel - DAY22)))
    {
      leds[offset_pixel] = CHSV(130, 255, 128);
    }
  }
  for (int offset_pixel = DAY29; offset_pixel < DAY35; offset_pixel++)
  {
    if (bitRead(min, (offset_pixel - DAY29)))
    {
      leds[offset_pixel] = CHSV(172, 255, 128);
    }
  }
  for (int offset_pixel = DAY36; offset_pixel < DAY42; offset_pixel++)
  {
    if (bitRead(sec, (offset_pixel - DAY36)))
    {
      leds[offset_pixel] = CHSV(214, 255, 128);
    }
  }

  leds[DAY42] = breathBetweenToColors(CHSV(0, 255, 64), CHSV(255, 255, 255), 5);
}

void modeProgress()
{
  // Show the presentage of the day that has passed.
  // Get the current time in seconds
  time_t now = timeClient.getEpochTime();

  for (int offset_pixel = DAY1; offset_pixel < DAY32; offset_pixel++)
  {
    if (bitRead(now, (offset_pixel - DAY1)))
    {
      leds[offset_pixel] = CHSV((DAY32 / 255) * offset_pixel, 255, 128);
    }
    else
    {
      leds[offset_pixel] = COLOR_OFF;
    }
  }

  // uint8_t hr = hour(now);
  // uint8_t min = minute(now);
  // uint8_t sec = second(now);

  // // Set all the LEDS to black.
  // for (int offset_pixel = 0; offset_pixel < PIXELS_COUNT; offset_pixel++)
  // {
  //   leds[offset_pixel] = COLOR_OFF;
  // }

  // // Get the total number of seconds in the day
  // uint32_t totalSeconds = 24 * 60 * 60;
  // // Get the number of seconds that have passed
  // uint32_t secondsPassed = (hr * 60 * 60) + (min * 60) + sec;

  // // Get the percentage of the day that has passed
  // float percentage = (float)secondsPassed / (float)totalSeconds;

  // // Get the number of pixels that should be lit
  // uint8_t pixelsLit = (uint8_t)(PIXELS_COUNT * percentage);

  // // Set the pixels that should be lit
  // for (int offset_pixel = 0; offset_pixel < pixelsLit; offset_pixel++)
  // {
  //   leds[offset_pixel] = CHSV((255 / PIXELS_COUNT) * offset_pixel, 255, 128);
  // }
}

void modeBreathing()
{
  for (int offset_pixel = 0; offset_pixel < PIXELS_COUNT; offset_pixel++)
  {
    leds[offset_pixel] = breathBetweenToColors(CHSV(PIXELS_COUNT / 255 * offset_pixel, 255, 64), CHSV(255, 255, 255), offset_pixel % 7);
  }
}

// Starting with the top left, sets leds illumiated in a progress bar.
// Once it reaches the end, it loops back with a new color.
void loadingAnimation()
{
  static uint8_t hue = 0;
  hue++;

  // // Starting with black. Loads a new LED once every few ms
  // const uint8_t LOADING_ANIMATION_SPEED = 10;

  // EVERY_N_MILLIS_I(LOADING_ANIMATION, LOADING_ANIMATION_SPEED)
  // {
  //   hue++;
  // }

  // The rest can be rainbow
  for (uint offset = DAY1; offset < PIXELS_COUNT; offset++)
  {
    leds[offset] = CHSV(hue + offset * 10, 255, 128);
  }

  // Display the version number in binary on the top row
  for (uint offset = DAY1; offset < DAY7; offset++)
  {
    leds[offset] = bitRead(BUILD_NUMBER, offset) ? CRGB::Red : CRGB::Black;
  }

  FastLED.show();
}

void animationNoWifi()
{
  static uint8_t hue = 0;
  EVERY_N_MILLIS_I(HUE, 20)
  {
    hue++;
  }

  EVERY_N_SECONDS_I(ANIMATION_NO_WIFI, 60)
  {
    ScrollText("No wifi", COLOR_FAIL);
  }

  // The rest can be rainbow
  for (uint offset = DAY1; offset < PIXELS_COUNT; offset++)
  {
    leds[offset] = CHSV(hue + offset * 10, 255, 128);
  }
  FastLED.show();
}

void ShowGlyphSearchingWiFi()
{
  for (uint offset = DAY1; offset < PIXELS_COUNT; offset++)
  {
    leds[offset] = COLOR_OFF;
  }

  // Display the glyph for searching for wifi
  leds[DAY36] = CHSV(0, 255, 128);

  leds[DAY29] = CHSV(40, 255, 128);
  leds[DAY30] = CHSV(40, 255, 128);
  leds[DAY37] = CHSV(40, 255, 128);

  leds[DAY22] = CHSV(80, 255, 128);
  leds[DAY23] = CHSV(80, 255, 128);
  leds[DAY24] = CHSV(80, 255, 128);
  leds[DAY31] = CHSV(80, 255, 128);
  leds[DAY38] = CHSV(80, 255, 128);

  leds[DAY15] = CHSV(120, 255, 128);
  leds[DAY16] = CHSV(120, 255, 128);
  leds[DAY17] = CHSV(120, 255, 128);
  leds[DAY18] = CHSV(120, 255, 128);
  leds[DAY25] = CHSV(120, 255, 128);
  leds[DAY32] = CHSV(120, 255, 128);
  leds[DAY39] = CHSV(120, 255, 128);

  FastLED.show();
}

void SetAllLEDs(CHSV color)
{
  for (int i = 0; i < PIXELS_COUNT; i++)
  {
    leds[i] = color;
  }
  FastLED.show();
}

uint16_t XY(uint8_t x, uint8_t y)
{
  if (x >= LED_MATRIX_WIDTH)
    return -1;
  if (y >= LED_MATRIX_HEIGHT)
    return -1;

  return (y * LED_MATRIX_WIDTH) + x;
}

// xOffset - Where in the frame to start drawing the glyph from the left. 0 = start at the left
// xGlyphOffset - clips the glyph from the left to right. 0 = no clipping
void ShowGlyph(const uint8_t *glyph, CHSV color, uint8_t xOffset /* = 0*/, uint8_t xGlyphOffset /*= 0 */)
{
  for (uint8_t y = 0; y < FONT_HEIGHT; y++)
  {
    for (uint8_t x = xGlyphOffset; x < FONT_WIDTH; x++)
    {
      // Align the glyph to the bottom of the display.
      uint16_t ledOffset = XY(xOffset + x - xGlyphOffset, (LED_MATRIX_HEIGHT - FONT_HEIGHT) + y);
      if (ledOffset == -1)
        continue; // out of bounds

      if (glyph[y] & (1 << (FONT_WIDTH - x)))
      {
        leds[ledOffset] = color;
      }
      else
      {
        leds[ledOffset] = COLOR_OFF;
      }
    }
  }
  FastLED.show();
}

const uint8_t *GetGlyph(char c)
{
  c = tolower(c);

  uint8_t offset = 0;

  // Serial.print('[');
  // Serial.print(c);
  // Serial.print("] =");

  if (c >= 'a' && c <= 'z')
  {
    offset = (int)c - 'a' + 1;
    // Serial.println("letters: " + (String)offset + ", ");
    return FONT_LETTERS[offset];
  }
  else if (c >= '0' && c <= '9')
  {
    offset = (int)c - '0';
    // Serial.println("numbers: " + (String)offset + ", ");
    return FONT_NUMBERS[offset];
  }
  else if (c == '.')
  {
    // Serial.println("numbers: 10, ");
    return FONT_NUMBERS[10];
  }
  else if (c == ' ')
  {
    // Serial.println("letters: 0, ");
    return FONT_LETTERS[0];
  }
  else
  {
    // Serial.println(" unknown using letters: 0, ");
    return FONT_LETTERS[0];
  }
}

void ScrollText(String text, CHSV color, uint16_t delayMs /* = 100 */)
{
  // Always add a space to the end so that the text scrolls off the end of the display
  text = " " + text + " ";

  // Turn off all the LEDs first.
  SetAllLEDs(COLOR_OFF);

  // Scroll thought the text one letter at a time, then scroll the next letter in
  // and scroll the first letter out. Repeat until the end of the text.
  // -1 for the carried return.
  for (int offset = 0; offset < (text.length() - 1) * LED_MATRIX_WIDTH; offset++)
  {
    ShowGlyph(GetGlyph(text[(offset / LED_MATRIX_WIDTH)]), color, 0, offset % LED_MATRIX_WIDTH);

    // Only show the 2nd letter if there is a 2nd letter to show.
    if (offset / LED_MATRIX_WIDTH < text.length())
    {
      ShowGlyph(GetGlyph(text[(offset / LED_MATRIX_WIDTH + 1)]), color, LED_MATRIX_WIDTH - offset % LED_MATRIX_WIDTH, 0);
    }

    delay(delayMs);
    FastLED.show();
  }
}
