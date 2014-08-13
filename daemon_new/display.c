/*
This is the EveryCook Raspberry Pi daemon. It reads inputs from the EveryCook Raspberry Pi shield and controls the outputs.
EveryCook is an open source platform for collecting all data about food and make it available to all kinds of cooking devices.

This program is copyright (C) by EveryCook. Written by Samuel Werder, Peter Turczak and Alexis Wiasmitinow.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

See GPLv3.htm in the main folder for details.
*/
#include "display.h"
	uint8_t displayI2c_7seg_top;			//SegAPin
	uint8_t displayI2c_7seg_top_left;		//SegBPin
	uint8_t displayI2c_7seg_top_right;		//SegFPin
	uint8_t displayI2c_7seg_center;		//SegGPin
	uint8_t displayI2c_7seg_bottom_left;	//SegEPin
	uint8_t displayI2c_7seg_bottom_right;	//SegCPin
	uint8_t displayI2c_7seg_bottom;		//SegDPin
	uint8_t displayI2c_7seg_period;		//SegDPPin

	uint8_t displayPercentToShow=0;
	char* displaytextToshow;
	uint16_t displayPictureToShow[9];

void displayClear(){
	virtualSpiAtmelSetNewMode(SPI_MODE_DISPLAY_CLEAR);
}

void displayFill(){
	uint16_t picture[9] = {0b0011111111111111,
							0b0011111111111111,
							0b0011111111111111,
							0b0011111111111111,
							0b0011111111111111,
							0b0011111111111111,
							0b0011111111111111,
							0b0011111111111111,
							0b0011111111111111};
	displayShowPicture(&picture[0]);					
}

void displayShowText(char* text){
	if (atmelGetDebug2()) {printf("--->atmelShowText\n");}
	displaytextToshow=text;
	virtualSpiAtmelSetNewMode(SPI_MODE_DISPLAY_TEXT);
}

void displayShowPercent(uint8_t percent){
	if (atmelGetDebug2()) {printf("--->atmelShowPercent\n");}
	displayPercentToShow=percent;
	virtualSpiAtmelSetNewMode(SPI_MODE_DISPLAY_PERCENT);
}

void displayShowPicture(uint16_t* picture){
	int i=0;
	if (atmelGetDebug2()) {printf("--->atmelShowPicture\n");}
	for(i=0; i<9;++i){
		displayPictureToShow[i]=picture[i];
	}
	virtualSpiAtmelSetNewMode(SPI_MODE_DISPLAY_PICTURE);
}

void displaySPI_Error_Picture_InvalidShowPercentText(uint8_t percent, char* text){
	if (atmelGetDebug2()) {printf("--->atmelShowPercentText\n");}
	displayPercentToShow=percent;
	strcpy(displaytextToshow,text);
	virtualSpiAtmelSetNewMode(SPI_MODE_DISPLAY_PERCENT_TEXT);
}

/** @brief display a char on 7 segment
 *  @param curSegmentDisplay : char to display
 *  @param state : state of 7 seg
 *  @param i2c_config
*/
void SegmentDisplaySimple(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config){
	if (curSegmentDisplay == 'P'){
		state->oldSegmentDisplay = 'P';
		writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
	} else if (curSegmentDisplay == 'H'){
		state->oldSegmentDisplay = 'H';
		writeI2CPin(displayI2c_7seg_top,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
	} else if (curSegmentDisplay == '0'){
		state->oldSegmentDisplay = '0';
		writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_center,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_ON);
	} else if (curSegmentDisplay == 'S'){
		state->oldSegmentDisplay = 'S';
		writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_ON);
	} else if (curSegmentDisplay == 'E'){
		state->oldSegmentDisplay = 'E';
		writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_ON);
	} else {
		state->oldSegmentDisplay = ' ';
		writeI2CPin(displayI2c_7seg_top,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_center,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
	}
}

void SegmentDisplayOptimized(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config){
	if (curSegmentDisplay == 'P'){
		if (curSegmentDisplay != state->oldSegmentDisplay){
			if (state->oldSegmentDisplay == 'H'){
				writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_OFF);
				//writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (state->oldSegmentDisplay == '0'){
				//writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_OFF);
				writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (state->oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_OFF);
				writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
			} else {
				SegmentDisplaySimple(curSegmentDisplay, state, i2c_config);
			}
			state->oldSegmentDisplay = curSegmentDisplay;
		}
	} else if (curSegmentDisplay == 'H'){
		if (curSegmentDisplay != state->oldSegmentDisplay){
			if (state->oldSegmentDisplay == 'P'){
				writeI2CPin(displayI2c_7seg_top,I2C_7SEG_OFF);
				//writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (state->oldSegmentDisplay == '0'){
				writeI2CPin(displayI2c_7seg_top,I2C_7SEG_OFF);
				//writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (state->oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(displayI2c_7seg_top,I2C_7SEG_OFF);
				writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
			} else {
				SegmentDisplaySimple(curSegmentDisplay, state, i2c_config);
			}
			state->oldSegmentDisplay = curSegmentDisplay;
		}
	} else if (curSegmentDisplay == '0'){
		if (curSegmentDisplay != state->oldSegmentDisplay){
			if (state->oldSegmentDisplay == 'H'){
				writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_center,I2C_7SEG_OFF);
				//writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_ON);
			} else if (state->oldSegmentDisplay == 'P'){
				//writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_center,I2C_7SEG_OFF);
				//writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_ON);
			} else if (state->oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_center,I2C_7SEG_OFF);
				writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_ON);
			} else {
				SegmentDisplaySimple(curSegmentDisplay, state, i2c_config);
			}
			state->oldSegmentDisplay = curSegmentDisplay;
		}
	} else {
		SegmentDisplaySimple(curSegmentDisplay, state, i2c_config);
		/*
		//Empty all
		writeI2CPin(displayI2c_7seg_top,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_center,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
		state->oldSegmentDisplay = ' ';
		*/
	}
}

