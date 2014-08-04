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
 * @brief      This class make beep everycook  
 * @author     Arthur Bouché
 */
#ifndef BEEPER_H_
#define BEEPER_H_

#include <stdio.h>
#include <string.h>
#include "daemon.h"
#include "bool.h"
#include "modes.h"
#include <time.h>

/** @brief beep if doRememberBeep==1 (after 1 sec, 5 sec 30 sec...)
*/
void beeperBeepEndStep();

/** @brief make beep everycook during X second
 *  @param durationS : X
*/
void beeperBeepSeconde(uint32_t durationS);

// Set function
void beeperSetDoRememberBeep(bool doRememberBeep);
void beeperSetBeepEndTime(uint32_t beepEndTime);
void beeperSetIsBuzzing(bool isBuzzing);
void beeperSetSettingsBeepStepEnd(uint8_t BeepStepEnd);

// Get function
bool beeperGetDoRememberBeep();
uint32_t beeperGetBeepEndTime();
bool beeperGetIsBuzzing();
uint8_t beeperGetSettingsBeepStepEnd();

#endif /*----BEEPER_H_-----*/
