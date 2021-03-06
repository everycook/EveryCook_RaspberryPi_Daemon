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


/* Header of the firmware.
 *
 */

#include "bool.h"

/** @brief reset all value
 */
void resetValues();

/** @brief controle temp,press,motor,valve,beep, display
*/
void ProcessCommand(void);

/** @brief put all value in char TotalUpdate with the good format
 *  @param * TotalUpdate : char witch will have
 */
void prepareState(char* TotalUpdate);

/** @brief write data in settings.statusFile
 *  @param * data
*/
void writeStatus(char* data);

/** @ brief write on file settings.logFile and settings.hourCounterFile
*/
void writeLog();

bool ReadFile();

/** @brief read one ligne of the file fp
 *  @param keyString : name of value
 *  @param valueString : value
 *  @param fp : file to read
 *  @return true line was read or false in a other case
 */
bool readConfigLine(char* keyString, char* valueString, FILE *fp);

/** @brief Read the configuration
 */
void ReadConfigurationFile(void);

/** @brief Read the calibration
 */
void ReadCalibrationFile(void);

/** @brief control weight
*/
void ScaleFunction(); 

/** @brief beep if doRememberBeep==1 (after 1 sec, 5 sec 30 sec...)
*/
void Beep();
