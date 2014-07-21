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
void displayClear(){
	SPIAtmelWrite(SPI_MODE_DISPLAY_CLEAR);
	getValidResultOrReset();
}

void displayShowText(char* text){
	if (atmelGetDebug2()) {printf("--->atmelShowText\n");}
	SPIAtmelWrite(SPI_MODE_DISPLAY_TEXT);
	uint8_t len = strlen(text);
	SPIAtmelWrite(len);
	uint8_t i;
	for(i=0;i<len;i++){
		SPIAtmelWrite(text[i]);
	}
	SPIAtmelWrite(0x00);
	getValidResultOrResetAdditionalValid(SPI_Error_Text_Invalid);
}

void displayShowPercent(uint8_t percent){
	if (atmelGetDebug2()) {printf("--->atmelShowPercent\n");}
	SPIAtmelWrite(SPI_MODE_DISPLAY_PERCENT);
	SPIAtmelWrite(percent);
	getValidResultOrReset();
}

void displayShowPicture(uint16_t* picture){
	if (atmelGetDebug2()) {printf("--->atmelShowPicture\n");}
	SPIAtmelWrite(SPI_MODE_DISPLAY_PICTURE);
	uint8_t i=0;
	for(; i<9;++i){
		SPIAtmelWrite((picture[i] >> 8) & 0xFF);
		SPIAtmelWrite(picture[i] & 0xFF);
	}
	SPIAtmelWrite(0x00);
	getValidResultOrResetAdditionalValid(SPI_Error_Picture_Invalid);
}

void displaySPI_Error_Picture_InvalidShowPercentText(uint8_t percent, char* text){
	if (atmelGetDebug2()) {printf("--->atmelShowPercentText\n");}
	SPIAtmelWrite(SPI_MODE_DISPLAY_PERCENT_TEXT);
	SPIAtmelWrite(percent);
	uint8_t len = strlen(text);
	SPIAtmelWrite(len);
	uint8_t i;
	for(i=0;i<len;i++){
		SPIAtmelWrite(text[i]);
	}
	SPIAtmelWrite(0x00);
	getValidResultOrResetAdditionalValid(SPI_Error_Text_Invalid);
}
