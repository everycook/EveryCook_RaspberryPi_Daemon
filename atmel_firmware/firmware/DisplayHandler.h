#ifndef DisplayHander_h
#define DisplayHander_h

#include <inttypes.h>
//#include <arduino.h>

#define PIXEL_ON 7
#define PIXEL_HALF 3
#define PIXEL_OFF 0


//namespace DisplayHandler
//{
  extern void DisplayHandler_setPicture(uint16_t* picture);
  extern void DisplayHandler_setText(char *newTextToShow);
  extern void DisplayHandler_setText2(char *newTextToShow);
  extern void DisplayHandler_showByte(uint8_t data);
  extern void DisplayHandler_clearPercentTextArea();
  extern void DisplayHandler_displayProgress(uint8_t percent, boolean noFlip);
  extern void DisplayHandler_DisplayBitMap();
  extern boolean DisplayHandler_readText();
  extern boolean DisplayHandler_displayText(boolean small);
  extern boolean DisplayHandler_displayText2(const char *textToShow, boolean small);
//}

#endif
