#include <Arduino.h>
#include "http.h"
#include "globals.h"
#include "database.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPConnect.h>

#include <TimeLib.h>

void httpIndex(AsyncWebServerRequest *request)
{
  String html = "<!DOCTYPE html><html><head><title>EveryDayCalendar</title></head><body><h1>Every Day Calendar</h1>";
  html += "<p>Version " + String(BUILD_NUMBER) + "</p>";
  html += "<p><a href='/update'>Firmware Update</a> - <a href='/database'>Download Database</a> - <a href='/reset'>clear settings and data</a> - <a href='/resetwifi'>Wifi Reset</a></p>";

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
  // html += "<p><strong>NTP Server</strong>: " + String(NTP_SERVER) + "</p>";
  // html += "<p><strong>Current Epoch Time</strong>: " + String(timeClient.getEpochTime()) + "</p>";

  html += "<h2>Display</h2>";
  html += "<p><strong>Mode</strong>: " + String(gMode) + "</p>";
  html += "<p><strong>gCurrentYear</strong>: " + String(gCurrentYear) + "</p>";
  html += "<p><strong>gCurrentMonth</strong>: " + String(gCurrentMonth) + " (" + (String)MONTHS_SHORT[gCurrentMonth] + ")</p>";

  // int daysInMonth = monthDays(gCurrentMonth, gCurrentYear);
  // int dayOfWeek = weekday(GetEpochForDate(gCurrentYear, gCurrentMonth, 1)) - 1;
  // html += "<p><strong>daysInMonth</strong>: " + String(daysInMonth) + "</p>";
  // html += "<p><strong>dayOfWeek</strong>: " + String(dayOfWeek) + " (" + DAY_SHORT[dayOfWeek] + ")</p>";

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

void httpResetWifi(AsyncWebServerRequest *request)
{
  ESPConnect.erase();

  String html = "<!DOCTYPE html><html><head><title>EveryDayCalendar</title></head><body><h1>Every Day Calendar - Reset Wifi</h1>";
  html += "<p><a href='/'>Home</a></p>";
  html += "<strong>httpResetWifi</strong>";

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
