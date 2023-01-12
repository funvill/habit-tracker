#include "fadepoints.h"
#include <Arduino.h>
#include "FastLED.h" // Required for CHSV
#include "globals.h"

const uint16_t FADE_SPEED = 70; // Lower the faster they fade away
const uint16_t PIXLE_CREATION_SPEED = 300; // Lower the more pixles are created

void modeFadePoints()
{

  EVERY_N_MILLIS_I(FADE_POINTS, FADE_SPEED)
  {
    // Fade all pixels by 1/8th
    for (uint8_t i = 0; i < PIXELS_COUNT; i++)
    {
      leds[i].nscale8(248);
    }
  }

  EVERY_N_MILLIS_I(CREATE_POINT, PIXLE_CREATION_SPEED)
  {
    // Attempt to create a new point on a dark pixel
    // Find a dark pixel
    uint8_t x = random8(0, PIXELS_COUNT);
    // We'll only look for a dark pixel 10 times before giving up
    for (uint8_t i = 0; i < 10; i++)
    {
      if (leds[x].r == 0 && leds[x].g == 0 && leds[x].b == 0)
      {
        // We found a dark pixel, so we can stop looking
        break;
      }
      // We didn't find a dark pixel, so try again
      x = random8(0, PIXELS_COUNT);
    }
    if (leds[x].r != 0 && leds[x].g != 0 && leds[x].b != 0)
    {
      // We didn't find a dark pixel, so we'll just have to wait until the next time
      // we're called.
      return;
    }

    // illiminate the pixel to max brightness with a random color
    leds[x] = CHSV(random8(), 255, 255);

    return;
  }
}