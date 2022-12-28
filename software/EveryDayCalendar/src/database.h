#include <Arduino.h>

// Helpers
int monthDays(uint8_t month, uint8_t year);

// Database
void loadsDatabase();
bool saveDatabase();
void resetDatabase();
void printDatabase(uint16_t year);
uint GetDayOfTheYear(uint8_t year, uint8_t month, uint8_t day);
bool DatabaseGet(uint8_t year, uint8_t month, uint8_t day, uint8_t offset);
void DatabaseSet(uint8_t year, uint8_t month, uint8_t day, uint8_t offset, bool success);

