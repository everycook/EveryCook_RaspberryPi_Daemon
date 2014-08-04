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
#ifndef DAEMONSTRUCTS_H_
#define DAEMONSTRUCTS_H_

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
	uint8_t ADC_update_rate;
	
	uint32_t ADC_ConfigReg[6];
	bool inverse[6];
	uint8_t PressHystereis;
	uint8_t TempHystereis;
	bool restarting_adc;
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

struct ADC_Value {
	uint32_t adc_value;
	double value;
	double valueByOffset;
};

struct ADC_Values {
	struct ADC_Value LoadCellFrontLeft;
	struct ADC_Value LoadCellFrontRight;
	struct ADC_Value LoadCellBackLeft;
	struct ADC_Value LoadCellBackRight;
	struct ADC_Value Press;
	struct ADC_Value Temp;
	
	struct ADC_Value Weight;
};

struct I2C_Config {
	uint8_t i2c_motor;				//ENLEVE
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
	
	uint8_t i2c_servo_stay_open;
};

struct I2C_solenoid_Values {//ENLEVE
	uint16_t i2c_solenoid_open;
	uint16_t i2c_solenoid_closed;
	
	float currentValue;
	uint16_t i2c_solenoid_value;
	uint8_t solenoidOpen;
};

struct I2C_Motor_Values {//ENLEVE
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

struct Button_Config {
	uint8_t button_pin[3];
	uint8_t button_inverse[3];
};

struct Button_Values {
	uint32_t button[3];
	uint32_t buttonPressedTime[3];
	uint32_t buttonOnTime[3];
	uint32_t buttonOffTime[3];
	uint32_t actionStartedTime[3];
};

struct Command_Values {
	double temp;
	int32_t press;
	uint8_t motorRpm; //0-255 ENLEVE
	uint32_t motorOn; // ENLEVE
	uint32_t motorOff;// ENLEVE
	double weight;
	uint32_t time;
	uint32_t mode;
	int32_t stepId;
};

struct Time_Values {
	uint32_t runTime; //we need a runtime in seconds
	uint32_t lastRunTime;
	uint32_t runTimeMillis;
	
	time_t nowTime;
	struct tm *localTime;
	
	uint32_t stepEndTime;
	uint32_t stepStartTime;
	uint32_t middlewareConnectTime;
	uint32_t motorStartTime; //When did we start the motor? ENLEVE
	uint32_t motorStopTime; //When did we stop the motor? ENLEVE
	uint32_t remainTime;
	uint32_t beepEndTime; //when to stop the beeper// ENLEVE
	uint32_t lastBlinkTime;
	
	time_t lastFileChangeTime;
	uint32_t simulationUpdateTime;
	uint32_t lastLogSaveTime; //GET
	uint32_t lastStatusTime;
	uint32_t heaterStartTime; //when did we start the heater //ENLEVE
	uint32_t heaterStopTime; //when did we stop the heater  //ENLEVE
	
	uint32_t servoStayEndTime; //when to close the servo
	int32_t lastWeightUpdateTime; //when to close the servo
};

struct Running_Mode {
	bool normalMode;
	bool calibration;
	bool measure_noise;
	bool test_7seg;
	bool test_servo;
	bool test_heat_led;
	bool test_motor;
	bool test_buttons;
	bool test_adc;
	bool test_heating_power;
	bool test_heating_press;
	bool test_serial;
	
	bool simulationMode;//GET
	bool simulationModeShow7Segment;
};

struct Settings {
	uint32_t shieldVersion;//GET

	uint8_t logSaveInterval; //setting for logging interval in seconds
	uint32_t logLines; //setting for amount of rows logging
	uint8_t DeleteLogOnStart;  //delete log at start saves disk space set 0 to keep log

	double weightReachedMultiplier;
	
	uint8_t BeepWeightReached;
	uint8_t BeepStepEnd; //ENLEVE
	
