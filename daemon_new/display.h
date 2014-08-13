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

#define I2C_7SEG_TOP			4	//SegAPin
#define I2C_7SEG_TOP_LEFT		3	//SegBPin
#define I2C_7SEG_TOP_RIGHT		5	//SegFPin
#define I2C_7SEG_CENTER			2	//SegGPin
#define I2C_7SEG_BOTTOM_LEFT	6	//SegEPin
#define I2C_7SEG_BOTTOM_RIGHT	8	//SegCPin
#define I2C_7SEG_BOTTOM			7	//SegDPin
#define I2C_7SEG_PERIOD			9	//SegDPPin

#define I2C_7SEG_ON				0
#define I2C_7SEG_OFF			4095

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "virtualspiAtmel.h"
#include "daemon_structs.h"
#include "basic_functions.h"
#include "daemon.h"



/** @brief clear the display
 */
void displayClear();
/** @brief make shine all the display
 */
void displayFill();
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
/** @brief blink one segment after one
 *  @param *i2c_config : configuration of I2C
*/
void blink7Segment(struct I2C_Config *i2c_config);
/** @brief display a char on 7 segment
 *  @param curSegmentDisplay : char to display
 *  @param state : state of 7 seg
 *  @param i2c_config
*/
void SegmentDisplaySimple(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config);
void SegmentDisplayOptimized(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config);

uint8_t displayGetI2c_7seg_top();			
uint8_t displayGetI2c_7seg_top_left();		
uint8_t displayGetI2c_7seg_top_right();		
uint8_t displayGetI2c_7seg_center();		
uint8_t displayGetI2c_7seg_bottom_left();	
uint8_t displayGetI2c_7seg_bottom_right();	
uint8_t displayGetI2c_7seg_bottom();		
uint8_t displayGetI2c_7seg_period();
uint8_t displayGetPercentToShow();	
char* displayGetTextToShow();
uint16_t* displayGetPictureToShow();
	
void displaySetI2c_7seg_top(uint8_t set);
void displaySetI2c_7seg_top_left(uint8_t set);
void displaySetI2c_7seg_top_right(uint8_t	set);
void displaySetI2c_7seg_center(uint8_t set);
void displaySetI2c_7seg_bottom_left(uint8_t set);
void displaySetI2c_7seg_bottom_right(uint8_t set);	
void displaySetI2c_7seg_bottom(uint8_t set );
void displaySetI2c_7seg_period(uint8_t set);

void displaySetTop(bool shine);
void displaySetTopLeft(bool shine);
void displaySetTopRight(bool shine);
void displaySetCenter(bool shine);
void displaySetBottomLeft(bool shine);
void displaySetBottomRight(bool shine);
void displaySetBottom(bool shine);
void displaySetPeriod(bool shine);
#endif /*----DISPLAY_H_-----*/
