#include <Arduino.h>
#include "FastLED.h"
#include "globals.h"


void SetAllLEDs(CHSV color = COLOR_OFF);
void animationNoWifi();
void loadingAnimation();
void ShowGlyphSearchingWiFi();
void ShowGlyphUpdating();
void ShowGlyph(const uint8_t *glyph, CHSV color = COLOR_SUCCESS, uint8_t xOffset = 0, uint8_t xGlyphOffset = 0);
const uint8_t *GetGlyph(char c);
void ScrollText(String text, CHSV color  = COLOR_FONT, uint16_t delayMs  = 100);
uint16_t XY(uint8_t x, uint8_t y);
CRGB breathBetweenToColors(CHSV start, CHSV end, float pulseSpeed = 2.0);