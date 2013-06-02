
#include <stdio.h>

#include <math.h>

#include <wiringPi.h>

#include "bool.h"

#include "modes.h"

#include "basic_functions.h"

#include "daemon_structs.h"

#include "hardwareFunctions.h"

/******************* functions to evaluate **********************/

double readTemp(struct Daemon_Values *dv){
	if (dv->runningMode->simulationMode){
		double tempValue = dv->oldCommandValues->temp;
		int deltaT=dv->newCommandValues->temp-dv->oldCommandValues->temp;
		if (dv->currentCommandValues->mode==MODE_HEATUP || dv->currentCommandValues->mode==MODE_COOK) {
			if (deltaT<0) {
				--tempValue;
			} else if (deltaT==0) {
				//Nothing
			} else if (deltaT <= 10) {
				tempValue = tempValue+1;
			} else if (deltaT <= 20) {
				tempValue = tempValue+5;
			} else if (deltaT <= 50) {
				tempValue = tempValue+25;
			} else {
				tempValue = tempValue+40;
			}
		} else if (dv->currentCommandValues->mode==MODE_COOLDOWN){
			tempValue = tempValue-1;
		}
		//if (settings->debug_enabled){printf("readTemp, new Value is %f\n", tempValue);}
		printf("readTemp, new Value is %f\n", tempValue);
		return tempValue;
	} else {
		uint32_t tempValueInt = readADC(dv->adc_config->ADC_Temp);
		double tempValue = (double)tempValueInt * dv->tempCalibration->scaleFactor+dv->tempCalibration->offset;
		tempValue = round(tempValue);
		if (dv->settings->debug_enabled || dv->runningMode->calibration || dv->settings->debug3_enabled){printf("Temp %d dig %.0f °C | ", tempValueInt, tempValue);}
		if (dv->runningMode->measure_noise) {
			if (tempValueInt>dv->adc_noise->MaxTemp) dv->adc_noise->MaxTemp=tempValueInt;
			if (tempValueInt<dv->adc_noise->MinTemp) dv->adc_noise->MinTemp=tempValueInt;
			dv->adc_noise->DeltaTemp=dv->adc_noise->MaxTemp-dv->adc_noise->MinTemp;
			printf("NoiseTemp %d | ", dv->adc_noise->DeltaTemp);
		}
		return tempValue;
	}
}


double readPress(struct Daemon_Values *dv){
	if (dv->runningMode->simulationMode){
		double pressValue = dv->oldCommandValues->press;
		int deltaP=dv->newCommandValues->press-dv->oldCommandValues->press;
		if (dv->currentCommandValues->mode==MODE_PRESSUP || dv->currentCommandValues->mode==MODE_PRESSHOLD) {
			if (deltaP<0) {
				--pressValue;
			} else if (deltaP==0) {
				//Nothing
			} else if (deltaP <= 10) {
				pressValue = pressValue+2;
			} else if (deltaP <= 50) {
				pressValue = pressValue+5;
			} else {
				pressValue = pressValue+10;
			}
		} else if (dv->currentCommandValues->mode==MODE_PRESSDOWN){
			pressValue = pressValue-1;
		}
		//if (settings->debug_enabled){printf("readPress, new Value is %f\n", pressValue);}
		printf("readPress, new Value is %f\n", pressValue);
		return pressValue;
	} else {
		uint32_t pressValueInt = readADC(dv->adc_config->ADC_Press);
		double pressValue = pressValueInt* dv->pressCalibration->scaleFactor+dv->pressCalibration->offset;
		if (dv->settings->debug_enabled || dv->runningMode->calibration || dv->settings->debug3_enabled){printf("Press %d digits %.1f kPa | ", pressValueInt, pressValue);}
		if (dv->runningMode->measure_noise){
			if (pressValueInt>dv->adc_noise->MaxPress) dv->adc_noise->MaxPress=pressValueInt;
			if (pressValueInt<dv->adc_noise->MinPress) dv->adc_noise->MinPress=pressValueInt;
			dv->adc_noise->DeltaPress=dv->adc_noise->MaxPress-dv->adc_noise->MinPress;
			printf("NoisePress %d |", dv->adc_noise->DeltaPress);
		}
		return pressValue;
	}
}

//Power control functions
bool HeatOn(struct Daemon_Values *dv){
	//if (dv->settings->debug_enabled || dv->runningMode->simulationMode){printf("HeatOn, was: %d\n", state->heatPowerStatus);}
	if (!dv->state->heatPowerStatus) { //if its off
		if (dv->settings->debug_enabled || dv->runningMode->simulationMode || dv->runningMode->calibration){printf("HeatOn status was: %d\n", dv->state->heatPowerStatus);}
		if (!dv->runningMode->simulationMode){
			writeControllButtonPin(IND_KEY4, 0); //"press" the power button
			delay(500);
			writeControllButtonPin(IND_KEY4, 1);
			if (dv->settings->debug3_enabled){printf("-->HeatOn\n");}
		}
		dv->state->heatPowerStatus=true; //save that we turned it on
		return true;
	} else {
		return false;
	}
}

