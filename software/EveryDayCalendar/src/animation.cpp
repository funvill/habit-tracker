#include "animation.h"
#include "globals.h"
#include "font.h"

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
    ScrollText("No wifi", COLOR_FAIL, 100);
  }

  // The rest can be rainbow
  for (uint offset = DAY1; offset < PIXELS_COUNT; offset++)
  {
    leds[offset] = CHSV(hue + offset * 10, 255, 128);
  }
  FastLED.show();
}

void ShowGlyphUpdating() {
  for (uint offset = DAY1; offset < PIXELS_COUNT; offset++)
  {
    leds[offset] = CHSV(0, 255, 128); // red 
  }

  leds[DAY1] = CHSV(0, 255, 0); // Green 
  leds[DAY7] = CHSV(0, 255, 0); // Green 
  leds[DAY36] = CHSV(0, 255, 0); // Green 
  leds[DAY42] = CHSV(0, 255, 0); // Green 

  // Display the version number in binary on the top row
  for (uint offset = DAY1; offset < DAY7; offset++)
  {
    leds[offset] = bitRead(BUILD_NUMBER, offset) ? CRGB::Red : CRGB::Black;
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

  // Display the version number in binary on the top row
  for (uint offset = DAY1; offset < DAY7; offset++)
  {
    leds[offset] = bitRead(BUILD_NUMBER, offset) ? CRGB::Red : CRGB::Black;
  }

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



// https://github.com/marmilicious/FastLED_examples/blob/master/breath_effect_v2.ino
CRGB breathBetweenToColors(CHSV start, CHSV end, float pulseSpeed /*= 2.0*/)
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