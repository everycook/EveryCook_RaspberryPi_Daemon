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

#include "bool.h"

double readTemp(struct Daemon_Values *dv);
int32_t readPress(struct Daemon_Values *dv);
bool HeatOn(struct Daemon_Values *dv);

/** @brief turn heat off
 *	@param *dv : Daemon_Values
 *  @return succed?
*/
bool HeatOff(struct Daemon_Values *dv);

/** @brief send command to the motor
 *  @param rpm : command
 *  @param *dv : configuration motor
*/
void setMotorRPM(uint16_t rpm, struct Daemon_Values *dv);

void setSolenoidOpen(bool open, struct Daemon_Values *dv);
void setServoOpen(uint8_t openPercent, uint8_t steps, uint16_t stepWait, struct Daemon_Values *dv);
double readWeight(struct Daemon_Values *dv);
double readWeightSeparate(double* values, struct Daemon_Values *dv);

/** @brief display a char on 7 segment
 *  @param curSegmentDisplay : char to display
 *  @param state : state of 7 seg
 *  @param i2c_config
*/
void SegmentDisplaySimple(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config);

void SegmentDisplayOptimized(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config);

/** @brief blink one segment after one
 *  @param *i2c_config : configuration of I2C
*/
void blink7Segment(struct I2C_Config *i2c_config);
