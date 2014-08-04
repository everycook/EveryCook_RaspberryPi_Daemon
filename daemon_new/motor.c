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
#include "motor.h"

const float MOTOR_RPM_TO_PWM = 4095.0/200.0;

uint8_t motorI2cConfig=I2C_MOTOR;

struct Motor_Command_Values motorNewCommandValues={0,0,0};
struct Motor_Command_Values motorCurrentCommandValues={0,0,0};
struct Motor_Command_Values motorOldCommandValues={0,0,0};

struct Motor_I2C_Values motor_i2c_values = {10,10, 0,0,0.0, 0,0,0,10}; //last value (motorRpm) set to 10 so it will be reset to 0 on startup

uint32_t motorStartTime; 
uint32_t motorStopTime;

uint32_t motorHourConter=0.0; // hour conter

void motorAtmelSetRPM(uint8_t rpm);
void motorI2cSetRPM(uint16_t rpm);

/** @brief send command to the motor
 *  @param rpm : command
 *  @param *dv : configuration motor
*/
void motorSetCommandRPM(uint16_t rpm){ //setMotorRPM
	if (daemonGetSettingsDebug_enabled()|| daemonGetSettingsDebug3_enabled()){printf("setMotorRPM, rpm: %d (current: %d)\n", rpm,motorGetI2cValuesMotorRpm());}
	if (rpm != 0 && rpm < motorGetI2cValuesSpeedMin()){
		rpm = motorGetI2cValuesSpeedMin();
		if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled()){printf("rpm was to low, changed to: %d\n", rpm);}
	}
	
	if (!heaterGetStatusHasPower() && rpm != 0){
		if(daemonGetSettingsShieldVersion() < 4){
			uint32_t timeouts[2];
			if (daemonGetSettingsShieldVersion() == 1){
				timeouts[0] = 12000;
				timeouts[1] = 6000;
			} else {
				timeouts[0] = 5000;
				timeouts[1] = 2000;
			}
			if ((daemonGetTimeValuesRunTimeMillis() - heaterGetStatusHasPowerLedOnLastTime()) > timeouts[0]){
				//Manipulate values, so it will set to 0 directly, because there is no power
				rpm = 0;
				motorSetI2cValuesMotorRpm(motorGetI2cValuesSpeedMin());
				motorSetI2cValuesDestRpm(motorGetI2cValuesSpeedMin());
			} else if ((daemonGetTimeValuesRunTimeMillis() - heaterGetStatusHasPowerLedOnLastTime()) > timeouts[1]){
				//set dest rpm to 0, so it will create a ramp down, if hasPower state is false it will not stop directly.
				rpm = 0;
			}
		} else {
			//Manipulate values, so it will set to 0 directly, because there is no power
			rpm = 0;
			motorSetI2cValuesMotorRpm(motorGetI2cValuesSpeedMin());
			motorSetI2cValuesDestRpm(motorGetI2cValuesSpeedMin());
		}
	}
	
	if ( motorGetI2cValuesMotorRpm()!= rpm){
		if (motor_i2c_values.destRpm != rpm){
			motorSetI2cValuesDestRpm(rpm);
			motor_i2c_values.currentStep = 0;
			motor_i2c_values.stepSize = motorGetI2cValuesSpeedRamp() / (1000.0/daemonGetSettingsLongDelay());
			motor_i2c_values.currentValue = motorGetI2cValuesMotorRpm();
			
			
			//logic for speed min
			if (rpm == 0){
				rpm = motorGetI2cValuesSpeedMin();
			} else if ( motorGetI2cValuesMotorRpm() == 0){
				motor_i2c_values.currentStep = -1;
				motor_i2c_values.currentValue = motorGetI2cValuesSpeedMin();
			}
			
			int16_t changeRpm = rpm - motor_i2c_values.currentValue;
			if (rpm < motorGetI2cValuesMotorRpm()){
				motor_i2c_values.stepSize = -motor_i2c_values.stepSize;
			}
			motor_i2c_values.steps = changeRpm / motor_i2c_values.stepSize;
			
			if (daemonGetRunningModeSimulationMode() || daemonGetSettingsDebug3_enabled()){printf("setMotorRPM: current:%d, steps:%d, stepSize:%.2f\n", motorGetI2cValuesMotorRpm(), motor_i2c_values.steps, motor_i2c_values.stepSize);}
		}
		
		if (motor_i2c_values.currentStep < motor_i2c_values.steps){
			++motor_i2c_values.currentStep;
			if (motor_i2c_values.currentStep != 0){
				motor_i2c_values.currentValue=motor_i2c_values.currentValue + motor_i2c_values.stepSize;
			}
			motor_i2c_values.i2c_motor_value=motor_i2c_values.currentValue * MOTOR_RPM_TO_PWM;
			if (daemonGetRunningModeSimulationMode() || daemonGetSettingsDebug3_enabled()){printf("setMotorRPM: value:%.2f, motor_value:%d\n", motor_i2c_values.currentValue, motor_i2c_values.i2c_motor_value);}
			motorSetI2cValuesMotorRpm(motor_i2c_values.currentValue);
			if (!daemonGetRunningModeSimulationMode()){
				if(daemonGetSettingsShieldVersion()<4){
					motorI2cSetRPM(motor_i2c_values.i2c_motor_value);
				} else {
					motorAtmelSetRPM(motorGetI2cValuesMotorRpm());
				}
			}

			daemonSetStateDelay(daemonGetSettingsLongDelay());
		} else {
			if (motorGetI2cValuesMotorRpm() != motor_i2c_values.destRpm){
				motor_i2c_values.currentValue = motor_i2c_values.destRpm;
				motor_i2c_values.i2c_motor_value=motor_i2c_values.currentValue * MOTOR_RPM_TO_PWM;
				if (daemonGetRunningModeSimulationMode() || daemonGetSettingsDebug3_enabled()){printf("setMotorRPM: value:%.2f, motor_value:%d\n", motor_i2c_values.currentValue, motor_i2c_values.i2c_motor_value);}
				motorSetI2cValuesMotorRpm(motor_i2c_values.destRpm);
				if (!daemonGetRunningModeSimulationMode()){
					if(daemonGetSettingsShieldVersion() < 4){
						motorI2cSetRPM(motor_i2c_values.i2c_motor_value);
					} else {
						motorAtmelSetRPM(motorGetI2cValuesMotorRpm());
					}
				}
			}

		}
	}
}

