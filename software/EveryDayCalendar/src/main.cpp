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

#include <time.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h> // DNS server used for captive portal

#include "EEPROM.h"

#include "globals.h"
#include "font.h"
#include "database.h"
#include "animation.h"
#include "http.h"

// Modes
#include "modes/fadepoints.h"
#include "modes/clocks.h"

CRGB leds[PIXELS_COUNT];

const char *NTP_SERVER = "pool.ntp.org";

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

#include <WiFi.h>
#include <ESPConnect.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h" // Used for displaying webpages
#include <AsyncElegantOTA.h>   // Used for Update via webpage. https://github.com/ayushsharma82/AsyncElegantOTA

// Set up the web server.
AsyncWebServer webServer(HTTP_PORT);
DNSServer dnsServer;

void setupServer()
{
  webServer.on("/", HTTP_GET, httpIndex);
  webServer.on("/database", HTTP_GET, httpDatabase);
  webServer.on("/set", HTTP_POST, httpSet);
  webServer.on("/reset", HTTP_GET, httpReset);
  webServer.on("/resetwifi", HTTP_GET, httpResetWifi);
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
  ESPConnect.autoConnect(APSSID.c_str());
  if (ESPConnect.begin(&webServer))
  {
    Serial.println("Connected to WiFi");
    Serial.println("IPAddress: " + WiFi.localIP().toString());
    delay(100); // Give the TCP stack a chance to connect to the network
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
  gMode = MODE_START;

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
  case MODE_BINARY_CLOCK:
    modeBinaryClock(timeClient.getEpochTime() + utcOffsetInSeconds);
    break;
  case MODE_EPOCH_CLOCK:
    modeEPOCHClock(timeClient.getEpochTime() + utcOffsetInSeconds);
    break;
  case MODE_BREATHING:
    modeBreathing();
    break;
  case MODE_FADE_POINTS:
    modeFadePoints();
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
    case MODE_BINARY_CLOCK:
      ScrollText("T");
      break;
    case MODE_EPOCH_CLOCK:
      ScrollText("E");
      break;
    case MODE_BREATHING:
      ScrollText("B");
      break;
    case MODE_FADE_POINTS:
      ScrollText("F");
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

void modeBreathing()
{
  for (int offset_pixel = 0; offset_pixel < PIXELS_COUNT; offset_pixel++)
  {
    leds[offset_pixel] = breathBetweenToColors(CHSV(PIXELS_COUNT / 255 * offset_pixel, 255, 64), CHSV(255, 255, 255), offset_pixel % 7);
  }
}
