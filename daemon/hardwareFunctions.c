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

#include <stdio.h>
#include <math.h>

#include <wiringPi.h>

#include "bool.h"
#include "modes.h"
#include "basic_functions.h"
#include "daemon_structs.h"
#include "hardwareFunctions.h"
#include "virtualspiAtmel.h"

const float MOTOR_RPM_TO_PWM = 4095.0/200.0;

/******************* functions to evaluate **********************/

double readTemp(struct Daemon_Values *dv){
		if (dv->settings->debug_enabled || dv->runningMode->calibration || dv->settings->debug3_enabled){printf("Temp %d dig %.0f C | ", dv->adc_values->Temp.adc_value, dv->adc_values->Temp.valueByOffset);}
		return dv->adc_values->Temp.valueByOffset;
}


int32_t readPress(struct Daemon_Values *dv){
		if (dv->settings->debug_enabled || dv->runningMode->calibration || dv->settings->debug3_enabled){printf("Press %d digits %.0f kPa | ", dv->adc_values->Press.adc_value, dv->adc_values->Press.valueByOffset);}
		return dv->adc_values->Press.valueByOffset;
}

//Power control functions
bool HeatOn(struct Daemon_Values *dv){
	if(dv->settings->shieldVersion < 4){
		//if (dv->settings->debug_enabled || dv->runningMode->simulationMode){printf("HeatOn, was: %d\n", state->heatPowerStatus);}
		if (dv->state->heatPowerStatus && !dv->heaterStatus->isOn){
			if ((dv->timeValues->runTimeMillis - dv->heaterStatus->isOnLastTime) > 6000){
				dv->state->heatPowerStatus = false;
			}
		}
		if (dv->adc_config->restarting_adc){
			return false;
		}
	
		if (!dv->state->heatPowerStatus) { //if its off
			if (dv->settings->debug_enabled || dv->runningMode->simulationMode){printf("HeatOn status was: %d, led is heating: %d\n", dv->state->heatPowerStatus, dv->heaterStatus->isOn);}
			if (!dv->runningMode->simulationMode){
				if (!dv->heaterStatus->isOn){
					writeControllButtonPin(IND_KEY_POWER, 0); //"press" the power button
					delay(500);
					writeControllButtonPin(IND_KEY_POWER, 1);
					if (dv->settings->debug3_enabled){printf("-->HeatOn\n");}
				} else {
					printf("ERROR: should turn HeatOn, but led's say it is already on, so nothing done\n");
				}
			}
			dv->state->heatPowerStatus=true; //save that we turned it on
			dv->timeValues->heaterStartTime = dv->timeValues->runTime;
			return true;
		} else {
			return false;
		}
	} else {
		if (dv->state->heatPowerStatus && !dv->heaterStatus->isOn){
			dv->state->heatPowerStatus = false;
		}

		if (dv->adc_config->restarting_adc){
			return false;
		}

		if (!dv->state->heatPowerStatus) { //if its off
			if (dv->settings->debug_enabled || dv->runningMode->simulationMode){printf("HeatOn status was: %d, led is heating: %d\n", dv->state->heatPowerStatus, dv->heaterStatus->isOn);}
			if (!dv->runningMode->simulationMode){
				if (!dv->heaterStatus->isOn){
					atmelSetHeating(true);
					if (dv->settings->debug3_enabled){printf("-->HeatOn\n");}
				} else {
					printf("ERROR: should turn HeatOn, but ATMega644 status say it's already on, so nothing done\n");
				}
			}
			dv->state->heatPowerStatus=true; //save that we turned it on
			dv->timeValues->heaterStartTime = dv->timeValues->runTime;
			return true;
		} else {
			return false;
		}
	}
}

