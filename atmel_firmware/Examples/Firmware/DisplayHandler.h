#ifndef DisplayHander_h
#define DisplayHander_h

#include <inttypes.h>
#include <arduino.h>

const uint8_t PIXEL_ON = 7;
const uint8_t PIXEL_HALF = 2;
const uint8_t PIXEL_OFF = 0;

namespace DisplayHandler
{
  extern void setPicture(uint16_t picture[9]);
  extern void setText(char *newTextToShow);
  extern void clearPercentTextArea();
  extern void displayProgress(uint8_t percent, boolean noFlip);
  extern void DisplayBitMap();
  extern boolean readText();
  extern boolean displayText(boolean small);
  extern boolean displaySmallText(const char *textToShow);
  extern boolean displayBigText(const char *text);
}

#endif
