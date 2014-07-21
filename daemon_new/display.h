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
/**
 * @brief      This class control the display of everycook,
 * 				you need to use VirtualSPIAtmelInit(bool debugEnabled)
 * 				befor use functions in this class
 * @author     Arthur Bouché
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_

#define SPI_MODE_DISPLAY_TEXT				4
#define SPI_MODE_DISPLAY_TEXT_SMALL			5
#define SPI_MODE_DISPLAY_PERCENT			6
#define SPI_MODE_DISPLAY_PERCENT_TEXT		7
#define SPI_MODE_DISPLAY_CLEAR				8
#define SPI_MODE_DISPLAY_PICTURE			9


#include <stdio.h>
#include <string.h>
#include "virtualspiAtmel.h"
/** @brief clear the display
 */
void displayClear();
/** @brief show the text on the display
 *  @param text
 */
void displayShowText(char* text);
/** @brief show the percent on the display
 *  @param percent
 */
void displayShowPercent(uint8_t percent);
/** @brief show the picture on the display
 *  @param picture
 */
void displayShowPicture(uint16_t* picture);
/** @brief show the percent and text on the display
 *  @param percent
 *  @param text
 */
void displayShowPercentText(uint8_t percent, char* text);

#endif /*----DISPLAY_H_-----*/