bool HeatOff(struct Daemon_Values *dv){
	if(dv->settings->shieldVersion < 4){
		if (!dv->state->heatPowerStatus && dv->heaterStatus->isOn){
			if ((dv->timeValues->runTimeMillis - dv->heaterStatus->isOnLastTime) < 3000){
				dv->state->heatPowerStatus = true;
			}
		}
		if (dv->adc_config->restarting_adc){
			return false;
		}
	
		//if (dv->settings->debug_enabled || dv->runningMode->simulationMode){printf("HeatOff, was: %d\n", state->heatPowerStatus);}
		if (dv->state->heatPowerStatus) { //if its on
			if (dv->settings->debug_enabled || dv->runningMode->simulationMode){printf("HeatOff status was: %d, led is heating: %d\n", dv->state->heatPowerStatus, dv->heaterStatus->isOn);}
			if (!dv->runningMode->simulationMode){
				if (dv->heaterStatus->isOn){
					writeControllButtonPin(IND_KEY_POWER, 0); //"press" the power button
					delay(500);
					writeControllButtonPin(IND_KEY_POWER, 1);
					if (dv->settings->debug3_enabled){printf("-->HeatOff\n");}
				} else {
					printf("ERROR: should turn HeatOff, but led's say it is already off, so nothing done\n");
				}
			}
			dv->state->heatPowerStatus=false; //save that we turned it off
			dv->timeValues->heaterStopTime = dv->timeValues->runTime;
			return true;
		} else {
			return false;
		}
	} else {
		if (!dv->state->heatPowerStatus && dv->heaterStatus->isOn){
			dv->state->heatPowerStatus = true;
		}

		if (dv->adc_config->restarting_adc){
			return false;
		}

		if (dv->state->heatPowerStatus) { //if its on
			if (dv->settings->debug_enabled || dv->runningMode->simulationMode){printf("HeatOff status was: %d, led is heating: %d\n", dv->state->heatPowerStatus, dv->heaterStatus->isOn);}
			if (!dv->runningMode->simulationMode){
				if (dv->heaterStatus->isOn){
					atmelSetHeating(false);
										if (dv->settings->debug3_enabled){printf("-->HeatOff\n");}
				} else {
					printf("ERROR: should turn HeatOff, but ATMega644 status say it is already off, so nothing done\n");
				}
			}
			dv->state->heatPowerStatus=false; //save that we turned it off
			dv->timeValues->heaterStopTime = dv->timeValues->runTime;
			return true;
		} else {
			return false;
		}
	}
}

void setMotorRPM(uint16_t rpm, struct Daemon_Values *dv){
	if (dv->settings->debug_enabled || dv->settings->debug3_enabled){printf("setMotorRPM, rpm: %d (current: %d)\n", rpm, dv->i2c_motor_values->motorRpm );}
	if (rpm != 0 && rpm < dv->i2c_motor_values->i2c_motor_speed_min){
		rpm = dv->i2c_motor_values->i2c_motor_speed_min;
		if (dv->settings->debug_enabled || dv->settings->debug3_enabled){printf("rpm was to low, changed to: %d\n", rpm);}
	}
	
	if (!dv->heaterStatus->hasPower && rpm != 0){
		if(dv->settings->shieldVersion < 4){
			uint32_t timeouts[2];
			if (dv->settings->shieldVersion == 1){
				timeouts[0] = 12000;
				timeouts[1] = 6000;
			} else {
				timeouts[0] = 5000;
				timeouts[1] = 2000;
			}
			if ((dv->timeValues->runTimeMillis - dv->heaterStatus->hasPowerLedOnLastTime) > timeouts[0]){
				//Manipulate values, so it will set to 0 directly, because there is no power
				rpm = 0;
				dv->i2c_motor_values->motorRpm = dv->i2c_motor_values->i2c_motor_speed_min;
				dv->i2c_motor_values->destRpm = dv->i2c_motor_values->i2c_motor_speed_min;
			} else if ((dv->timeValues->runTimeMillis - dv->heaterStatus->hasPowerLedOnLastTime) > timeouts[1]){
				//set dest rpm to 0, so it will create a ramp down, if hasPower state is false it will not stop directly.
				rpm = 0;
			}
		} else {
			//Manipulate values, so it will set to 0 directly, because there is no power
			rpm = 0;
			dv->i2c_motor_values->motorRpm = dv->i2c_motor_values->i2c_motor_speed_min;
			dv->i2c_motor_values->destRpm = dv->i2c_motor_values->i2c_motor_speed_min;
		}
	}
	
	if (dv->i2c_motor_values->motorRpm != rpm){
		if (dv->i2c_motor_values->destRpm != rpm){
			dv->i2c_motor_values->destRpm = rpm;
			dv->i2c_motor_values->currentStep = 0;
			dv->i2c_motor_values->stepSize = dv->i2c_motor_values->i2c_motor_speed_ramp / (1000.0/dv->settings->LongDelay);
			dv->i2c_motor_values->currentValue = dv->i2c_motor_values->motorRpm;
			
			
			//logic for speed min
			if (rpm == 0){
				rpm = dv->i2c_motor_values->i2c_motor_speed_min;
			} else if (dv->i2c_motor_values->motorRpm == 0){
				dv->i2c_motor_values->currentStep = -1;
				dv->i2c_motor_values->currentValue = dv->i2c_motor_values->i2c_motor_speed_min;
			}
			
			int16_t changeRpm = rpm - dv->i2c_motor_values->currentValue;
			if (rpm < dv->i2c_motor_values->motorRpm){
				dv->i2c_motor_values->stepSize = -dv->i2c_motor_values->stepSize;
			}
			dv->i2c_motor_values->steps = changeRpm / dv->i2c_motor_values->stepSize;
			
			if (dv->runningMode->simulationMode || dv->settings->debug3_enabled){printf("setMotorRPM: current:%d, steps:%d, stepSize:%.2f\n", dv->i2c_motor_values->motorRpm, dv->i2c_motor_values->steps, dv->i2c_motor_values->stepSize);}
		}
		
		if (dv->i2c_motor_values->currentStep < dv->i2c_motor_values->steps){
			++dv->i2c_motor_values->currentStep;
			if (dv->i2c_motor_values->currentStep != 0){
				dv->i2c_motor_values->currentValue=dv->i2c_motor_values->currentValue + dv->i2c_motor_values->stepSize;
			}
			dv->i2c_motor_values->i2c_motor_value=dv->i2c_motor_values->currentValue * MOTOR_RPM_TO_PWM;
			if (dv->runningMode->simulationMode || dv->settings->debug3_enabled){printf("setMotorRPM: value:%.2f, motor_value:%d\n", dv->i2c_motor_values->currentValue, dv->i2c_motor_values->i2c_motor_value);}
			dv->i2c_motor_values->motorRpm = dv->i2c_motor_values->currentValue;
			if (!dv->runningMode->simulationMode){
				if(dv->settings->shieldVersion < 4){
					writeI2CPin(dv->i2c_config->i2c_motor, dv->i2c_motor_values->i2c_motor_value);
				} else {
					atmelSetMotorRPM(dv->i2c_motor_values->motorRpm);
				}
			}

			dv->state->Delay = dv->settings->LongDelay;
		} else {
			if (dv->i2c_motor_values->motorRpm != dv->i2c_motor_values->destRpm){
				dv->i2c_motor_values->currentValue = dv->i2c_motor_values->destRpm;
				dv->i2c_motor_values->i2c_motor_value=dv->i2c_motor_values->currentValue * MOTOR_RPM_TO_PWM;
				if (dv->runningMode->simulationMode || dv->settings->debug3_enabled){printf("setMotorRPM: value:%.2f, motor_value:%d\n", dv->i2c_motor_values->currentValue, dv->i2c_motor_values->i2c_motor_value);}
				dv->i2c_motor_values->motorRpm = dv->i2c_motor_values->destRpm;
				if (!dv->runningMode->simulationMode){
					if(dv->settings->shieldVersion < 4){
						writeI2CPin(dv->i2c_config->i2c_motor, dv->i2c_motor_values->i2c_motor_value);
					} else {
						atmelSetMotorRPM(dv->i2c_motor_values->motorRpm);
					}
				}
			}

		}
	}
}

