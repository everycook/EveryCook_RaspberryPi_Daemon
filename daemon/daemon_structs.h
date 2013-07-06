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

#include <time.h>

#include "bool.h"

struct ADC_Config {	
	uint8_t ADC_LoadCellFrontLeft;
	uint8_t ADC_LoadCellFrontRight;
	uint8_t ADC_LoadCellBackLeft;
	uint8_t ADC_LoadCellBackRight;
	uint8_t ADC_Press;
	uint8_t ADC_Temp;

	uint8_t ADC_ref; //0=REFIN1, 1=REFIN2, 2=Internal 1.17V, 3=reserved
};

struct ADC_Noise_Values {	
	uint32_t MaxWeight1;
	uint32_t MinWeight1;
	uint32_t DeltaWeight1;
	uint32_t MaxWeight2;
	uint32_t MinWeight2;
	uint32_t DeltaWeight2;
	uint32_t MaxWeight3;
	uint32_t MinWeight3;
	uint32_t DeltaWeight3;
	uint32_t MaxWeight4;
	uint32_t MinWeight4;
	uint32_t DeltaWeight4;
	uint32_t MaxTemp;
	uint32_t MinTemp;
	uint32_t DeltaTemp;
	uint32_t MaxPress;
	uint32_t MinPress;
	uint32_t DeltaPress;
};

struct ADC_Calibration {
	double scaleFactor;
	double offset;
	
	uint32_t ADC1;
	double Value1;
	uint32_t ADC2;
	double Value2;
};

struct I2C_Config {
	uint8_t i2c_motor;
	uint8_t i2c_servo;
	uint8_t i2c_7seg_top;			//SegAPin
	uint8_t i2c_7seg_top_left;		//SegBPin
	uint8_t i2c_7seg_top_right;		//SegFPin
	uint8_t i2c_7seg_center;		//SegGPin
	uint8_t i2c_7seg_bottom_left;	//SegEPin
	uint8_t i2c_7seg_bottom_right;	//SegCPin
	uint8_t i2c_7seg_bottom;		//SegDPin
	uint8_t i2c_7seg_period;		//SegDPPin
};

struct I2C_Servo_Values {
	uint16_t i2c_servo_open;
	uint16_t i2c_servo_closed;
	
	uint8_t destOpenPercent;
	uint8_t steps;
	float stepSize;
	uint8_t stepPercentSize;
	
	uint8_t currentStep;
	float currentValue;
	uint16_t i2c_servo_value;
	uint8_t servoOpen;
};

struct I2C_Motor_Values {
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

struct Command_Values {
	double temp;
	int32_t press;
	uint8_t motorRpm; //0-255 
	uint32_t motorOn;
	uint32_t motorOff;
	double weight;
	uint32_t time;
	uint32_t mode;
	uint32_t stepId;
};

struct Time_Values {
	uint32_t runTime; //we need a runtime in seconds
	uint32_t lastRunTime;
	
	time_t nowTime;
	struct tm *localTime;
	
	uint32_t stepEndTime;
	uint32_t stepStartTime;
	uint32_t middlewareConnectTime;
	uint32_t motorStartTime; //When did we start the motor?
	uint32_t remainTime;
	uint32_t beepEndTime; //when to stop the beeper
	uint32_t lastBlinkTime; //when to stop the beeper
	
	time_t lastFileChangeTime;
	uint32_t simulationUpdateTime;
	uint32_t lastLogSaveTime;
	uint32_t lastStatusTime;
};
struct Running_Mode {
	bool normalMode;
	bool calibration;
	bool measure_noise;
	bool test_7seg;
	bool test_servo;
	bool test_heat_led;
	bool test_motor;
	
	bool simulationMode;
	bool simulationModeShow7Segment;
};

struct Settings {
	uint32_t shieldVersion;

	uint8_t logSaveInterval; //setting for logging interval in seconds
	uint32_t logLines; //setting for amount of rows logging
	uint8_t DeleteLogOnStart;  //delete log at start saves disk space set 0 to keep log

	double weightReachedMultiplier;
	
	uint8_t BeepWeightReached;
	uint8_t BeepStepEnd;
	
	//Values for change 7seg display
	uint32_t LowTemp;
	int32_t LowPress;

	//delay for normal operation and scale mode
	uint32_t LongDelay;
	uint32_t ShortDelay;
	
	//Options
	bool doRememberBeep;
	
	char* middlewareHostname;
	int middlewarePortno;
	bool useMiddleware;
	bool useFile;
	
	bool debug_enabled;
	bool debug3_enabled;

	uint16_t test_servo_min;
	uint16_t test_servo_max;
	
	char *configFile;
	char *calibrationFile;
	char *commandFile;
	char *statusFile;
	char *logFile;

};

struct State {
	bool running;
	bool dataChanged;
	bool timeChanged;

	uint32_t Delay;

	double referenceForce; //the reference to get the zero of the scale
	bool scaleReady;

	bool heatPowerStatus; //Induction heater power 0=off >0=on

	bool isBuzzing; //to be sure first send a off to buzzer

	char oldSegmentDisplay;
	bool blinkState;

	int sockfd;
	char* oldSendedString;
	uint32_t logLineNr;
	
	char middlewareBuffer[256];

	char TotalUpdate[512];
	uint32_t value[15];
	char names[15][10];
	
	FILE *logFilePointer;
	char** logLines;
};

struct Heater_Led_Values {
	bool hasPower;
	bool isHeating;
	bool noPan;
	uint32_t hasPowerLedLastTime;
	uint32_t isHeatingLedLastTime;
	uint32_t noPanLedLastTime;
	
	bool tempSensorError;
	bool IGBTSensorError;
	bool voltageToHeightError;
	bool voltageToLowError;
	bool bowOutOfWaterError;
	bool IGBTTempToHeightError;
	uint32_t tempSensorErrorLedLastTime;
	uint32_t IGBTSensorErrorLedLastTime;
	uint32_t voltageToHeightErrorLedLastTime;
	uint32_t voltageToLowErrorLedLastTime;
	uint32_t bowOutOfWaterErrorLedLastTime;
	uint32_t IGBTTempToHeightErrorLedLastTime;
	
	char* errorMsg;
	uint32_t ledValues[6];
};

struct Daemon_Values {
	struct ADC_Config *adc_config;
	struct ADC_Noise_Values *adc_noise;

	struct ADC_Calibration *forceCalibration;
	struct ADC_Calibration *pressCalibration;
	struct ADC_Calibration *tempCalibration;
	struct I2C_Config *i2c_config;

	struct I2C_Servo_Values *i2c_servo_values;
	struct I2C_Motor_Values *i2c_motor_values;

	struct Command_Values *newCommandValues;
	struct Command_Values *currentCommandValues;
	struct Command_Values *oldCommandValues;

	struct Time_Values *timeValues;

	struct Running_Mode *runningMode;

	struct Settings *settings;
	struct State *state;
	struct Heater_Led_Values *heaterStatus;
};