	//Values for change 7seg display
	uint32_t LowTemp;
	int32_t LowPress;

	//delay for normal operation and scale mode
	uint32_t LongDelay;//GET
	uint32_t ShortDelay;
	
	//Options
	bool doRememberBeep;//ENLEVE
	
	char* middlewareHostname;
	int middlewarePortno;
	bool useMiddleware;
	bool useFile;
	
	bool debug_enabled;//GET
	bool debug3_enabled;//GET
	bool debug4_enabled;//GET
	bool use_spi_dev;

	uint16_t test_servo_min;
	uint16_t test_servo_max;
	
	uint16_t test_ADC_update_rate;
	uint16_t test_ADC_offsetCalibration;
	uint16_t test_ADC_fullScaleCalibration;
	
	char *configFile;
	char *calibrationFile;
	char *commandFile;
	char *statusFile;
	char *logFile;
	char *hourCounterFile;
	char *installPath;
	char *atmelDevicePath;
};

struct State {
	bool running;
	bool dataChanged;
	bool timeChanged;

	uint32_t Delay;

	double referenceForce; //the reference to get the zero of the scale
	bool scaleReady;

	bool heatPowerStatus; //Induction heater power 0=off >0=on //ENELVE

	bool isBuzzing; //to be sure first send a off to buzzer//ENLEVE

	char oldSegmentDisplay;
	bool blinkState;

	int sockfd;
	char* oldSendedString;
	uint32_t logLineNr;
	
	char middlewareBuffer[256];

	char TotalUpdate[512];
	uint32_t value[15];
	char names[15][10];
	char actionText[255];
	char *language;
	bool alwaysReadMode;
	
	FILE *logFilePointer;
	char** logLines;
	
	double weightValues[4];
	uint8_t weightPercent;
	bool lidClosed;
	bool lidLocked;
	bool pusherLocked;
};

struct Heater_Led_Values {//ENELVE
	bool hasPower;
	bool isOn;
	bool isModeHeating;
	bool isModeKeepwarm;
	uint8_t level;
	uint32_t hasPowerLedOnLastTime;
	uint32_t hasPowerLedOffLastTime;
	uint32_t isOnLastTime;
	
	bool hasError;
	bool noPanError;
	bool IGBTTempToHeightError;
	bool tempSensorError;
	bool IGBTSensorError;
	bool voltageToHeightError;
	bool voltageToLowError;
	bool bowOutOfWaterError;
	uint32_t errorLastTime;
	
	bool lastMultiplexer;
	bool lastDiffMultiplexer;
	bool ledsOffBlinkState;
	
	uint32_t multiplexerLastTime1;
	
	char* errorMsg;
	uint32_t ledValues[6];
};

struct HourCounter {
	char identifier[4];
	uint32_t daemon;
	uint32_t heater;//ENLEVE
	uint32_t motor;//ENLEVE
};

struct Daemon_Values {
	struct ADC_Config *adc_config;
	struct ADC_Noise_Values *adc_noise;

	struct ADC_Calibration *forceCalibration;
	struct ADC_Calibration *pressCalibration;
	struct ADC_Calibration *tempCalibration;
	struct I2C_Config *i2c_config;

	struct I2C_Servo_Values *i2c_servo_values;
	struct I2C_solenoid_Values *i2c_solenoid_values;
	struct I2C_Motor_Values *i2c_motor_values;

	struct Command_Values *newCommandValues;
	struct Command_Values *currentCommandValues;
	struct Command_Values *oldCommandValues;

	struct Time_Values *timeValues;

	struct Running_Mode *runningMode;

	struct Settings *settings;
	struct State *state;
	struct Heater_Led_Values *heaterStatus;
	struct HourCounter *hourCounter;
	struct Button_Config *buttonConfig;
	struct Button_Values *buttonValues;
	struct ADC_Values *adc_values;
};

#endif /*----DAEMONSTRUCTS_H_-----*/