void setSolenoidOpen(bool open, struct Daemon_Values *dv){
	if (dv->settings->debug_enabled || dv->settings->debug3_enabled){printf("setSolenoidOpen, open: %d\n", open);}
	if (dv->i2c_solenoid_values->solenoidOpen != open){
		if(dv->settings->shieldVersion < 4){
			if (open){
				dv->i2c_solenoid_values->currentValue = dv->i2c_solenoid_values->i2c_solenoid_open;
			} else {
				dv->i2c_solenoid_values->currentValue = dv->i2c_solenoid_values->i2c_solenoid_closed;
			}
			dv->i2c_solenoid_values->i2c_solenoid_value = (int)(dv->i2c_solenoid_values->currentValue);
			
			if (dv->runningMode->simulationMode || dv->settings->debug3_enabled){printf("setSolonoidOpen: value:%.2f, solonoid_value:%d\n", dv->i2c_solenoid_values->currentValue, dv->i2c_solenoid_values->i2c_solenoid_value);}
			if (!dv->runningMode->simulationMode){
				writeI2CPin(dv->i2c_config->i2c_servo, dv->i2c_solenoid_values->i2c_solenoid_value);
			}
		} else {
			if (dv->runningMode->simulationMode || dv->settings->debug3_enabled){printf("setSolonoidOpen: old: %d, new: %d\n",dv->i2c_solenoid_values->solenoidOpen, open);}
			if (!dv->runningMode->simulationMode){
				atmelSetSolenoidOpen(open);
			}

		}
		dv->i2c_solenoid_values->solenoidOpen = open;
	}
}

