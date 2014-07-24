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
 * @brief      This class control the solenoid valve of everycook,
 * @author     Arthur Bouché
 */
#ifndef SOLENOID_H_
#define SOLENOID_H_

#include <stdio.h>
#include "daemon.h"
#include "basic_functions.h"
#include "virtualspiAtmel.h"

#define I2C_VALVE_OPEN_VALUE	210
#define I2C_VALVE_CLOSED_VALUE	350

#define I2C_SERVO				1

struct Solenoid_I2c_Values {
	uint16_t i2c_solenoid_open;
	uint16_t i2c_solenoid_closed;
	
	float currentValue;
	uint16_t i2c_solenoid_value;
	uint8_t solenoidOpen;
};
/** @brief open or close the solenoid valve
 *  @param open : true/false = open/close
 */
void solenoidSetOpen(bool open);

// Set function
void solenoidSetI2cOpen(uint16_t i2c0pen);
void solenoidSetI2cClosed(uint16_t i2cClosed);
// Get function
uint16_t solenoidGetI2cOpen();
uint16_t solenoidGetI2cClosed();

#endif /*----SOLENOID_H_-----*/
