#ifndef DisplayHander_h
#define DisplayHander_h

#include <inttypes.h>
//#include <arduino.h>

#define PIXEL_ON 7
#define PIXEL_HALF 2
#define PIXEL_OFF 0

//namespace DisplayHandler
//{
  extern void DisplayHandler_setPicture(uint16_t picture[9]);
  extern void DisplayHandler_setText(char *newTextToShow);
  extern void DisplayHandler_clearPercentTextArea();
  extern void DisplayHandler_displayProgress(uint8_t percent, boolean noFlip);
  extern void DisplayHandler_DisplayBitMap();
  extern boolean DisplayHandler_readText();
  extern boolean DisplayHandler_displayText(boolean small);
  extern boolean DisplayHandler_displaySmallText(const char *textToShow);
  extern boolean DisplayHandler_displayBigText(const char *text);
//}

#endif
