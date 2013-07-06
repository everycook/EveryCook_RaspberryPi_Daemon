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



void resetValues();
void ProcessCommand(void);
void prepareState(char* TotalUpdate);
void writeStatus(char* data);
void writeLog();
void parseSockInput(char* input);
bool ReadFile();
void evaluateInput();
bool readConfigLine(char* keyString, char* valueString, FILE *fp);
void ReadConfigurationFile(void);
void ReadCalibrationFile(void);


void OptionControl();
void TempControl();
void PressControl();
void MotorControl();
void ValveControl();
void ScaleFunction();
void setFreqTimer();
void FreqHandler(); 
void SegmentDisplay();
void Beep();
