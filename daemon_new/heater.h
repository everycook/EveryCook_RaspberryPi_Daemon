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
 * @brief      This class control the heater 
 * @author     Arthur Bouché
 */
#ifndef HEATER_H_
#define HEATER_H_

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <wiringPi.h>
#include "virtualspiAtmel.h"
#include "daemon.h"
#include "basic_functions.h"

struct Heater_Leds_Values {
	bool hasPower;
	bool isOn;
	bool isModeHeating;
	bool isModeKeepwarm;
	uint8_t level;
	uint32_t hasPowerLedOnLastTime;
	uint32_t hasPowerLedOffLastTime;
	uint32_t isOnLastTime;
	
	bool hasError;
	bool noPanError;
	bool IGBTTempToHeightError;
	bool tempSensorError;
	bool IGBTSensorError;
	bool voltageToHeightError;
	bool voltageToLowError;
	bool bowOutOfWaterError;
	uint32_t errorLastTime;
	
	bool lastMultiplexer;
	bool lastDiffMultiplexer;
	bool ledsOffBlinkState;
	
	uint32_t multiplexerLastTime1;
	
	char* errorMsg;
	uint32_t ledValues[6];
};

/** @brief turn heat on
 *	@param *dv : Daemon_Values
 *  @return succed?
*/
bool heaterOn();

/** @brief turn heat off
 *	@param *dv : Daemon_Values
 *  @return succed?
*/
bool heaterOff();

/** @brief update time of heater
*/
void heaterUpdateTime();

/** start the thread that read leds
 */
void heaterStartThreadLedReader();
/** stop the thread that read leds
 */
void heaterStopThreadLedReader();

void heaterTestHeatLed();

uint8_t heaterAtmelGetHeatingOutputLevel();

// Set function
void heaterSetStatusErrorMsg(char* msg);
void heaterSetStatusIsOn(bool ison);
void heaterSetStatusHasError(bool error);
void heaterSetPowerStatus(bool status);
void heaterSetStatusIsOnLastTime(uint32_t lasttime);
void heaterSetStatusHasPower(bool haspower); 
void heaterSetStatusHasPowerLedOnLastTime(uint32_t lastime);
void heaterSetStatusNoPanError(bool paserror);
void heaterSetHourCounter(uint32_t hourcounter);
void heaterSetStatusLedValuesI(uint8_t i,uint32_t ledvalues);
void heaterSetPWMTrue(uint8_t pwm);
void heaterSetTempTrans(uint8_t tempTrans);
// Get function
char* heaterGetStatusErrorMsg();
bool heaterGetStatusIsOn();
bool heaterGetStatusIsOnLastTime();
bool heaterGetStatusHasPower();
bool heaterGetStatusHasPowerLedOnLastTime();
bool heaterGetStatusNoPanError();
uint32_t heaterGetStatusLedValuesI(uint8_t i);
bool heaterGetPowerStatus();
uint32_t heaterGetHourCounter();
uint8_t heaterGetPWMTrue();
uint8_t heaterGetTempTrans();
#endif /*----HEATER_H_-----*/
