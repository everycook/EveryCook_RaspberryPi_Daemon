#include "Charliplexing.h" //Imports the library, which needs to be Initialized in setup.
#include "DisplayHandler.h"
#include "FontHandler.h"
#include "NormalFont.h"
#include "SmallFont.h"
#include "mytypes.h"
#include "communication.h"

#include <inttypes.h>
#include <util/delay.h>
#include <avr/wdt.h>


//#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>
#include <math.h>
#include <string.h>

//DISPLAY_COLS x DISPLAY_ROWS
const uint8_t PERCENT_FIRST_ROW = 6; //0-8
const uint8_t PERCENT_LAST_ROW = 8; //0-8
const uint8_t PERCENT_100_PERCENT_COL = 10;

const uint8_t PERCENT_TEXT_NONE = 0;
const uint8_t PERCENT_TEXT_STOP = 1;
const uint8_t PERCENT_TEXT_TO_MUTCH = 2;



char *textToShow = "EveryCook is starting";
int8_t textXCord = DISPLAY_COLS;
int8_t textYCord = 0;
int8_t textCharPos = 0;

uint8_t lastPercentCols = 0;
boolean lastHalfLed = false;
uint8_t lastPercentText = 0;

uint16_t pictureToShow[9];



void DisplayHandler_setPicture(uint16_t* picture){
	uint8_t i;
	for(i=0; i<9; i++){
		pictureToShow[i]=picture[i];
	}
}

void DisplayHandler_setText(char *newTextToShow){
  textToShow = newTextToShow;
  textXCord = DISPLAY_COLS;
  textYCord = 0;
  textCharPos = 0;
}

void DisplayHandler_setText2(char *newTextToShow){
  textToShow = newTextToShow;
  textXCord = 0;
  textYCord = 0;
  textCharPos = 0;
}

void DisplayHandler_clearPercentTextArea(){
	uint8_t y=0;
  for (; y<5; y++){
     LedSign_Horizontal(y, PIXEL_OFF);
  }
}

