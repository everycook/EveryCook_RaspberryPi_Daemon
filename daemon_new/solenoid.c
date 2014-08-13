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
#include "solenoid.h"
uint8_t i2c_servo_config=I2C_SERVO;
struct Solenoid_I2c_Values solenoid = {I2C_VALVE_OPEN_VALUE, I2C_VALVE_CLOSED_VALUE, I2C_VALVE_CLOSED_VALUE, 0, 0};

void atmelSetSolenoidOpen(bool open);


void solenoidSetOpen(bool open){
	if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled()){printf("setSolenoidOpen, open: %d\n", open);}
	if (solenoid.solenoidOpen != open){
		if(daemonGetSettingsShieldVersion() < 4){
			if (open){
				solenoid.currentValue = solenoidGetI2cOpen();
			} else {
				solenoid.currentValue = solenoidGetI2cClosed();
			}
				solenoid.i2c_solenoid_value = (int)(solenoid.currentValue);
			
			if (daemonGetRunningModeSimulationMode() || daemonGetSettingsDebug3_enabled()){printf("setSolonoidOpen: value:%.2f, solonoid_value:%d\n", solenoid.currentValue,solenoid.i2c_solenoid_value);}
			if (!daemonGetRunningModeSimulationMode()){
				writeI2CPin(i2c_servo_config, solenoid.i2c_solenoid_value);
			}
		} else {
			if (daemonGetRunningModeSimulationMode() || daemonGetSettingsDebug3_enabled()){printf("setSolonoidOpen: old: %d, new: %d\n",solenoid.solenoidOpen, open);}
			if (!daemonGetRunningModeSimulationMode()){
			}
		}
		solenoid.solenoidOpen = open;
	}
}
// Set function
void solenoidSetI2cOpen(uint16_t i2c0pen){
	solenoid.i2c_solenoid_open=i2c0pen;
}
void solenoidSetI2cClosed(uint16_t i2cClosed){
	solenoid.i2c_solenoid_closed=i2cClosed;
}
// Get function
uint16_t solenoidGetI2cOpen(){
	return solenoid.i2c_solenoid_open;
}
uint16_t solenoidGetI2cClosed(){
	return solenoid.i2c_solenoid_closed;
}
uint8_t solenoidGetOpen(){
	return solenoid.solenoidOpen;
}
