#include <Arduino.h>

// Helpers
int monthDays(uint8_t month, uint16_t year);

// Database
void loadsDatabase();
bool saveDatabase();
void resetDatabase();
void printDatabase(uint16_t year);
uint GetDayOfTheYear(uint16_t year, uint8_t month, uint8_t day);
bool DatabaseGet(uint16_t year, uint8_t month, uint8_t day, uint8_t offset);
uint8_t DatabaseGetOffsetRaw(uint16_t dayOffset);
uint8_t DatabaseGetRaw(uint16_t year, uint8_t month, uint8_t day);
void DatabaseSet(uint16_t year, uint8_t month, uint8_t day, uint8_t offset, bool success);