/*displayProgress logic:
- we have 14 led's
- you see 100% indicator on the 12th col from beginning (4 rows)
- the 100% indicator will fade out, if you came near the 100% ()
- percentage are NOT linear:
  - 0-80 are 8 leds, linear (10% each led) 8
  - 80-88 is 1 led    84      (8% each led)  9
  - 88-95 is 1 led    91      (7% each led)  10
  - 95-100 is 1 led   98      (5% each led)  11
  - 100-105 is 1 led  103      (5% each led)  12
  - 105-112 is 1 led  110      (7% each led)  13
  - 112-120 is 1 led  116      (8% each led)  14
*/
void DisplayHandler_displayProgress(uint8_t percent, boolean noFlip){
  boolean changed = false;
  boolean halfLed = false;
  uint8_t percentCols = 0; //1-14
  uint8_t percentGrayScale100 = 0; //0-7
//  uint8_t percent10 = floor(percent/10.0); //0-7 //debug

  //calc leds
  if (percent<80){
    double percentColsDouble = (10.0 * (unsigned int)percent) / 100.0; //linear: 10 leds if there are 100%
    percentCols = floor(percentColsDouble); //(uint8_t)percentColsDouble;
    halfLed = (percentColsDouble - percentCols) > 0.5;

    if (percentCols>7){
      percentGrayScale100 = 0;
//    } else if (percentCols<2) {
//      percentGrayScale100 = 7;
    } else {
      percentGrayScale100 = 7-percentCols;
    }
  } else if (percent<84){
    percentCols = 8;
    halfLed = false;
  } else if (percent<88){
    percentCols = 8;
    halfLed = true;
  } else if (percent<91){
    percentCols = 9;
    halfLed = false;
  } else if (percent<95){
    percentCols = 9;
    halfLed = true;
  } else if (percent<98){
    percentCols = 10;
    halfLed = false;
  } else if (percent<100){
    percentCols = 10;
    halfLed = true;
  } else if (percent<103){
    percentCols = 11;
    halfLed = false;
  } else if (percent<105){
    percentCols = 11;
    halfLed = true;
  } else if (percent<120){
    percentCols = 12;
    halfLed = false;
  } else if (percent<112){
    percentCols = 12;
    halfLed = true;
  } else if (percent<116){
    percentCols = 13;
    halfLed = false;
  } else if (percent<120){
    percentCols = 13;
    halfLed = true;
  } else {
    percentCols = 14;
    halfLed = false;
  }

  if (percentCols != lastPercentCols || halfLed != lastHalfLed){
    changed = true;
  }

  //Show Text
  if (percent >= 98 && percent < 105){
    //STOP
    if (lastPercentText != PERCENT_TEXT_STOP || changed){
      lastPercentText = PERCENT_TEXT_STOP;
      changed = true;
      DisplayHandler_clearPercentTextArea();
      FontHandler_Draw(0, 0, SmallFont_getChar('S'), PIXEL_ON);
      FontHandler_Draw(4, 0, SmallFont_getChar('T'), PIXEL_ON);
      FontHandler_Draw(7, 0, SmallFont_getChar('O'), PIXEL_ON);
      FontHandler_Draw(11, 0, SmallFont_getChar('P'), PIXEL_ON);
    }

  } else if (percent >= 105){
    //to mutch
    if (lastPercentText != PERCENT_TEXT_TO_MUTCH || changed){
      lastPercentText = PERCENT_TEXT_TO_MUTCH;
      changed = true;
      DisplayHandler_clearPercentTextArea();
      //TODO show text
    }
  } else {
    //clear
    if (lastPercentText != PERCENT_TEXT_NONE || changed){
      lastPercentText = PERCENT_TEXT_NONE;
      changed = true;
      DisplayHandler_clearPercentTextArea();
    }
  }

  //Show progressbar
  if (changed){
    if (percentCols>0){
	  uint8_t x=0;
      for (; x<percentCols; x++){
	  uint8_t y=PERCENT_FIRST_ROW;
        for (; y<=PERCENT_LAST_ROW; y++){
          LedSign_Set(x,y, PIXEL_ON);
        }
      }
    }
    if (halfLed){
	uint8_t y=PERCENT_FIRST_ROW;
      for (; y<=PERCENT_LAST_ROW; y++){
        LedSign_Set(percentCols,y, PIXEL_HALF);
      }
      percentCols++;
    }
    if (percentCols<14){
	uint8_t x=percentCols+1;
      for (; x<DISPLAY_COLS; x++){
	  uint8_t y=PERCENT_FIRST_ROW;
        for (; y<=PERCENT_LAST_ROW; y++){
          LedSign_Set(x,y, PIXEL_OFF);
        }
      }
    }
  }
  LedSign_Horizontal(5, PIXEL_OFF);
  if (percentCols<PERCENT_100_PERCENT_COL){
    LedSign_Set(PERCENT_100_PERCENT_COL,6, percentGrayScale100);
    LedSign_Set(PERCENT_100_PERCENT_COL,7, percentGrayScale100);
    LedSign_Set(PERCENT_100_PERCENT_COL,8, percentGrayScale100);
  }
  LedSign_Set(PERCENT_100_PERCENT_COL,5, PIXEL_ON);
/* //debug
  for(int i=0;i<percent10;i++){
    LedSign_Set(i,5, 7);
  }
*/
  if (changed && !noFlip){
    LedSign_Flip(true);
  }
}

