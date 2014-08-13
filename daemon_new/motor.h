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
 * @brief      This class control the motor of everycook,
 * @author     Arthur Bouché
 */
#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdio.h>
#include "daemon.h"
#include "virtualspiAtmel.h"
#include "basic_functions.h"
#include "heater.h"

#define I2C_MOTOR	0
struct Motor_Command_Values{
	uint8_t Rpm; //0-255 
	uint32_t On;
	uint32_t Off;
};
	

struct Motor_I2C_Values {
	uint16_t i2c_motor_speed_min;
	uint16_t i2c_motor_speed_ramp;
	
	uint16_t destRpm;
	uint16_t steps;
	float stepSize;
	
	int16_t currentStep;
	float currentValue;
	uint16_t i2c_motor_value;
	uint16_t motorRpm;
};

/** @brief send command to the motor
 *  @param rpm : command
 *  @param *dv : configuration motor
*/
void motorSetCommandRPM(uint16_t rpm);

void motorSetSpeedRPM(uint16_t valueRpm);

void motorControl();

void motorUpdateTime();

bool motorGetPosSensor();
uint8_t motorGetMotorRPM();
uint8_t motorGetMotorSpeed();
//GET
uint16_t motorGetI2cValuesSpeedMin();
uint16_t motorGetI2cValuesSpeedRamp();
uint16_t motorGetI2cValuesMotorRpm();
uint8_t motorGetI2cConfig();
uint8_t motorGetNewCommandValuesRpm();
uint32_t motorGetNewCommandValuesOn();
uint32_t motorGetNewCommandValuesOff();
uint8_t motorGetCurrentCommandValuesRpm();
uint32_t motorGetCurrentCommandValuesOn();
uint32_t motorGetCurrentCommandValuesOff();
uint32_t motorGetHourCounter();
uint8_t motorGetRPMTrue();
bool motorGetSensor();
uint8_t motorGetPWMTrue();

//SET
void motorSetI2cValuesMotorRpm(uint16_t rpm);
void motorSetI2cValuesDestRpm(uint16_t destrpm);
void motorSetHourCounter(uint32_t hour);
void motorSetNewCommandValuesRpm(uint8_t newRpm);
void motorSetNewCommandValuesOn(uint32_t on);
void motorSetNewCommandValuesOff(uint32_t off);
void motorSetI2cValuesSpeedMin(uint16_t speedMin);
void motorSetI2cValuesSpeedRamp(uint16_t speedRamp);
void motorSetI2cConfig(uint8_t i2cConfig);
void motorSetRPMTrue(uint8_t rpm);
void motorSetSensor(bool sensor);
void motorSetPWMTrue(uint8_t pwm);

#endif /*----MOTOR_H_-----*/