void setServoOpen(uint8_t openPercent, uint8_t steps, uint16_t stepWait, struct Daemon_Values *dv){
	if (dv->settings->debug_enabled || dv->settings->debug3_enabled){printf("setServoOpen, open: %d\n", openPercent);}
	if (dv->i2c_servo_values->servoOpen != openPercent){
		if (steps<=0){
			steps=1;
		}
		if (dv->i2c_servo_values->steps != steps || dv->i2c_servo_values->destOpenPercent != openPercent){
			dv->i2c_servo_values->steps = steps;
			dv->i2c_servo_values->destOpenPercent = openPercent;
			
			int16_t minMaxDiff = dv->i2c_servo_values->i2c_servo_open-dv->i2c_servo_values->i2c_servo_closed;
			int8_t changePercent = openPercent - dv->i2c_servo_values->servoOpen;
			float onePercent = minMaxDiff/100.0;
			float stepSize = (changePercent*onePercent)/steps;
			dv->i2c_servo_values->stepSize = stepSize;
			dv->i2c_servo_values->stepPercentSize = changePercent/steps;
			dv->i2c_servo_values->currentStep = 0;
			
			dv->i2c_servo_values->currentValue = dv->i2c_servo_values->servoOpen * onePercent;
			dv->i2c_servo_values->i2c_servo_value = dv->i2c_servo_values->i2c_servo_closed;
			if (dv->runningMode->simulationMode || dv->settings->debug3_enabled){printf("setServoOpen: minMaxDiff:%d, changePercent:%d onePercent:%.2f, stepSize:%.2f\n", minMaxDiff, changePercent, onePercent, stepSize);}
		}
		
		if (dv->i2c_servo_values->currentStep < steps){
		//while(dv->i2c_servo_values->currentStep < steps){
			++dv->i2c_servo_values->currentStep;
			dv->i2c_servo_values->currentValue=dv->i2c_servo_values->currentValue + dv->i2c_servo_values->stepSize;
			dv->i2c_servo_values->i2c_servo_value=dv->i2c_servo_values->i2c_servo_closed + (int)(dv->i2c_servo_values->currentValue);
			if (dv->runningMode->simulationMode || dv->settings->debug3_enabled){printf("setServoOpen: value:%.2f, servo_value:%d\n", dv->i2c_servo_values->currentValue, dv->i2c_servo_values->i2c_servo_value);}
			if (!dv->runningMode->simulationMode){
				writeI2CPin(dv->i2c_config->i2c_servo, dv->i2c_servo_values->i2c_servo_value);
			}
			dv->i2c_servo_values->servoOpen = dv->i2c_servo_values->servoOpen + dv->i2c_servo_values->stepPercentSize;
			//delay(stepWait);
			dv->state->Delay = stepWait;
		} else {
			dv->i2c_servo_values->servoOpen = openPercent;
			dv->state->Delay = dv->settings->LongDelay;
		}
	} else {
		if (dv->state->Delay == stepWait){
			dv->state->Delay = dv->settings->LongDelay;
		}
	}
}

double readWeight(struct Daemon_Values *dv){
		uint32_t weightValue1 = dv->adc_values->LoadCellFrontLeft.adc_value;
		uint32_t weightValue2 = dv->adc_values->LoadCellFrontRight.adc_value;
		uint32_t weightValue3 = dv->adc_values->LoadCellBackLeft.adc_value;
		uint32_t weightValue4 = dv->adc_values->LoadCellBackRight.adc_value;
		
		uint32_t weightValueSum = dv->adc_values->Weight.adc_value;
		double weightValue = dv->adc_values->Weight.value;
		
		//if (settings->debug_enabled || runningMode->calibration){printf("readWeight %d digits, %->1f grams\n", weightValueSum, weightValue);}
		if (dv->settings->debug_enabled || dv->runningMode->calibration){printf("Weight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", weightValueSum, weightValue, (weightValue+dv->state->referenceForce), weightValue1, weightValue2, weightValue3, weightValue4);}

		return weightValue;
}

double readWeightSeparate(double* values, struct Daemon_Values *dv){
		uint32_t weightValue1 = dv->adc_values->LoadCellFrontLeft.adc_value;
		uint32_t weightValue2 = dv->adc_values->LoadCellFrontRight.adc_value;
		uint32_t weightValue3 = dv->adc_values->LoadCellBackLeft.adc_value;
		uint32_t weightValue4 = dv->adc_values->LoadCellBackRight.adc_value;
		
		if (values != NULL){
			values[0] = dv->adc_values->LoadCellFrontLeft.value;
			values[1] = dv->adc_values->LoadCellFrontRight.value;
			values[2] = dv->adc_values->LoadCellBackLeft.value;
			values[3] = dv->adc_values->LoadCellBackRight.value;
		}
		
		uint32_t weightValueSum = dv->adc_values->Weight.adc_value;
		double weightValue = dv->adc_values->Weight.value;
		
		if (dv->settings->debug_enabled || dv->runningMode->calibration){
			if (values != NULL){
				printf("Weight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d / FL %f FR %f BL %f BR %f)\n", weightValueSum, weightValue, (weightValue+dv->state->referenceForce), weightValue1, weightValue2, weightValue3, weightValue4, values[0], values[1], values[2], values[3]);
			} else {
				printf("Weight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", weightValueSum, weightValue, (weightValue+dv->state->referenceForce), weightValue1, weightValue2, weightValue3, weightValue4);
			}
		}
		return weightValue;
}