void DisplayHandler_DisplayBitMap()
{
  uint8_t line = 0;       //Row counter
  unsigned long data;  //Temporary storage of the row data
  for(line = 0; line < 9; line++) {
    data = pictureToShow[line];
	uint8_t led=0;
    for (; led<14; ++led) {
      //if (data & (1<<led)) { //data is reversed, last bit is first pixel
      if (data & (1<<(13-led))) { //data is right assigned, so it show the last 14 bits of the word
        LedSign_Set(led, line, PIXEL_ON);
      } else {
        LedSign_Set(led, line, 0);
      }
    }
  }
  LedSign_Flip(true);
}

boolean DisplayHandler_readText(){
	uint8_t textLen = 0;
	uint8_t readAmount = 0;
	textLen = readSPI(true);
	char *newText = (char *) malloc(textLen * sizeof(char) + 1);
	while(readAmount < textLen){
		newText[readAmount] = readSPI(true);
		wdt_reset();
		readAmount++;
	}
	boolean sucess = false;
	if(newText[readAmount] != 00){
		newText[readAmount] = 0x00;
/*		if(availableSPI() == 0) {
			_delay_ms(500);
		}
*/
		if(availableSPI() == 0) {
			//Serial.println("no sign after text end, it have to end with a 0x00");
			sucess = false;
		} else {
			uint8_t inByte = peekSPI();
			if (inByte != 0x00) {
				//Serial.println("text have to end with a 0x00");
				sucess = false;
			} else {
				inByte = readSPI(true);
				//Serial.println("text is OK");
				sucess = true;
			}
		}
	} else {
		//Serial.println("text end submited");
		sucess = true;
	}
//	if (sucess){
		if (textToShow != NULL){
			free(textToShow);
		}
		textToShow = newText;
		textXCord = DISPLAY_COLS;
		textYCord = 0;
		textCharPos = 0;
//	}
	return sucess;
}

boolean DisplayHandler_displayText(boolean small){
  if (textToShow == NULL){
    return false;
  }
  if (small){
    return DisplayHandler_displaySmallText(textToShow);
  } else {
    return DisplayHandler_displayBigText(textToShow);
  }
}

boolean DisplayHandler_displaySmallText(const char *text){
  int8_t x=textXCord;
  int8_t i=textCharPos;
//  LedSign_Clear();
  DisplayHandler_clearPercentTextArea();
  int8_t x2, i2;
  for (x2=x, i2=i; x2<DISPLAY_COLS;) {
    if (i2 < strlen(text)){
      const uint8_t *character = SmallFont_getChar(text[i2]);
      int8_t w = FontHandler_Draw(x2, textYCord, character, PIXEL_ON);
      x2 += w;
      i2 = (i2+1);
    } else {
      //add a "space"
      x2 += 4;
      i2 = (i2+1);
    }
    if (x2 <= 0){ // the currently drawn character was completely out of screen
      //adjust looping values for next loop
      x = x2, i = i2;
    }
  }
  LedSign_Flip(true);
  x--;
  textXCord=x;
  textCharPos=i;

  if (i == strlen(text) && x <= 0){
//    currentMode = 0;
    return false;
  } else {
    return true;
  }
}

boolean DisplayHandler_displayBigText(const char *text){
  int8_t x=textXCord;
  int8_t i=textCharPos;
  LedSign_Clear(PIXEL_OFF);
  int8_t x2, i2;
  for (x2=x, i2=i; x2<DISPLAY_COLS;) {
    if (i2 < strlen(text)){
      const uint8_t *character = NormalFont_getChar(text[i2]);
      int8_t w = FontHandler_Draw(x2, textYCord, character, PIXEL_ON);
      x2 += w;
      i2 = (i2+1);
    } else {
      //add a "space"
      x2 += 5;
      i2 = (i2+1);
    }
    if (x2 <= 0){ // the currently drawn character was completely out of screen
      //adjust looping values for next loop
      x = x2, i = i2;
    }
  }
  LedSign_Flip(true);
  x--;
  textXCord=x;
  textCharPos=i;

  if (i == strlen(text) && x <= 0){
//    currentMode = 0;
    return false;
  } else {
    return true;
  }
}


