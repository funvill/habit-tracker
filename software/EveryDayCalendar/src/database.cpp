#include <Arduino.h>
#include "EEPROM.h"
#include "globals.h"
#include "database.h"

uint8_t database[DAYS_IN_YEAR];

uint16_t EEPROM_VERSION = 0x0001;
uint16_t EEPROM_OFFSET_VERSION = 0x0000;
uint16_t EEPROM_OFFSET_CALENDAR = 0x0002;
uint16_t EEPROM_SIZE = sizeof(uint16_t) + (sizeof(uint8_t) * DAYS_IN_YEAR);

// EPROM version (0x0001)
// ---------------------------------
//
// [EEPROM_Version]       2 BYTES  | 0x0001
// [CALENDAR]           366 BYTES  | 0x0002 - 0x0175
//
//

// Helpers
// =====================

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

// Database
// ====================

// loads the database from the EEPROM into memory
void loadsDatabase()
{
  // Sets the current database to empty (0xFF)
  for (int i = 0; i < DAYS_IN_YEAR; i++)
  {
    database[i] = 0; // OFF
  }

  Serial.println("Loading database from EEPROM");
  // Load the database from EEPROM
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

  uint16_t version = 0;
  EEPROM.get(EEPROM_OFFSET_VERSION, version);
  if (version != EEPROM_VERSION)
  {
    Serial.println("EEPROM version does not match, resetting the database");
    resetDatabase();
    return;
  }

  EEPROM.get(EEPROM_OFFSET_CALENDAR, database);
  EEPROM.end();
}

void resetDatabase()
{
  Serial.println("Reseting the database, and saving it to EEPROM");
  for (int i = 0; i < DAYS_IN_YEAR; i++)
  {
    database[i] = 0; // OFF
  }
  saveDatabase();
}

bool saveDatabase()
{
  Serial.println("Saving database to EEPROM");

  // Save the database to EEPROM
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(EEPROM_OFFSET_VERSION, EEPROM_VERSION);
  EEPROM.put(EEPROM_OFFSET_CALENDAR, database);
  EEPROM.commit();
  EEPROM.end();
  return true;
}

void printDatabase(uint16_t year)
{
  Serial.println("Print Database: ");
  for (int offset = 0; offset < DAYS_IN_YEAR; offset++)
  {
    if (database[offset] > 0)
    {
      // Base on the day of the year, print the month and day
      uint8_t month = 1;
      uint16_t dayOfTheYear = offset;
      while (dayOfTheYear > monthDays(month, year))
      {
        dayOfTheYear -= monthDays(month, year);
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

  uint8_t before = database[dayOfTheYear];
  database[dayOfTheYear] = bitWrite(database[dayOfTheYear], offset, success);

  // Check to see if the value has changed
  if (before != database[dayOfTheYear])
  {
    // The value has changed, save the database
    saveDatabase();
  }

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

uint8_t DatabaseGetOffsetRaw(uint16_t dayOffset) {
  if( dayOffset > DAYS_IN_YEAR ) {
    return 0;
  }
  return database[dayOffset];
}

uint8_t DatabaseGetRaw(uint8_t year, uint8_t month, uint8_t day)
{
  uint dayOfTheYear = GetDayOfTheYear(year, month, day);
  return database[dayOfTheYear];
}
bool DatabaseGet(uint8_t year, uint8_t month, uint8_t day, uint8_t offset)
{
  return bitRead(DatabaseGetRaw(year, month, day), offset);
}

// ====================