void SegmentDisplaySimple(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config){
	if (curSegmentDisplay == 'P'){
		state->oldSegmentDisplay = 'P';
		writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
	} else if (curSegmentDisplay == 'H'){
		state->oldSegmentDisplay = 'H';
		writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
	} else if (curSegmentDisplay == '0'){
		state->oldSegmentDisplay = '0';
		writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_ON);
	} else if (curSegmentDisplay == 'S'){
		state->oldSegmentDisplay = 'S';
		writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_ON);
	} else if (curSegmentDisplay == 'E'){
		state->oldSegmentDisplay = 'E';
		writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_ON);
	} else {
		state->oldSegmentDisplay = ' ';
		writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
	}
}

void SegmentDisplayOptimized(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config){
	if (curSegmentDisplay == 'P'){
		if (curSegmentDisplay != state->oldSegmentDisplay){
			if (state->oldSegmentDisplay == 'H'){
				writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_OFF);
				//writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (state->oldSegmentDisplay == '0'){
				//writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_OFF);
				writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (state->oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_OFF);
				writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
			} else {
				SegmentDisplaySimple(curSegmentDisplay, state, i2c_config);
			}
			state->oldSegmentDisplay = curSegmentDisplay;
		}
	} else if (curSegmentDisplay == 'H'){
		if (curSegmentDisplay != state->oldSegmentDisplay){
			if (state->oldSegmentDisplay == 'P'){
				writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_OFF);
				//writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (state->oldSegmentDisplay == '0'){
				writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_OFF);
				//writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (state->oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_OFF);
				writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
			} else {
				SegmentDisplaySimple(curSegmentDisplay, state, i2c_config);
			}
			state->oldSegmentDisplay = curSegmentDisplay;
		}
	} else if (curSegmentDisplay == '0'){
		if (curSegmentDisplay != state->oldSegmentDisplay){
			if (state->oldSegmentDisplay == 'H'){
				writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_OFF);
				//writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_ON);
			} else if (state->oldSegmentDisplay == 'P'){
				//writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_OFF);
				//writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_ON);
			} else if (state->oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_OFF);
				writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_ON);
			} else {
				SegmentDisplaySimple(curSegmentDisplay, state, i2c_config);
			}
			state->oldSegmentDisplay = curSegmentDisplay;
		}
	} else {
		SegmentDisplaySimple(curSegmentDisplay, state, i2c_config);
		/*
		//Empty all
		writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
		state->oldSegmentDisplay = ' ';
		*/
	}
}

void blink7Segment(struct I2C_Config *i2c_config){
	//mitte
	printf("center: %d\n", i2c_config->i2c_7seg_center);
	writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_config->i2c_7seg_center,I2C_7SEG_OFF);
	
	//links
	printf("top left: %d\n", i2c_config->i2c_7seg_top_left);
	writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_config->i2c_7seg_top_left,I2C_7SEG_OFF);
	
	//oben
	printf("top: %d\n", i2c_config->i2c_7seg_top);
	writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_config->i2c_7seg_top,I2C_7SEG_OFF);
	
	
	//rechts
	printf("top right: %d\n", i2c_config->i2c_7seg_top_right);
	writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_config->i2c_7seg_top_right,I2C_7SEG_OFF);
	
	//ulinks
	printf("bottom left: %d\n", i2c_config->i2c_7seg_bottom_left);
	writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_config->i2c_7seg_bottom_left,I2C_7SEG_OFF);
	
	//unten
	printf("bottom: %d\n", i2c_config->i2c_7seg_bottom);
	writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_config->i2c_7seg_bottom,I2C_7SEG_OFF);
	
	//urechts
	printf("bottom right: %d\n", i2c_config->i2c_7seg_bottom_right);
	writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_config->i2c_7seg_bottom_right,I2C_7SEG_OFF);
	
	//punkt
	printf("period: %d\n", i2c_config->i2c_7seg_period);
	writeI2CPin(i2c_config->i2c_7seg_period,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_config->i2c_7seg_period,I2C_7SEG_OFF);
	delay(5000);
}