bool motorGetPosSensor(){//atmelGetMotorPosSensor
	SPIAtmelWrite(SPI_MODE_GET_MOTOR_POS_SENSOR);
	return getResult();
}

uint8_t motorGetMotorRPM(){//atmelGetMotorRPM
	SPIAtmelWrite(SPI_MODE_GET_MOTOR_RPM);
	return getResult();
}

uint8_t motorGetMotorSpeed(){//atmelGetMotorSpeed
	SPIAtmelWrite(SPI_MODE_GET_MOTOR_SPEED);
	return getResult();
}

void motorSetSpeedRPM(uint16_t valueRpm){
	if(daemonGetSettingsShieldVersion() < 4){
				motorI2cSetRPM(valueRpm);
	} else {
				motorAtmelSetRPM(valueRpm);
	}
}

void motorAtmelSetRPM(uint8_t rpm){//atmelSetMotorRPM
	if (atmelGetDebug2()) {printf("--->motorAtmelSetRPM\n");}
	SPIAtmelWrite(SPI_MODE_MOTOR);
	SPIAtmelWrite(rpm);
	getValidResultOrReset();
}
void motorI2cSetRPM(uint16_t rpm){
	writeI2CPin(motorI2cConfig,rpm);
}

/** @brief Controle Motor
*/
void motorControl(){//MotorControl
	if (daemonGetCurrentCommandValuesMode()==MODE_CUT || daemonGetCurrentCommandValuesMode()>=MIN_TEMP_MODE){
		if (daemonGetCurrentCommandValuesMode()==MODE_CUT) daemonSetTimeValuesStepEndTime(daemonGetTimeValuesRunTime()+1);
		if (motorNewCommandValues.Rpm > 0 && daemonGetCurrentCommandValuesMode()<=MAX_STATUS_MODE) {
			if  (motorNewCommandValues.On > 0 && motorNewCommandValues.Off > 0) {
				if (daemonGetTimeValuesRunTime() >= motorStartTime+motorNewCommandValues.On && daemonGetTimeValuesRunTime() < motorStartTime+motorNewCommandValues.On+motorNewCommandValues.Off) { 
					motorCurrentCommandValues.Rpm = 0; 
				} else if (motorCurrentCommandValues.Rpm==0){
					motorCurrentCommandValues.Rpm = motorNewCommandValues.Rpm;
					motorStartTime=daemonGetTimeValuesRunTime();
				} else if (motorCurrentCommandValues.Rpm != motorNewCommandValues.Rpm){
					motorCurrentCommandValues.Rpm = motorNewCommandValues.Rpm;
				}
			} else {
				motorCurrentCommandValues.Rpm = motorNewCommandValues.Rpm;
				motorStartTime=daemonGetTimeValuesRunTime();
			}
		} else {
			motorCurrentCommandValues.Rpm = 0;
		}
	} else {
		motorCurrentCommandValues.Rpm = 0;
	}
	motorSetCommandRPM(motorCurrentCommandValues.Rpm);
	
	if (motorGetI2cValuesMotorRpm()== 0){
		if(motorStopTime < motorStartTime){
			motorStopTime = daemonGetTimeValuesRunTime();
		}
	}
}