bool HeatOff(struct Daemon_Values *dv){
	//if (dv->settings->debug_enabled || dv->runningMode->simulationMode){printf("HeatOff, was: %d\n", state->heatPowerStatus);}
	if (dv->state->heatPowerStatus) { //if its on
		if (dv->settings->debug_enabled || dv->runningMode->simulationMode){printf("HeatOff\n");}
		if (!dv->runningMode->simulationMode){
			writeControllButtonPin(IND_KEY4, 0); //"press" the power button
			delay(500);
			writeControllButtonPin(IND_KEY4, 1);
			if (dv->settings->debug3_enabled){printf("-->HeatOff\n");}
		}
		dv->state->heatPowerStatus=false; //save that we turned it off
		return true;
	} else {
		return false;
	}
}

void setMotorPWM(uint16_t pwm, struct Daemon_Values *dv){
	if (dv->settings->debug_enabled){printf("setMotorPWM, pwm: %d\n", pwm);}
	if (dv->state->motorPwm != pwm){
		if (!dv->runningMode->simulationMode){
			writeI2CPin(dv->i2c_config->i2c_motor, pwm);
		} else {
			printf("setMotorPWM, pwm: %d\n", pwm);
		}
		dv->state->motorPwm = pwm;
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
			int8_t changePercent = openPercent-dv->i2c_servo_values->servoOpen;
			float onePercent = minMaxDiff/100.0;
			float stepSize = (changePercent*onePercent)/steps;
			dv->i2c_servo_values->stepSize = stepSize;
			dv->i2c_servo_values->stepPercentSize = changePercent/steps;
			dv->i2c_servo_values->currentStep = 0;
			
			dv->i2c_servo_values->currentValue = dv->i2c_servo_values->servoOpen*onePercent;
			dv->i2c_servo_values->i2c_servo_value = dv->i2c_servo_values->i2c_servo_closed;
			if (dv->runningMode->simulationMode || dv->settings->debug3_enabled){printf("setServoOpen: minMaxDiff:%d, changePercent:%d onePercent:%.2f, stepSize:%.2f\n", minMaxDiff, changePercent, onePercent, stepSize);}
		}
		
		if (dv->i2c_servo_values->currentStep < steps){
		//while(dv->i2c_servo_values->currentStep < steps){
			++dv->i2c_servo_values->currentStep;
			dv->i2c_servo_values->currentValue=dv->i2c_servo_values->currentValue+dv->i2c_servo_values->stepSize;
			dv->i2c_servo_values->i2c_servo_value=dv->i2c_servo_values->i2c_servo_closed + (int)(dv->i2c_servo_values->currentValue);
			if (dv->runningMode->simulationMode || dv->settings->debug3_enabled){printf("setServoOpen: value:%.2f, servo_value:%d\n", dv->i2c_servo_values->currentValue, dv->i2c_servo_values->i2c_servo_value);}
			if (!dv->runningMode->simulationMode){
				writeI2CPin(dv->i2c_config->i2c_servo, dv->i2c_servo_values->i2c_servo_value);
			}
			dv->i2c_servo_values->servoOpen = dv->i2c_servo_values->servoOpen+dv->i2c_servo_values->stepPercentSize;
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
	if (dv->runningMode->simulationMode){
		if (!dv->state->scaleReady) {
			return 0.0;
		} else {
			if (dv->timeValues->runTime <= dv->timeValues->simulationUpdateTime){
				return dv->oldCommandValues->weight; 
			} else {
				int deltaW = dv->newCommandValues->weight-dv->oldCommandValues->weight;
				double weightValue = dv->oldCommandValues->weight;
				if (deltaW<0) {
					--weightValue;
				} else if (deltaW==0) {
					//Nothing
				} else if (deltaW<=10) {
					weightValue = weightValue + 2;
				} else if (deltaW<=50) {
					weightValue = weightValue + 5;
				} else {
					weightValue = weightValue + 10;
				}
				if (weightValue != dv->oldCommandValues->weight){
					weightValue += dv->state->referenceForce;
					printf("readWeight, new Value is %f (old:%f)\n", weightValue, dv->oldCommandValues->weight);
					//if (settings->debug_enabled){printf("readWeight, new Value is %f\n", weightValue);}
				} else {
					weightValue += dv->state->referenceForce;
				}
				dv->timeValues->simulationUpdateTime = dv->timeValues->runTime;
				return weightValue;
			}
		}
	} else {
		uint32_t weightValue1 = readADC(dv->adc_config->ADC_LoadCellFrontLeft);
		uint32_t weightValue2 = readADC(dv->adc_config->ADC_LoadCellFrontRight);
		uint32_t weightValue3 = readADC(dv->adc_config->ADC_LoadCellBackLeft);
		uint32_t weightValue4 = readADC(dv->adc_config->ADC_LoadCellBackRight);
		
		uint32_t weightValueSum = (weightValue1+weightValue2+weightValue3+weightValue4) / 4;
		
		double weightValue = (double)weightValueSum;
		weightValue *=dv->forceCalibration->scaleFactor;
		weightValue = roundf(weightValue);
		//if (settings->debug_enabled || runningMode->calibration){printf("readWeight %d digits, %->1f grams\n", weightValueSum, weightValue);}
		if (dv->settings->debug_enabled || dv->runningMode->calibration){printf("Weight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", weightValueSum, weightValue, (weightValue+dv->state->referenceForce), weightValue1, weightValue2, weightValue3, weightValue4);}
		if (dv->runningMode->measure_noise){
			if (weightValue1>dv->adc_noise->MaxWeight1) dv->adc_noise->MaxWeight1=weightValue1;
			if (weightValue1<dv->adc_noise->MinWeight1) dv->adc_noise->MinWeight1=weightValue1;
			dv->adc_noise->DeltaWeight1=dv->adc_noise->MaxWeight1-dv->adc_noise->MinWeight1;
			printf("NoiseWeightFL %d | ", dv->adc_noise->DeltaWeight1);
			
			if (weightValue2>dv->adc_noise->MaxWeight2) dv->adc_noise->MaxWeight2=weightValue2;
			if (weightValue2<dv->adc_noise->MinWeight2) dv->adc_noise->MinWeight2=weightValue2;
			dv->adc_noise->DeltaWeight2=dv->adc_noise->MaxWeight2-dv->adc_noise->MinWeight2;
			printf("NoiseWeightFR %d | ", dv->adc_noise->DeltaWeight2);
			
			if (weightValue3>dv->adc_noise->MaxWeight3) dv->adc_noise->MaxWeight3=weightValue3;
			if (weightValue3<dv->adc_noise->MinWeight3) dv->adc_noise->MinWeight3=weightValue3;
			dv->adc_noise->DeltaWeight3=dv->adc_noise->MaxWeight3-dv->adc_noise->MinWeight3;
			printf("NoiseWeightBL %d | ", dv->adc_noise->DeltaWeight3);
			
			if (weightValue4>dv->adc_noise->MaxWeight4) dv->adc_noise->MaxWeight4=weightValue4;
			if (weightValue4<dv->adc_noise->MinWeight4) dv->adc_noise->MinWeight4=weightValue4;
			dv->adc_noise->DeltaWeight4=dv->adc_noise->MaxWeight4-dv->adc_noise->MinWeight4;
			printf("NoiseWeightBR %d\n", dv->adc_noise->DeltaWeight4);
		}
		return weightValue;
	}
}

double readWeightSeparate(double* values, struct Daemon_Values *dv){
	if (dv->runningMode->simulationMode){
		if (!dv->state->scaleReady) {
			return 0.0;
		} else {
			int deltaW = dv->newCommandValues->weight-dv->oldCommandValues->weight;
			double weightValue = dv->oldCommandValues->weight;
			if (deltaW<0) {
				--weightValue;
			} else if (deltaW==0) {
				//Nothing
			} else if (deltaW<=10) {
				weightValue = weightValue + 2;
			} else if (deltaW<=50) {
				weightValue = weightValue + 5;
			} else {
				weightValue = weightValue + 10;
			}
			weightValue += dv->state->referenceForce;
			if (dv->settings->debug_enabled){printf("readWeight, new Value is %f\n", weightValue);}
			
			if (values != NULL){
				values[0] = weightValue;
				values[1] = weightValue;
				values[2] = weightValue;
				values[3] = weightValue;
			}
			return weightValue;
		}
	} else {
		uint32_t weightValue1 = readADC(dv->adc_config->ADC_LoadCellFrontLeft);
		uint32_t weightValue2 = readADC(dv->adc_config->ADC_LoadCellFrontRight);
		uint32_t weightValue3 = readADC(dv->adc_config->ADC_LoadCellBackLeft);
		uint32_t weightValue4 = readADC(dv->adc_config->ADC_LoadCellBackRight);
		
		if (values != NULL){
			values[0] = (double)weightValue1 * dv->forceCalibration->scaleFactor;
			values[1] = (double)weightValue2 * dv->forceCalibration->scaleFactor;
			values[2] = (double)weightValue3 * dv->forceCalibration->scaleFactor;
			values[3] = (double)weightValue4 * dv->forceCalibration->scaleFactor;
		}
		
		uint32_t weightValueSum = (weightValue1+weightValue2+weightValue3+weightValue4) / 4;
		
		double weightValue = (double)weightValueSum;
		weightValue *=dv->forceCalibration->scaleFactor;
		weightValue = roundf(weightValue);
		if (dv->settings->debug_enabled){
			if (values != NULL){
				printf("readWeight, new Value is %d / %f (%d, %d, %d, %d / %f, %f, %f, %f)\n", weightValueSum, weightValue, weightValue1, weightValue2, weightValue3, weightValue4, values[0], values[1], values[2], values[3]);
			} else {
				printf("readWeight, new Value is %d / %f (%d, %d, %d, %d)\n", weightValueSum, weightValue, weightValue1, weightValue2, weightValue3, weightValue4);
			}
		}
		return weightValue;
	}
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