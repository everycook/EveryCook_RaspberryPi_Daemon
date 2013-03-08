/*
This is the EveryCook Raspberry Pi deamon. It reads inputs from the EveryCook Raspberry Pi shield and controls the outputs.
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

#define MIN_COOK_MODE	10
#define MAX_COOK_MODE	29

#define MIN_TEMP_MODE	10
#define MAX_TEMP_MODE	19

#define MIN_PRESS_MODE	20
#define MAX_PRESS_MODE	29

#define MIN_STATUS_MODE	30
#define MAX_STATUS_MODE	39

#define MIN_ERROR_MODE	40
#define MAX_ERROR_MODE	49


#define MODE_STANDBY				0
#define MODE_CUT					1	//STIME, MORPM(, MOON, MOOFF)
#define MODE_SCALE					2	//W0, STIME

#define MODE_HEATUP					10 	//T0, STIME, MORPM, MOON, MOOFF
#define MODE_COOK					11 	//T0, STIME, MORPM, MOON, MOOFF
#define MODE_COOLDOWN				12 	//T0, STIME, MORPM, MOON, MOOFF

#define MODE_PRESSUP				20	//P0, STIME, MORPM, MOON, MOOFF
#define MODE_PRESSHOLD				21	//P0, STIME, MORPM, MOON, MOOFF
#define MODE_PRESSDOWN				22	//P0, STIME, MORPM, MOON, MOOFF
#define MODE_PRESSVENT				23	//P0, STIME, MORPM, MOON, MOOFF

#define MODE_HOT					30
#define MODE_PRESSURIZED			31
#define MODE_COLD					32
#define MODE_PRESSURELESS			33
#define MODE_WEIGHT_REACHED			34
#define MODE_COOK_TIMEEND			35
#define MODE_RECIPE_END				39

#define MODE_INPUT_ERROR			40
#define MODE_EMERGENCY_SHUTDOWN		41
#define MODE_MOTOR_OVERLOAD			42

#define MODE_WATCHDOG				50
#define MODE_COMMUNICATION_ERROR	53



#define MODE_OPTIONS_BEGIN					100
#define MODE_OPTION_REMEMBER_BEEP_ON		100
#define MODE_OPTION_REMEMBER_BEEP_OFF		101
#define MODE_OPTION_7SEGMENT_BLINK			102


void resetValues();
void ProcessCommand(void);
void WriteFile(void);
bool ReadFile(void);
void ReadConfigurationFile(void);


void OptionControl();
void TempControl();
void PressControl();
void MotorControl();
void ValveControl();
void ScaleFunction();
void setFreqTimer();
void FreqHandler(); 
void SegmentDisplay();
void SegmentDisplaySimple(char curSegmentDisplay);
void SegmentDisplayOptimized(char curSegmentDisplay);
void Beep();