/** @brief update time of motor
*/
void motorUpdateTime(){//updateMotorTime
	//if (currentCommandValues.motorRpm>0){
	if (motor_i2c_values.motorRpm>0){
		if (motorStartTime < daemonGetTimeValuesLastLogSaveTime()){
			motorHourConter = motorHourConter + (daemonGetTimeValuesRunTime() - daemonGetTimeValuesLastLogSaveTime());
		} else {
			motorHourConter = motorHourConter + (daemonGetTimeValuesRunTime() - motorStartTime);
		}
	} else if (motorGetI2cValuesMotorRpm() == 0){
		if (motorStopTime > daemonGetTimeValuesLastLogSaveTime()){
			motorHourConter = motorHourConter + (motorStopTime - daemonGetTimeValuesLastLogSaveTime());
		}
	}
}
//GET
uint16_t motorGetI2cValuesSpeedMin(){
	return motor_i2c_values.i2c_motor_speed_min;
}
uint16_t motorGetI2cValuesSpeedRamp(){
	return motor_i2c_values.i2c_motor_speed_ramp;
}
uint16_t motorGetI2cValuesMotorRpm(){
	return motor_i2c_values.motorRpm;
}
uint8_t motorGetI2cConfig(){
	return motorI2cConfig;
}
uint8_t motorGetNewCommandValuesRpm(){
	return motorNewCommandValues.Rpm;
}
uint32_t motorGetNewCommandValuesOn(){
	return motorNewCommandValues.On;
}
uint32_t motorGetNewCommandValuesOff(){
	return motorNewCommandValues.Off;
}
uint8_t motorGetCurrentCommandValuesRpm(){
	return motorCurrentCommandValues.Rpm;
}
uint32_t motorGetCurrentCommandValuesOn(){
	return motorCurrentCommandValues.On;
}
uint32_t motorGetCurrentCommandValuesOff(){
	return motorCurrentCommandValues.Off;
}
uint32_t motorGetHourCounter(){
	return motorHourConter;
}
//SET
void motorSetI2cValuesMotorRpm(uint16_t rpm){
	motor_i2c_values.motorRpm=rpm;
}
void motorSetI2cValuesDestRpm(uint16_t destrpm){
	motor_i2c_values.destRpm=destrpm;
}
void motorSetHourCounter(uint32_t hour){
	motorHourConter=hour;
}
void motorSetNewCommandValuesRpm(uint8_t newRpm){
	motorNewCommandValues.Rpm=newRpm;
}
void motorSetNewCommandValuesOn(uint32_t on){
	motorNewCommandValues.On=on;
}
void motorSetNewCommandValuesOff(uint32_t off){
	motorNewCommandValues.Off=off;
}
void motorSetI2cValuesSpeedMin(uint16_t speedMin){
	motor_i2c_values.i2c_motor_speed_min=speedMin;
}
void motorSetI2cValuesSpeedRamp(uint16_t speedRamp){
	motor_i2c_values.i2c_motor_speed_ramp=speedRamp;
}
void motorSetI2cConfig(uint8_t i2cConfig){
	motorI2cConfig=i2cConfig;
}
