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

//#include <wiringPi.h>

#include "bool.h"
#include "modes.h"
#include "basic_functions.h"
#include "daemon_structs.h"
#include "hardwareFunctions.h"
#include "virtualspiAtmel.h"



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


