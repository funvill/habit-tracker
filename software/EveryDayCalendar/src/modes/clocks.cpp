#include "clocks.h"
#include <Arduino.h>
#include "FastLED.h" // Required for CHSV
#include "globals.h"

#include <TimeLib.h>
#include "animation.h"

void modeBinaryClock(time_t now)
{
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
void modeEPOCHClock(time_t now)
{
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
}