void blink7Segment(struct I2C_Config *i2c_config){
	//mitte
	printf("center: %d\n", displayI2c_7seg_center);
	writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(displayI2c_7seg_center,I2C_7SEG_OFF);
	
	//links
	printf("top left: %d\n", displayI2c_7seg_top_left);
	writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_OFF);
	
	//oben
	printf("top: %d\n", displayI2c_7seg_top);
	writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(displayI2c_7seg_top,I2C_7SEG_OFF);
	
	
	//rechts
	printf("top right: %d\n", displayI2c_7seg_top_right);
	writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_OFF);
	
	//ulinks
	printf("bottom left: %d\n", displayI2c_7seg_bottom_left);
	writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_OFF);
	
	//unten
	printf("bottom: %d\n", displayI2c_7seg_bottom);
	writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
	
	//urechts
	printf("bottom right: %d\n", displayI2c_7seg_bottom_right);
	writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_OFF);
	
	//punkt
	printf("period: %d\n", displayI2c_7seg_period);
	writeI2CPin(displayI2c_7seg_period,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(displayI2c_7seg_period,I2C_7SEG_OFF);
	delay(5000);
}

uint8_t displayGetI2c_7seg_top(){
	return displayI2c_7seg_top;
}			
uint8_t displayGetI2c_7seg_top_left(){
	return displayI2c_7seg_top_left;
}					
uint8_t displayGetI2c_7seg_top_right(){
	return displayI2c_7seg_top_right;
}					
uint8_t displayGetI2c_7seg_center(){
	return displayI2c_7seg_center;	
}					
uint8_t displayGetI2c_7seg_bottom_left(){
	return displayI2c_7seg_bottom_left;
}				
uint8_t displayGetI2c_7seg_bottom_right(){
	return displayI2c_7seg_bottom_right;
}				
uint8_t displayGetI2c_7seg_bottom(){
	return displayI2c_7seg_bottom;	
}					
uint8_t displayGetI2c_7seg_period(){
	return displayI2c_7seg_period;
}			
uint8_t displayGetPercentToShow(){
	return displayPercentToShow;
}
char* displayGetTextToShow(){
	return displaytextToshow;
}
uint16_t* displayGetPictureToShow(){
	return displayPictureToShow;
}

void displaySetI2c_7seg_top(uint8_t set){
	displayI2c_7seg_top=set;
}
void displaySetI2c_7seg_top_left(uint8_t set){
	displayI2c_7seg_top_left=set;
}
void displaySetI2c_7seg_top_right(uint8_t	set){
	displayI2c_7seg_top_right=set;
}
void displaySetI2c_7seg_center(uint8_t set){
	displayI2c_7seg_center=set;
}
void displaySetI2c_7seg_bottom_left(uint8_t set){
	displayI2c_7seg_bottom_left=set;
}
void displaySetI2c_7seg_bottom_right(uint8_t set){
	displayI2c_7seg_bottom_right=set;
}
void displaySetI2c_7seg_bottom(uint8_t set ){
	displayI2c_7seg_bottom=set;
}
void displaySetI2c_7seg_period(uint8_t set){
	displayI2c_7seg_period=set;
}
	
void displaySetTop(bool shine){
	if(shine){
		writeI2CPin(displayI2c_7seg_top,I2C_7SEG_ON);
	}else{		
		writeI2CPin(displayI2c_7seg_top,I2C_7SEG_OFF);
	}			
}
void displaySetTopLeft(bool shine){
	if(shine){
		writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_ON);
	}else{		
		writeI2CPin(displayI2c_7seg_top_left,I2C_7SEG_OFF);
	}			
}
void displaySetTopRight(bool shine){
	if(shine){
		writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_ON);
	}else{		
		writeI2CPin(displayI2c_7seg_top_right,I2C_7SEG_OFF);
	}			
}
void displaySetCenter(bool shine){
	if(shine){
		writeI2CPin(displayI2c_7seg_center,I2C_7SEG_ON);
	}else{		
		writeI2CPin(displayI2c_7seg_center,I2C_7SEG_OFF);
	}			
}
void displaySetBottomLeft(bool shine){
	if(shine){
		writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_ON);
	}else{		
		writeI2CPin(displayI2c_7seg_bottom_left,I2C_7SEG_OFF);
	}			
}
void displaySetBottomRight(bool shine){
	if(shine){
		writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_ON);
	}else{		
		writeI2CPin(displayI2c_7seg_bottom_right,I2C_7SEG_OFF);
	}			
}
void displaySetBottom(bool shine){
	if(shine){
		writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_ON);
	}else{		
		writeI2CPin(displayI2c_7seg_bottom,I2C_7SEG_OFF);
	}			
}
void displaySetPeriod(bool shine){
	if(shine){
		writeI2CPin(displayI2c_7seg_period,I2C_7SEG_ON);
	}else{		
		writeI2CPin(displayI2c_7seg_period,I2C_7SEG_OFF);
	}			
}
