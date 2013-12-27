/*
This is the EveryCook Raspberry Pi daemon. It reads inputs from the EveryCook Raspberry Pi shield and controls the outputs.
EveryCook is an open source platform for collecting all data about food and make it available to all kinds of cooking devices.

This program is copyright (C) by EveryCook. Written by Samuel Werder, Peter Turczak and Alexis Wiasmitinow.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

// See GPLv3.htm in the main folder for details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <math.h>

#include <softPwm.h>
#include <wiringPi.h>

#include <string.h>

#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <pthread.h>

#include "bool.h"
#include "modes.h"
#include "basic_functions.h"
#include "convertFunctions.h"
#include "middlewareSocket.h"
#include "daemon.h"

#include "virtualspi.h"
#include "ad7794_interface.h"


#include "daemon_structs.h"
#include "hardwareFunctions.h"


const uint32_t MIN_STATUS_INTERVAL = 20;

const uint8_t ALLWAYS_STOP_BUZZING = 0;

const uint8_t dataType = TYPE_TEXT;

const double SECONDS_PER_HOUR = 3600; //60*60

const uint8_t HOUR_COUNTER_VERSION = 0;

struct ADC_Config adc_config = {0,1,2,3,4,5, 0,8, {0x710, 0x711, 0x712, 0x713, 0x714, 0x115}, {0,0,0,0,0,0}};
struct ADC_Noise_Values adc_noise = {0,1000000000,0, 0,1000000000,0, 0,1000000000,0, 0,1000000000,0, 0,1000000000,0, 0,1000000000,0};
struct ADC_Values adc_values;

struct ADC_Calibration forceCalibration = {};
struct ADC_Calibration pressCalibration = {};
struct ADC_Calibration tempCalibration = {};
struct I2C_Config i2c_config = {I2C_MOTOR, I2C_SERVO, I2C_7SEG_TOP, I2C_7SEG_TOP_LEFT, I2C_7SEG_TOP_RIGHT, I2C_7SEG_CENTER, I2C_7SEG_BOTTOM_LEFT, I2C_7SEG_BOTTOM_RIGHT, I2C_7SEG_BOTTOM, I2C_7SEG_PERIOD};

struct I2C_Servo_Values i2c_servo_values = {I2C_VALVE_OPEN_VALUE, I2C_VALVE_CLOSED_VALUE, 0, 0, 0, 0, 0, I2C_VALVE_CLOSED_VALUE, 0};
struct I2C_solenoid_Values i2c_solenoid_values = {I2C_VALVE_OPEN_VALUE, I2C_VALVE_CLOSED_VALUE, I2C_VALVE_CLOSED_VALUE, 0, 0};
struct I2C_Motor_Values i2c_motor_values = {10,10, 0,0,0.0, 0,0,0,10}; //last value (motorRpm) set to 10 so it will be reset to 0 on startup


struct Command_Values newCommandValues = {0,0,0,0,0,0,0,0,-2};
struct Command_Values currentCommandValues = {0,0,0,0,0,0,0,0,-2};
struct Command_Values oldCommandValues = {0,0,0,0,0,0,0,0,-2};

struct Time_Values timeValues = {};

struct Running_Mode runningMode = {true, false, false, false, false, false, false, false, false,  false, false};

struct Settings settings = {0, 5,0,1, 1.0, 1,1, 40,10, 500,1, false, "127.0.0.1",8000,true,true, false,false,false,false, 100,800, 8,0x0000,0x0000, "config","/opt/EveryCook/daemon/calibration","/dev/shm/command","/dev/shm/status","/var/log/EveryCook_Daemon.log","/opt/EveryCook/daemon/hourCounter","/opt/EveryCook/"};

struct State state = {true,true,true, 1/*setting.ShortDelay*/, 0,false, false, true, ' ',false, -1,"", 0};

struct Heater_Led_Values heaterStatus = {};

struct Daemon_Values daemon_values;

//struct HourCounter hourCounter = {['H','C','V', HOUR_COUNTER_VERSION], 0.0, 0.0, 0.0};
struct HourCounter hourCounter = {"HCV\0", 0.0, 0.0, 0.0};

struct Button_Config buttonConfig = {{17,27,22},{0,0,0}};
struct Button_Values buttonValues;

pthread_t threadHeaterLedReader;
pthread_t threadReadADCValues;
pthread_t threadHandleButtons;


uint8_t HeaterErrorPattern[7][6] = {{0, 0, 0, 0, 0, 0},{0, 0, 1, 1, 1, 1},{1, 0, 1, 1, 1, 1},{0, 1, 1, 1, 1, 1},{1, 0, 0, 1, 1, 1},{0, 0, 0, 1, 1, 1}};
char* HeaterErrorCodes[] = {"No Pan", "Temperature Sensor out of work", "IGBT Sensor out of work","Voltage To High(>260V)", "Voltage to Low(<85V)","The bow out of water","IGBT Temperature To High"};
uint8_t HeaterErrorTimeout[7] = {1, 4, 4, 2, 2, 3, 2};

int parseParams(int argc, const char* argv[]);
void defineSignalHandler();
void handleSignal(int signum);
void clearHourCounter();
void initOutputFile(void);
bool checkForInput();
void doOutput();

void updateMotorTime();
void updateHeaterTime();

void *heaterLedEvaluation(void *ptr);
void *readADCValues(void *ptr);
void *handleButtons(void *ptr);
void speak(char* text);
void *speakThreadFunc(void *ptr);

/*
The Modes:
0 = Standby
//cold operations
1 = Cut, 2 = Scale
//hot operations
10 = Heatup, 11 = Cook, 12 = Cooldown, 13 = Reduce (not implemented yet) 14-19 (not implemented yet)
//pressurizes operations
20 = Pressurize, 21 = Pressurecook, 22 = Pressuredown, 23 = Pressureoff 24-29 (not implemented yet)
//status messages
30 = Hot, 31 = Pressurized, 32 = Cold, 33 = Pressureless, 34 = weight reached 35-39 (not implemented yet)
//error messages
40 = input error, 41 = emergency shutdown (both not implemented yet) 42 = motor overload
//special commands
50 = watchdog test
*/

int main(int argc, const char* argv[]){
	daemon_values.adc_config = &adc_config;
	daemon_values.adc_noise = &adc_noise;
	daemon_values.forceCalibration = &forceCalibration;
	daemon_values.pressCalibration = &pressCalibration;
	daemon_values.tempCalibration = &tempCalibration;
	daemon_values.i2c_config = &i2c_config;
	daemon_values.i2c_servo_values = &i2c_servo_values;
	daemon_values.i2c_motor_values = &i2c_motor_values;
	daemon_values.newCommandValues = &newCommandValues;
	daemon_values.currentCommandValues = &currentCommandValues;
	daemon_values.oldCommandValues = &oldCommandValues;
	daemon_values.timeValues = &timeValues;
	daemon_values.runningMode = &runningMode;
	daemon_values.settings = &settings;
	daemon_values.state = &state;
	daemon_values.heaterStatus = &heaterStatus;
	daemon_values.hourCounter = &hourCounter;
	daemon_values.buttonConfig = &buttonConfig;
	daemon_values.buttonValues = &buttonValues;
	daemon_values.adc_values = &adc_values;
	
	printf("starting EveryCook daemon...\n");
	if (settings.debug_enabled){printf("main\n");}
	
	int result=parseParams(argc, argv);
	if (result != -1){
		return result;
	}
	defineSignalHandler();
	state.alwaysReadMode = false;
	
	StringClean(state.TotalUpdate, 512);
	ReadConfigurationFile();
	ReadCalibrationFile();
	
	initHardware(settings.shieldVersion, buttonConfig.button_pin, buttonConfig.button_inverse);
	delay(30);
	
	initOutputFile();
	state.Delay = settings.LongDelay;
	
	resetValues();
	state.referenceForce = forceCalibration.offset; //for default reference Force in calibration mode
	SegmentDisplaySimple(' ', &state, &i2c_config);
	
	if (settings.debug_enabled){printf("commandFile is: %s\n", settings.commandFile);}
	if (settings.debug_enabled){printf("statusFile is: %s\n", settings.statusFile);}
	
	//remove commandfile so no unexpectet action will be done
	if (remove(settings.commandFile) != 0){
		printf("Error while remove old commandfile: %s\n", settings.commandFile);
	}
		
	timeValues.runTime = time(NULL); //TODO WIA CHANGE THIS
	printf("runtime is now: %d \n", timeValues.runTime);
	timeValues.stepStartTime = timeValues.runTime; //TODO WIA CHANGE THIS
	
	if (runningMode.normalMode){
		pthread_create(&threadHeaterLedReader, NULL, heaterLedEvaluation, NULL); //(void*) message1
		pthread_create(&threadReadADCValues, NULL, readADCValues, NULL);
		if(settings.shieldVersion != 1){
			pthread_create(&threadHandleButtons, NULL, handleButtons, NULL);
		}
		while (state.running){
			if (settings.debug_enabled){printf("main loop...\n");}
			if (timeValues.lastRunTime != timeValues.runTime){
				state.timeChanged = true;
				timeValues.lastRunTime = timeValues.runTime;
			}
			bool valueChanged = checkForInput();
			
			timeValues.runTime = time(NULL); //TODO WIA CHANGE THIS
			timeValues.runTimeMillis = millis();
			
			if (valueChanged){
				if (settings.debug_enabled){printf("valueChanged=true\n");}
				//TODO: timeValues.lastFileChangeTime = timeValues.runTime;
				evaluateInput();
			}
			ProcessCommand();
			currentCommandValues.time = timeValues.runTime - timeValues.stepStartTime;
			
			doOutput();
			
			if (settings.debug_enabled){printf("main loop end.\n");}
			delay(state.Delay);
		}
		HeatOff(&daemon_values);
		//cheating so it will realy stop motor
		i2c_motor_values.motorRpm = i2c_motor_values.i2c_motor_speed_min;
		i2c_motor_values.destRpm = i2c_motor_values.motorRpm;
		setMotorRPM(0, &daemon_values);
		
		if (settings.shieldVersion < 3){
			setServoOpen(0, 1, 0, &daemon_values);
		} else {
			setSolenoidOpen(false, &daemon_values);
		}
		if(settings.shieldVersion != 1){
			pthread_join(threadHandleButtons, NULL);
		}
		pthread_join(threadReadADCValues, NULL);
		pthread_join(threadHeaterLedReader, NULL);
	} else if (runningMode.calibration || runningMode.measure_noise){
		if (runningMode.calibration) {
			pthread_create(&threadHeaterLedReader, NULL, heaterLedEvaluation, NULL); //(void*) message1
		}
		pthread_create(&threadReadADCValues, NULL, readADCValues, NULL);
		while (state.running){
			timeValues.runTime = time(NULL);
			timeValues.runTimeMillis = millis();
			//printf("we are in calibration mode, be careful!\n Heat will turn on automatically if switch is on!\n Use switch to turn heat off.\n");	
			if (runningMode.calibration) HeatOn(&daemon_values);
			if (settings.debug_enabled || runningMode.calibration || settings.debug3_enabled || runningMode.measure_noise) {printf("time %8d | ", timeValues.runTimeMillis);}
			
			if (settings.debug_enabled || runningMode.calibration || settings.debug3_enabled){printf("Temp %d dig %.0f °C | ", adc_values.Temp.adc_value, adc_values.Temp.valueByOffset);}
			if (runningMode.measure_noise) {printf("NoiseTemp %d | ", adc_noise.DeltaTemp);}
			
			if (settings.debug_enabled || runningMode.calibration || settings.debug3_enabled){printf("Press %d digits %.0f kPa | ", adc_values.Press.adc_value, adc_values.Press.valueByOffset);}
			if (runningMode.measure_noise){printf("NoisePress %d |", adc_noise.DeltaPress);}
			
			if (settings.debug_enabled || runningMode.calibration){printf("Weight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", adc_values.Weight.adc_value, adc_values.Weight.value, adc_values.Weight.valueByOffset, adc_values.LoadCellFrontLeft.adc_value, adc_values.LoadCellFrontRight.adc_value, adc_values.LoadCellBackLeft.adc_value, adc_values.LoadCellBackRight.adc_value);}			
			if (runningMode.measure_noise){
				printf("NoiseWeightFL %d | ", adc_noise.DeltaWeight1);
				printf("NoiseWeightFR %d | ", adc_noise.DeltaWeight2);
				printf("NoiseWeightBL %d | ", adc_noise.DeltaWeight3);
				printf("NoiseWeightBR %d\n", adc_noise.DeltaWeight4);
			}
			SegmentDisplay();
			delay(state.Delay);
		}
		pthread_join(threadReadADCValues, NULL);
		if (runningMode.calibration){
			if (runningMode.calibration) HeatOff(&daemon_values);
			pthread_join(threadHeaterLedReader, NULL);
		}
	} else if (runningMode.test_7seg){
		while (state.running){
			blink7Segment(&i2c_config);
			delay(state.Delay);
		}
	} else if (runningMode.test_servo){
		int16_t diff = settings.test_servo_max-settings.test_servo_min;
		int16_t step = diff / 20;
		uint16_t value=settings.test_servo_min;
		if (step == 0){
			printf("%d\n",value);
			writeI2CPin(i2c_config.i2c_servo, value);
		} else if (step>0){
			printf(">0 step: %d\n",step);
			while (value<=settings.test_servo_max && state.running){
				printf("%d\n",value);
				writeI2CPin(i2c_config.i2c_servo, value);
				delay(500);
				value=value+step;
			}
		} else {
			printf("<0 step: %d\n",step);
			while (value>=settings.test_servo_max && state.running){
				printf("%d\n",value);
				writeI2CPin(i2c_config.i2c_servo, value);
				delay(500);
				value=value+step;
			}
		}
		delay(2000);
	} else if (runningMode.test_heat_led){
		state.Delay = 10;
		uint32_t led[6];
		led[0] = 0;
		led[1] = 0;
		led[2] = 0;
		led[3] = 0;
		led[4] = 0;
		led[5] = 0;
		
		heaterStatus.errorMsg = NULL;
		heaterStatus.isOn = false;
		heaterStatus.hasError = false;
		state.heatPowerStatus = false;
		HeatOn(&daemon_values);
		
		if (settings.shieldVersion == 1){
			uint32_t lastHeatLedTime = time(NULL);
			uint32_t heatLedValuesSameCount = 0;
			uint32_t noPanLedLastTime = 0;
			while(state.running){
				timeValues.runTime = time(NULL);
				led[0] = readSignPin(0);
				led[1] = readSignPin(1);
				led[2] = readSignPin(2);
				led[3] = readSignPin(3);
				led[4] = readSignPin(4);
				led[5] = readSignPin(5);
				
				bool different = false;
				
				if (led[3] == 0){
					heaterStatus.isOnLastTime = timeValues.runTime;
					if (!heaterStatus.isOn){
						heaterStatus.isOn = true;
						different = true;
					}
				} else if (heaterStatus.isOn && heaterStatus.isOnLastTime + 3 < timeValues.runTime){
					heaterStatus.isOn = false;
						different = true;
				}
				
				if (led[0] == 1 && led[1] == 1 && led[2] == 1 && led[3] == 1 && led[4] == 1 && led[5] == 1){
					if (heaterStatus.hasPower && heaterStatus.hasPowerLedOnLastTime + 4 < timeValues.runTime){
						heaterStatus.hasPower = false;
						different = true;
					}
				} else {
					heaterStatus.hasPowerLedOnLastTime = timeValues.runTime;
					if (!heaterStatus.hasPower){
						heaterStatus.hasPower = true;
						different = true;
					}
				}
				
				if (led[0] == 0 && led[1] == 0 && led[2] == 0 && led[3] == 0 && led[4] == 0 && led[5] == 0){
					noPanLedLastTime = timeValues.runTime;
					if (!heaterStatus.noPanError){
						heaterStatus.noPanError = true;
						different = true;
					}
				} else {
					if (heaterStatus.noPanError && noPanLedLastTime + 1 < timeValues.runTime){
						heaterStatus.noPanError = false;
						different = true;
					}
				}
				
				uint8_t i = 0;
				for (; i<6; ++i){
					if (heaterStatus.ledValues[i] != led[i]){
						different = true;
						break;
					}
				}
				
				if (different){
					char* errorMsg;
					bool errorFound = false;
					
					uint8_t errNr = 1;
					for (; errNr<7; ++errNr){
						bool isSame = true;
						for (i = 0; i<6; ++i){
							if (HeaterErrorPattern[errNr][i] != led[i]){
								isSame = false;
								break;
							}
						}
						if (isSame){
							errorFound = true;
							errorMsg = HeaterErrorCodes[errNr];
							break;
						}
					}
					if (errorFound){
						heaterStatus.errorMsg = errorMsg;
						printf("leds:\t%d\t%d\t%d\t%d\t%d\t%d\t|\thas power:%d\tis heating:%d\tnoPan:%d\t\telapsed Time:%d\tprev amount:%d\t\tError:%s\n",led[0],led[1],led[2],led[3],led[4],led[5], heaterStatus.hasPower,heaterStatus.isOn,heaterStatus.noPanError, timeValues.runTime - lastHeatLedTime, heatLedValuesSameCount, errorMsg);
					} else {
						printf("leds:\t%d\t%d\t%d\t%d\t%d\t%d\t|\thas power:%d\tis heating:%d\tnoPan:%d\t\telapsed Time:%d\tprev amount:%d\n",led[0],led[1],led[2],led[3],led[4],led[5], heaterStatus.hasPower,heaterStatus.isOn,heaterStatus.noPanError, timeValues.runTime - lastHeatLedTime, heatLedValuesSameCount);
						heaterStatus.errorMsg = NULL;
					}
					i = 0;
					for (; i<6; ++i){
						heaterStatus.ledValues[i] = led[i];
					}
					lastHeatLedTime = time(NULL);
					heatLedValuesSameCount = 0;
				} else {
					heatLedValuesSameCount = heatLedValuesSameCount+1;
				}
				delay(state.Delay);
			}
		} else if (settings.shieldVersion == 2){
			uint32_t lastHeatLedTime = millis();
			uint32_t heatLedValuesSameCount = 0;
			while(state.running){
				bool different = false;
				uint8_t i = 0;
				bool newHasError = false;
				
				uint32_t leds, led1,led2,led3;
				uint32_t multiplexer = readSignPin(0);
				delayMicroseconds(200);
				leds = readSignPin(0);
				led1 = readSignPin(1);
				led2 = readSignPin(2);
				led3 = readSignPin(3);
				delayMicroseconds(200);
				timeValues.runTimeMillis = millis();
				if (multiplexer == leds && leds == readSignPin(0) && led1 == readSignPin(1) && led2 == readSignPin(2) && led3 == readSignPin(3)){
					if (multiplexer==0){
						led[0] = led1;
						led[1] = led2;
						led[2] = led3;
						if (heaterStatus.hasPower && (timeValues.runTimeMillis - heaterStatus.multiplexerLastTime1) > 200){
							//multiplexer value not changed for a amount of time -> No Power
							led[3] = 0;
							led[4] = 0;
							led[5] = 0;
						}
					} else {
						led[3] = led1;
						led[4] = led2;
						led[5] = led3;
						heaterStatus.multiplexerLastTime1 = timeValues.runTimeMillis;
					}
					
					if (settings.debug_enabled){printf("%d\t%d\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",timeValues.runTimeMillis,multiplexer, led[0],led[1],led[2],led[3],led[4],led[5]);}
				} else {
					delay(state.Delay);
					continue;
				}
				bool multiplexerChanged = false;
				if (multiplexer != heaterStatus.lastMultiplexer){
					heaterStatus.lastMultiplexer = multiplexer;
					multiplexerChanged = true;
				}
				
				for (; i<6; ++i){
					if (heaterStatus.ledValues[i] != led[i]){
						different = true;
						break;
					}
				}
				
				if (!different && heaterStatus.lastDiffMultiplexer != multiplexer){
					//OK
					if (settings.debug_enabled){printf("diff changed complete %d\n", multiplexer);}
					heaterStatus.lastDiffMultiplexer = multiplexer;
				} else {
					if (different){
						heaterStatus.lastDiffMultiplexer = multiplexer;
						if (settings.debug_enabled){
							printf("there was a difference on multiplexer %d\n", multiplexer);
							printf("before\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",heaterStatus.ledValues[0],heaterStatus.ledValues[1],heaterStatus.ledValues[2],heaterStatus.ledValues[3],heaterStatus.ledValues[4],heaterStatus.ledValues[5]);
							printf("after\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",led[0],led[1],led[2],led[3],led[4],led[5]);
						}
						i = 0;
						for (; i<6; ++i){
							heaterStatus.ledValues[i] = led[i];
						}
						delay(state.Delay);
						continue;
					} else if (!multiplexerChanged){
						//if (settings.debug_enabled){printf("multiplexer not changed %d\n", multiplexer);}
						delay(state.Delay);
						continue;
					}
				}
				if (settings.debug_enabled){printf("do check\n");}
				if (led[IND_LED_POWER]){
					heaterStatus.hasPowerLedOnLastTime = timeValues.runTimeMillis;
					if (!heaterStatus.hasPower){
						heaterStatus.hasPower = true;
						different = true;
					}
				} else {
					heaterStatus.hasPowerLedOffLastTime = timeValues.runTimeMillis;
					if (heaterStatus.hasPower && ((timeValues.runTimeMillis - heaterStatus.hasPowerLedOnLastTime) > 1000 || (timeValues.runTimeMillis - heaterStatus.multiplexerLastTime1) > 200)){ //if not heating, blinking in about 500ms cycles
						heaterStatus.hasPower = false;
						different = true;
						
						//reset all values, there could not be any state if no power
						heaterStatus.isOn = false;
						heaterStatus.isModeHeating = false;
						heaterStatus.isModeKeepwarm = false;
						heaterStatus.level = 0;
					}
				}
				if (heaterStatus.hasPower){
					if(led[IND_LED_TEMP_MAX] ^ led[IND_LED_TEMP_MIDDLE] ^ led[IND_LED_TEMP_MIN]){
						//one(only one!) of the level leds are on
						if (led[IND_LED_MODE_HEATING] ^ led[IND_LED_MODE_KEEPWARM]){
							heaterStatus.isOnLastTime = timeValues.runTimeMillis;
							heaterStatus.errorMsg = NULL;
							if (!heaterStatus.isOn || heaterStatus.hasError){
								heaterStatus.isOn = true;
								heaterStatus.isModeHeating = led[IND_LED_MODE_HEATING];
								heaterStatus.isModeKeepwarm = led[IND_LED_MODE_KEEPWARM];
								different = true;
							}
							if(led[IND_LED_TEMP_MAX]){
								heaterStatus.level = 3;
							} else if(led[IND_LED_TEMP_MIDDLE]){
								heaterStatus.level = 2;
							} else if(led[IND_LED_TEMP_MIN]){
								heaterStatus.level = 1;
							}
						} else {
							newHasError = true;
						}
					} else {
						newHasError = true;
					}
				}
				char* errorMsg = NULL;
				if(newHasError){
					if(heaterStatus.hasPower){
						if (heaterStatus.isOn && (timeValues.runTimeMillis - heaterStatus.hasPowerLedOffLastTime) < 1000){ //if not heating, blinking in about 500ms cycles
							if (settings.debug_enabled){printf("Set isOn to false\n");}
							heaterStatus.isOn = false;
							different = true;
						} else if (!heaterStatus.isOn && (timeValues.runTimeMillis - heaterStatus.hasPowerLedOffLastTime) > 1000){
							//This one is needed for correct error handling, if it is set to false in couse of error, but is still on
							if (settings.debug_enabled){printf("Set isOn to true\n");}
							heaterStatus.isOn = true;
							different = true;
						}
					}
					if (heaterStatus.isOn){
						if(different || !heaterStatus.hasError || (timeValues.runTimeMillis - heaterStatus.errorLastTime) > 5000){
							if(!led[IND_LED_MODE_HEATING] && !led[IND_LED_MODE_KEEPWARM] && !led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
								//Error leds off blink state
								if (settings.debug_enabled){printf("Error leds off blink state\n");}
								heaterStatus.ledsOffBlinkState = true;
							} else {
								//There could only be on error at a time
								heaterStatus.hasError = true;
								heaterStatus.ledsOffBlinkState = false;
								heaterStatus.level = 0;
								heaterStatus.isModeHeating = false;
								heaterStatus.isModeKeepwarm = false;
								different = true;
								
								if (settings.debug_enabled){printf("check witch error it is...\n");}
								if(led[IND_LED_MODE_HEATING] && led[IND_LED_MODE_KEEPWARM] && led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
									heaterStatus.noPanError = true;
									errorMsg = "No Pan";
								} else if(led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
									heaterStatus.IGBTTempToHeightError = true;
									errorMsg = "IGBT Temperature To High";
								} else if(led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
									heaterStatus.tempSensorError = true;
									errorMsg = "Temperature Sensor out of work";
								} else if(!led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
									heaterStatus.IGBTSensorError = true;
									errorMsg = "IGBT Sensor out of work";
								} else if(!led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
									heaterStatus.voltageToHeightError = true;
									errorMsg = "Voltage To High(>260V)";
								} else if(!led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
									heaterStatus.voltageToLowError = true;
									errorMsg = "Voltage to Low(<85V)";
								} else if(led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
									heaterStatus.bowOutOfWaterError = true;
									errorMsg = "The bow out of water";
								}
								heaterStatus.errorLastTime = timeValues.runTimeMillis;
							}
						}
					}
				} else {
					if(heaterStatus.hasError){
						heaterStatus.hasError = false;
						heaterStatus.noPanError = false;
						heaterStatus.IGBTTempToHeightError = false;
						heaterStatus.tempSensorError = false;
						heaterStatus.IGBTSensorError = false;
						heaterStatus.voltageToHeightError = false;
						heaterStatus.voltageToLowError = false;
						heaterStatus.bowOutOfWaterError = false;
						heaterStatus.ledsOffBlinkState = false;
					}
				}
				if (different){
					if (heaterStatus.hasError){
						if (errorMsg != NULL){
							heaterStatus.errorMsg = errorMsg;
						}
						if (heaterStatus.errorMsg != NULL){
							printf("leds:\t%d\t%d\t%d\t%d\t%d\t%d\t|\thas power:%d\tis heating:%d\tnoPan:%d\t\telapsed Time:%d  \tprev amount:%d\t\tError:%s\n",led[0],led[1],led[2],led[3],led[4],led[5], heaterStatus.hasPower,heaterStatus.isOn,heaterStatus.noPanError, timeValues.runTimeMillis - lastHeatLedTime, heatLedValuesSameCount, heaterStatus.errorMsg);
						}
					} else {
						printf("leds:\t%d\t%d\t%d\t%d\t%d\t%d\t|\thas power:%d\tis heating:%d\tnoPan:%d\t\telapsed Time:%d  \tprev amount:%d\n",led[0],led[1],led[2],led[3],led[4],led[5], heaterStatus.hasPower,heaterStatus.isOn,heaterStatus.noPanError, timeValues.runTimeMillis - lastHeatLedTime, heatLedValuesSameCount);
						heaterStatus.errorMsg = NULL;
					}
					i = 0;
					for (; i<6; ++i){
						heaterStatus.ledValues[i] = led[i];
					}
					lastHeatLedTime = timeValues.runTimeMillis;
					heatLedValuesSameCount = 0;
				} else {
					heatLedValuesSameCount = heatLedValuesSameCount+1;
				}
				delay(state.Delay);
			}
		}
	} else if (runningMode.test_motor){
		int16_t diff = settings.test_servo_max-settings.test_servo_min;
		int16_t step = diff / 10;
		uint16_t value=settings.test_servo_min;
		printf("motor is i2c pin: %d\n", i2c_config.i2c_motor);
		if (step == 0){
			printf("%d\n",value);
			writeI2CPin(i2c_config.i2c_motor, value);
		} else if (step>0){
			printf(">0 step: %d\n",step);
			while (value<=settings.test_servo_max && state.running){
				printf("%d\n",value);
				writeI2CPin(i2c_config.i2c_motor, value);
				delay(500);
				value=value+step;
			}
		} else {
			printf("<0 step: %d\n",step);
			while (value>=settings.test_servo_max && state.running){
				printf("%d\n",value);
				writeI2CPin(i2c_config.i2c_motor, value);
				delay(500);
				value=value+step;
			}
		}
		delay(2000);
		writeI2CPin(i2c_config.i2c_motor, 0);
	} else if (runningMode.test_buttons){
		state.Delay = 10;
		printf("test_buttons\n");
		uint32_t buttonCount = sizeof(buttonValues.button) / sizeof(buttonValues.button[0]);
		printf("buttonCount: %d\n", buttonCount);
		uint32_t but[buttonCount];
		uint8_t i = 0;
		while (state.running){
			timeValues.runTimeMillis = millis();
			i=0;
			for(; i<buttonCount; ++i){
				but[i] = readButton(i);
				if (but[i]){
					if (but[i] != buttonValues.button[i]){
						buttonValues.buttonOnTime[i] = timeValues.runTimeMillis;
					}
					buttonValues.buttonPressedTime[i] = timeValues.runTimeMillis - buttonValues.buttonOnTime[i];
				} else {
					if (but[i] != buttonValues.button[i]){
						buttonValues.buttonOffTime[i] = timeValues.runTimeMillis;
					}
				}
				buttonValues.button[i] = but[i];
				printf("\t%d (pressed: %d, ontime: %d, offtime: %d)", but[i], buttonValues.buttonPressedTime[i], buttonValues.buttonOnTime[i], buttonValues.buttonOffTime[i]);
			}
			printf("\n");
			
			delay(state.Delay);
		}
	} else if (runningMode.test_adc){
		state.Delay = 5;
		printf("test_adc\n");
		
		struct adc_private adc;
		uint8_t adcChannel = 0;
		if (settings.use_spi_dev){
			ad7794_reset(&adc);
			delay(30);
		
			ad7794_write_data(&adc, AD7794_MODE, settings.test_ADC_update_rate & 0x000F);
			ad7794_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
		} else {
			SPIReset();	
			delay(30);
			SPIWrite2Bytes(WRITE_MODE_REG, settings.test_ADC_update_rate & 0x000F);
			SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
		}
		delay(50);
		uint32_t lastTime = millis();
		uint32_t amount = 0;
		uint32_t adcValues[8] = {0,0,0,0,0,0};
		uint32_t adcTime[8] = {0,0,0,0,0,0};
		uint32_t adcLoops[8] = {0,0,0,0,0,0};
		
		if (settings.test_ADC_offsetCalibration != 0 || settings.test_ADC_fullScaleCalibration != 0){
		  if (settings.use_spi_dev){
			printf("before calibration\n");
			for (;adcChannel<8;adcChannel++) {
				ad7794_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
				delay(50);
				uint32_t fullscale = ad7794_read_data_reg(&adc, AD7794_FULLSC);
				uint32_t offset = ad7794_read_data_reg(&adc, AD7794_OFFSET);
				delay(50);
				printf("ADC(%d) fullscale: %d, offset: %d\n", adcChannel, fullscale, offset);
			}
			
			if (settings.test_ADC_offsetCalibration != 0){
				//Offset Calibration
				adcChannel = 0;
				ad7794_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
				ad7794_write_data(&adc, AD7794_MODE, settings.test_ADC_offsetCalibration | (settings.test_ADC_update_rate & 0x000F));
				while (state.running){	
					uint8_t adcState=0;
					ad7794_communicate(&adc, AD7794_STATUS, AD7794_DIRECTION_READ, 1, &adcState);
					if (settings.debug_enabled){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
					if ((adcState & 0x80) == 0){
						//Set ADC Channel & Gain to use
						ad7794_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
						delay(150);
						//System Full-Scale Calibration.
						ad7794_write_data(&adc, AD7794_MODE, settings.test_ADC_offsetCalibration | (settings.test_ADC_update_rate & 0x000F));
						if (adcChannel < 5){
							++adcChannel;
						} else {
							break;
						}
					}
				}
			}
			if (settings.test_ADC_fullScaleCalibration != 0){
				//fullScale Calibration
				adcChannel = 0;
				ad7794_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
				ad7794_write_data(&adc, AD7794_MODE, settings.test_ADC_fullScaleCalibration | (settings.test_ADC_update_rate & 0x000F));
				while (state.running){
					uint8_t adcState=0;
					ad7794_communicate(&adc, AD7794_STATUS, AD7794_DIRECTION_READ, 1, &adcState);
					if (settings.debug_enabled){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
					if ((adcState & 0x80) == 0){
						//Set ADC Channel & Gain to use
						ad7794_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
						delay(150);
						//System Full-Scale Calibration.
						ad7794_write_data(&adc, AD7794_MODE, settings.test_ADC_fullScaleCalibration | (settings.test_ADC_update_rate & 0x000F));
						if (adcChannel < 5){
							++adcChannel;
						} else {
							break;
						}
					}
				}
			}
			
			printf("after calibration\n");
			adcChannel = 0;
			for (;adcChannel<6;adcChannel++) {
				ad7794_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
				delay(50);
				uint32_t fullscale = ad7794_read_data_reg(&adc, AD7794_FULLSC);
				uint32_t offset = ad7794_read_data_reg(&adc, AD7794_OFFSET);
				delay(50);
				printf("ADC(%d) fullscale: %d, offset: %d\n", adcChannel, fullscale, offset);
			}
		  } else {
			printf("before calibration\n");
			for (;adcChannel<8;adcChannel++) {
				SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
				delay(50);
				uint32_t fullscale = SPIRead3Bytes(READ_FULLSCALE_REG);
				uint32_t offset = SPIRead3Bytes(READ_SHIFT_REG);
				delay(50);
				printf("ADC(%d) fullscale: %d, offset: %d\n", adcChannel, fullscale, offset);
			}
			
			if (settings.test_ADC_offsetCalibration != 0){
				//Offset Calibration
				adcChannel = 0;
				SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
				SPIWrite2Bytes(WRITE_MODE_REG, settings.test_ADC_offsetCalibration | (settings.test_ADC_update_rate & 0x000F));
				while (state.running){
					uint8_t adcState = SPIReadByte(READ_STATUS_REG);
					if (settings.debug_enabled){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
					if ((adcState & 0x80) == 0){
						//Set ADC Channel & Gain to use
						SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
						delay(150);
						//System Full-Scale Calibration.
						SPIWrite2Bytes(WRITE_MODE_REG, settings.test_ADC_offsetCalibration | (settings.test_ADC_update_rate & 0x000F));
						if (adcChannel < 5){
							++adcChannel;
						} else {
							break;
						}
					}
				}
			}
			if (settings.test_ADC_fullScaleCalibration != 0){
				//fullScale Calibration
				adcChannel = 0;
				SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
				SPIWrite2Bytes(WRITE_MODE_REG, settings.test_ADC_fullScaleCalibration | (settings.test_ADC_update_rate & 0x000F));
				while (state.running){
					uint8_t adcState = SPIReadByte(READ_STATUS_REG);
					if (settings.debug_enabled){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
					if ((adcState & 0x80) == 0){
						//Set ADC Channel & Gain to use
						SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
						delay(150);
						//System Full-Scale Calibration.
						SPIWrite2Bytes(WRITE_MODE_REG, settings.test_ADC_fullScaleCalibration | (settings.test_ADC_update_rate & 0x000F));
						if (adcChannel < 5){
							++adcChannel;
						} else {
							break;
						}
					}
				}
			}
			
			printf("after calibration\n");
			adcChannel = 0;
			for (;adcChannel<6;adcChannel++) {
				SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
				delay(50);
				uint32_t fullscale = SPIRead3Bytes(READ_FULLSCALE_REG);
				uint32_t offset = SPIRead3Bytes(READ_SHIFT_REG);
				delay(50);
				printf("ADC(%d) fullscale: %d, offset: %d\n", adcChannel, fullscale, offset);
			}
		  }
			uint16_t systemOffset = systemZeroScaleCalibration; //needed because its a "DEFINE" value...
			if (settings.test_ADC_offsetCalibration == systemOffset){
				//forceCalibration.offset=forceCalibration.Value1 - forceCalibration.ADC1*forceCalibration.scaleFactor ;  //offset in g //for default reference force in calibration mode only
				//pressCalibration.offset=pressCalibration.Value1 - pressCalibration.ADC1*pressCalibration.scaleFactor ;  //offset in pa
				//tempCalibration.offset=tempCalibration.Value1 - tempCalibration.ADC1*tempCalibration.scaleFactor ;  //offset in °C
				
				forceCalibration.offset=0 - 0x800001*forceCalibration.scaleFactor;  //offset in g //for default reference force in calibration mode only
				pressCalibration.offset=0 - 0x800001*pressCalibration.scaleFactor;  //offset in pa
				tempCalibration.offset=0 - 0x800001*tempCalibration.scaleFactor;  //offset in °C
			}
		}
		
		if (settings.use_spi_dev){
			ad7794_write_data(&adc, AD7794_MODE, settings.test_ADC_update_rate & 0x000F);
			adcChannel = 0;
			ad7794_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
		} else {
			SPIWrite2Bytes(WRITE_MODE_REG, settings.test_ADC_update_rate & 0x000F);
			adcChannel = 0;
			SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
		}
		while (state.running){
			uint8_t adcState=0;
			if (settings.use_spi_dev){
				ad7794_communicate(&adc, AD7794_STATUS, AD7794_DIRECTION_READ, 1, &adcState);
			} else {
				adcState = SPIReadByte(READ_STATUS_REG);
			}
			if (settings.debug_enabled){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
			if ((adcState & AD7794_STATUS_NOTREADY_MASK) == 0){
				uint32_t data;
				if (settings.use_spi_dev){
					data = ad7794_read_data(&adc);
				} else {
					data = SPIRead3Bytes(READ_DATA_REG);
				}
				//printf("readADC(%d): %d / %06X, loops: %d, time: %d\n", adcChannel, data, data, amount, millis() - lastTime);
				if (adc_config.inverse[adcChannel]){
					data = 0x00FFFFFF - data;
				}
			
				adcValues[adcChannel] = data;
				adcTime[adcChannel] = millis() - lastTime;
				adcLoops[adcChannel] = amount;
				if (adcChannel < 7){
					++adcChannel;
				} else {
					adcChannel = 0;
					uint32_t totalTime = adcTime[0] + adcTime[1] + adcTime[2] + adcTime[3] + adcTime[4] + adcTime[5] + adcTime[6] + adcTime[7];
					
					//printf("%d\tval:\t%d\t%d\t%d\t%d\t%d\t%d\tTime:\t%d\t%d\t%d\t%d\t%d\t%d\tloops:\t%d\t%d\t%d\t%d\t%d\t%d\n", totalTime, adcValues[0], adcValues[1], adcValues[2], adcValues[3], adcValues[4], adcValues[5], adcTime[0], adcTime[1], adcTime[2], adcTime[3], adcTime[4], adcTime[5], adcLoops[0], adcLoops[1], adcLoops[2], adcLoops[3], adcLoops[4], adcLoops[5]);
					//printf("%d\tval:\t%d\t%d\t%d\t%d\t%d\t%d\tTime:\t%d | %d | %d | %d | %d | %d\tloops:\t%d | %d | %d | %d | %d | %d\n", totalTime, adcValues[0], adcValues[1], adcValues[2], adcValues[3], adcValues[4], adcValues[5], adcTime[0], adcTime[1], adcTime[2], adcTime[3], adcTime[4], adcTime[5], adcLoops[0], adcLoops[1], adcLoops[2], adcLoops[3], adcLoops[4], adcLoops[5]);
					
					double fl = ((double)adcValues[adc_config.ADC_LoadCellFrontLeft]);
					double fr = ((double)adcValues[adc_config.ADC_LoadCellFrontRight]);
					double bl = ((double)adcValues[adc_config.ADC_LoadCellBackLeft]);
					double br = ((double)adcValues[adc_config.ADC_LoadCellBackRight]);
					double fullWeight = (fl + fr + bl + br) / 4.0;
					fl = fl * forceCalibration.scaleFactor + forceCalibration.offset;
					fr = fr * forceCalibration.scaleFactor + forceCalibration.offset;
					bl = bl * forceCalibration.scaleFactor + forceCalibration.offset;
					br = br * forceCalibration.scaleFactor + forceCalibration.offset;
					fullWeight = fullWeight * forceCalibration.scaleFactor + forceCalibration.offset;
					double ps = ((double)adcValues[adc_config.ADC_Press]) * pressCalibration.scaleFactor + pressCalibration.offset;
					double tp = ((double)adcValues[adc_config.ADC_Temp]) * tempCalibration.scaleFactor + tempCalibration.offset;
					
					
					printf("%d\tweight:\t%.1f\tdig:\tfl:%.1f\tfr:%.1f\tbl:%.1f\tbr:%.1f\tps:%.1f\ttp:%.1f\tval:\t%d\t%d\t%d\t%d\t%d\t%d\tTime:\t%d | %d | %d | %d | %d | %d\tloops:\t%d | %d | %d | %d | %d | %d\t\tInternal Temp: %d / %d(time:%d, loops:%d)\tAVdd: %d / %d(time:%d, loops:%d)\n", totalTime, fullWeight,fl,fr,bl,br,ps,tp,
					/*adcValues[0], adcValues[1], adcValues[2], adcValues[3], adcValues[4], adcValues[5],
					adcTime[0], adcTime[1], adcTime[2], adcTime[3], adcTime[4], adcTime[5], 
					adcLoops[0], adcLoops[1], adcLoops[2], adcLoops[3], adcLoops[4], adcLoops[5]);*/
					adcValues[adc_config.ADC_LoadCellFrontLeft], adcValues[adc_config.ADC_LoadCellFrontRight], adcValues[adc_config.ADC_LoadCellBackLeft], adcValues[adc_config.ADC_LoadCellBackRight], adcValues[adc_config.ADC_Press], adcValues[adc_config.ADC_Temp],
					adcTime[adc_config.ADC_LoadCellFrontLeft], adcTime[adc_config.ADC_LoadCellFrontRight], adcTime[adc_config.ADC_LoadCellBackLeft], adcTime[adc_config.ADC_LoadCellBackRight], adcTime[adc_config.ADC_Press], adcTime[adc_config.ADC_Temp], 
					adcLoops[adc_config.ADC_LoadCellFrontLeft], adcLoops[adc_config.ADC_LoadCellFrontRight], adcLoops[adc_config.ADC_LoadCellBackLeft], adcLoops[adc_config.ADC_LoadCellBackRight], adcLoops[adc_config.ADC_Press], adcLoops[adc_config.ADC_Temp],
					adcValues[6], adcValues[6] - 0x800001, adcTime[6], adcLoops[6],  adcValues[7], adcValues[7] - 0x800001, adcTime[7], adcLoops[7]);
				}
				lastTime = millis();
				amount = 0;
				//Set ADC Channel & Gain to use
				if (settings.use_spi_dev){
					ad7794_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
				} else {
					SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
				}
			} else {
				++amount;
			}
			delay(state.Delay);
		}
	} else if (runningMode.test_heating_power){
		pthread_create(&threadHeaterLedReader, NULL, heaterLedEvaluation, NULL); //(void*) message1
		pthread_create(&threadReadADCValues, NULL, readADCValues, NULL);
		uint8_t currentTestPart = 0;
		uint32_t partReachedTime = 0;
		uint32_t partErrorTime = 0;
		
		uint32_t weight = 0;
		//uint32_t startTime;
		uint32_t time40 = 0;
		uint32_t time90 = 0;
		double lastTemp = 0;
		uint32_t lastOutputTime = 0;
		
		Beep();
		//time for initialize weight
		currentCommandValues.mode = MODE_SCALE;
		delay(2000);
		currentCommandValues.mode = MODE_STANDBY;
		
		printf("Open lid (and remove stirrer if inserted)\n");
		while (state.running){
			timeValues.runTime = time(NULL);
			timeValues.runTimeMillis = millis();
			if (settings.debug_enabled || settings.debug3_enabled){printf("time: %9d\tWeight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", timeValues.runTimeMillis,  adc_values.Weight.adc_value, adc_values.Weight.value, adc_values.Weight.valueByOffset, adc_values.LoadCellFrontLeft.adc_value, adc_values.LoadCellFrontRight.adc_value, adc_values.LoadCellBackLeft.adc_value, adc_values.LoadCellBackRight.adc_value);}
			if (currentTestPart == 0){
				if (state.lidOpen){
					if (partReachedTime == 0){
						partReachedTime = timeValues.runTimeMillis;
					} else if (timeValues.runTimeMillis - partReachedTime > 3000){
						currentCommandValues.mode = MODE_SCALE;
						delay(1000); //to be sure current walue is good referenceForce
						state.referenceForce = -adc_values.Weight.value;
						state.scaleReady = true;
						
						currentTestPart++;
						partReachedTime = 0;
						partErrorTime = 0;
						printf("insert 2 Liter of water\n");
					}
				} else {
					partReachedTime = 0;
				}
			} else if (currentTestPart == 1){
				printf("time: %5.3f\tWeight: %.0fg\n", (double)timeValues.runTimeMillis / 1000, adc_values.Weight.valueByOffset);
				if (adc_values.Weight.valueByOffset >= 2000){
					if (partReachedTime == 0){
						timeValues.beepEndTime = timeValues.runTime+1;
						Beep();
						partReachedTime = timeValues.runTimeMillis;
					} else if (timeValues.runTimeMillis - partReachedTime > 3000){
						weight = adc_values.Weight.valueByOffset;
						currentCommandValues.mode = MODE_STANDBY;
						
						currentTestPart++;
						partReachedTime = 0;
						partErrorTime = 0;
						printf("insert stirrer and close lid\n");
					}
				} else {
					partReachedTime = 0;
				}
			} else if (currentTestPart == 2){
				if (!state.lidOpen){
					if (adc_values.Weight.valueByOffset - weight < 400){
						if (partErrorTime == 0){
							partErrorTime = timeValues.runTimeMillis;
							printf("you need to add stirrer before close lid!\n");
						} else if (timeValues.runTimeMillis - partErrorTime > 4000){
							partErrorTime = 0;
						}
					} else {
						if (partReachedTime == 0){
							partReachedTime = timeValues.runTimeMillis;
						} else if (timeValues.runTimeMillis - partReachedTime > 5000){
							//startTime = timeValues.runTimeMillis;
							
							currentTestPart++;
							partReachedTime = 0;
							partErrorTime = 0;
							printf("wait, messurement is processing\n");
						}
					}
				} else {
					partReachedTime = 0;
				}
			} else if (currentTestPart == 3){
				setMotorRPM(50, &daemon_values);
				HeatOn(&daemon_values);
				currentCommandValues.press = adc_values.Press.valueByOffset;
				currentCommandValues.temp = adc_values.Temp.valueByOffset;
				
				if (lastTemp != adc_values.Temp.valueByOffset || timeValues.runTimeMillis - lastOutputTime > 5000) {
					printf("time: %5.3f\tcurrent temp: %.0f\n", (double)timeValues.runTimeMillis / 1000, adc_values.Temp.valueByOffset);
					lastOutputTime = timeValues.runTimeMillis;
					lastTemp = adc_values.Temp.valueByOffset;
				}
				if (adc_values.Temp.valueByOffset >= 40){
					if (partReachedTime == 0){
						partReachedTime = timeValues.runTimeMillis;
					} else if (timeValues.runTimeMillis - partReachedTime > 5000){
						time40 = partReachedTime;
						
						currentTestPart++;
						partReachedTime = 0;
						partErrorTime = 0;
						printf("reached 40°C, messure time \n");
					}
				} else {
					partReachedTime = 0;
				}
			} else if (currentTestPart == 4){
				setMotorRPM(50, &daemon_values);
				HeatOn(&daemon_values);
				currentCommandValues.press = adc_values.Press.valueByOffset;
				currentCommandValues.temp = adc_values.Temp.valueByOffset;
				
				if (lastTemp != adc_values.Temp.valueByOffset || timeValues.runTimeMillis - lastOutputTime > 5000) {
					printf("time: %5.3f\tcurrent temp: %.0f\n", (double)timeValues.runTimeMillis / 1000, adc_values.Temp.valueByOffset);
					lastOutputTime = timeValues.runTimeMillis;
					lastTemp = adc_values.Temp.valueByOffset;
				} 
				if (adc_values.Temp.valueByOffset >= 90){
					if (partReachedTime == 0){
						partReachedTime = timeValues.runTimeMillis;
					} else if (timeValues.runTimeMillis - partReachedTime > 5000){
						time90 = partReachedTime;
						
						printf("reached 90°C\n");
						
						//Add standard heatUp time for pan 1700g metal: 0.48 J / g*K
						double cp_pan = 0.48;
						double m_pan = 1700;
						double cp_water = 4.18;
						double m_water = weight;
						double usedTime = (time90-time40)/1000;
						double P = (cp_water*m_water+m_pan*cp_pan) * 50 / usedTime; //P=m*cp*deltaT/deltaZeit
						
						currentTestPart++;
						partReachedTime = 0;
						partErrorTime = 0;
						printf("used %.0f seconds for heatup %d g water from 40°C to 90°C => %.2f\n", usedTime, weight, P);
					}
				} else {
					partReachedTime = 0;
				}
			} else if (currentTestPart == 5){
				HeatOff(&daemon_values);
				setMotorRPM(0, &daemon_values);
				if (partReachedTime == 0){
					partReachedTime = timeValues.runTimeMillis;
				} else if (timeValues.runTimeMillis - partReachedTime > 6000){
					state.running = false;
				}
			}
			
			SegmentDisplay();
			delay(state.Delay);
		}
		pthread_join(threadReadADCValues, NULL);
		HeatOff(&daemon_values);
		pthread_join(threadHeaterLedReader, NULL);
	} else if (runningMode.test_heating_press){
		pthread_create(&threadHeaterLedReader, NULL, heaterLedEvaluation, NULL); //(void*) message1
		pthread_create(&threadReadADCValues, NULL, readADCValues, NULL);
		uint8_t currentTestPart = 0;
		uint32_t partReachedTime = 0;
		uint32_t partErrorTime = 0;
		
		uint32_t weight = 0;
		//uint32_t startTime;
		uint32_t timePress1 = 0;
		uint32_t timePress80 = 0;
		double startTemp = 0;
		double lastTemp = 0;
		double lastPress = 0;
		uint32_t lastOutputTime = 0;
		
		Beep();
		//time for initialize weight
		currentCommandValues.mode = MODE_SCALE;
		delay(2000);
		currentCommandValues.mode = MODE_STANDBY;
		
		printf("Open lid (and remove stirrer if inserted)\n");
		while (state.running){
			timeValues.runTime = time(NULL);
			timeValues.runTimeMillis = millis();
			if (settings.debug_enabled || settings.debug3_enabled){printf("time: %9d\tWeight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", timeValues.runTimeMillis,  adc_values.Weight.adc_value, adc_values.Weight.value, adc_values.Weight.valueByOffset, adc_values.LoadCellFrontLeft.adc_value, adc_values.LoadCellFrontRight.adc_value, adc_values.LoadCellBackLeft.adc_value, adc_values.LoadCellBackRight.adc_value);}
			if (currentTestPart == 0){
				if (state.lidOpen){
					if (partReachedTime == 0){
						partReachedTime = timeValues.runTimeMillis;
					} else if (timeValues.runTimeMillis - partReachedTime > 3000){
						currentCommandValues.mode = MODE_SCALE;
						delay(1000); //to be sure current walue is good referenceForce
						state.referenceForce = -adc_values.Weight.value;
						state.scaleReady = true;
						
						currentTestPart++;
						partReachedTime = 0;
						partErrorTime = 0;
						printf("insert 0.5 Liter of water\n");
					}
				} else {
					partReachedTime = 0;
				}
			} else if (currentTestPart == 1){
				printf("time: %5.3f\tWeight: %.0fg\n", (double)timeValues.runTimeMillis / 1000, adc_values.Weight.valueByOffset);
				if (adc_values.Weight.valueByOffset >= 500){
					if (partReachedTime == 0){
						timeValues.beepEndTime = timeValues.runTime+1;
						Beep();
						partReachedTime = timeValues.runTimeMillis;
					} else if (timeValues.runTimeMillis - partReachedTime > 3000){
						weight = adc_values.Weight.valueByOffset;
						currentCommandValues.mode = MODE_STANDBY;
						
						currentTestPart++;
						partReachedTime = 0;
						partErrorTime = 0;
						printf("insert stirrer, close lid, add and lock pusher\n");
					}
				} else {
					partReachedTime = 0;
				}
			} else if (currentTestPart == 2){
				if (!state.lidOpen){
					if (adc_values.Weight.valueByOffset - weight < 400){
						if (partErrorTime == 0){
							partErrorTime = timeValues.runTimeMillis;
							printf("you need to add stirrer before close lid!\n");
						} else if (timeValues.runTimeMillis - partErrorTime > 4000){
							partErrorTime = 0;
						}
					} else {
						if (partReachedTime == 0){
							partReachedTime = timeValues.runTimeMillis;
						} else if (timeValues.runTimeMillis - partReachedTime > 5000){
							//startTime = timeValues.runTimeMillis;
							
							currentTestPart++;
							partReachedTime = 0;
							partErrorTime = 0;
							printf("wait, messurement is processing\n");
						}
					}
				} else {
					partReachedTime = 0;
				}
			} else if (currentTestPart == 3){
				setMotorRPM(50, &daemon_values);
				HeatOn(&daemon_values);
				currentCommandValues.press = adc_values.Press.valueByOffset;
				currentCommandValues.temp = adc_values.Temp.valueByOffset;
				
				if (lastTemp != adc_values.Temp.valueByOffset || lastPress != adc_values.Press.valueByOffset || timeValues.runTimeMillis - lastOutputTime > 5000) {
					printf("time: %5.3f\tcurrent temp: %.0f, current Press: %.0f\n", (double)timeValues.runTimeMillis / 1000, adc_values.Temp.valueByOffset, adc_values.Press.valueByOffset);
					lastOutputTime = timeValues.runTimeMillis;
					lastTemp = adc_values.Temp.valueByOffset;
					lastPress = adc_values.Press.valueByOffset;
				}
				if (adc_values.Press.valueByOffset >= 1){
					if (partReachedTime == 0){
						partReachedTime = timeValues.runTimeMillis;
						startTemp = adc_values.Temp.valueByOffset;
					} else if (timeValues.runTimeMillis - partReachedTime > 5000){
						timePress1 = partReachedTime;
						
						currentTestPart++;
						partReachedTime = 0;
						partErrorTime = 0;
						printf("reached 1kPa, messure time \n");
					}
				} else {
					partReachedTime = 0;
				}
			} else if (currentTestPart == 4){
				setMotorRPM(50, &daemon_values);
				HeatOn(&daemon_values);
				currentCommandValues.press = adc_values.Press.valueByOffset;
				currentCommandValues.temp = adc_values.Temp.valueByOffset;
				
				if (lastTemp != adc_values.Temp.valueByOffset || lastPress != adc_values.Press.valueByOffset || timeValues.runTimeMillis - lastOutputTime > 5000) {
					printf("time: %5.3f\tcurrent temp: %.0f, current Press: %.0f\n", (double)timeValues.runTimeMillis / 1000, adc_values.Temp.valueByOffset, adc_values.Press.valueByOffset);
					lastOutputTime = timeValues.runTimeMillis;
					lastTemp = adc_values.Temp.valueByOffset;
					lastPress = adc_values.Press.valueByOffset;
				}
				if (adc_values.Press.valueByOffset >= 80){
					if (partReachedTime == 0){
						partReachedTime = timeValues.runTimeMillis;
					} else if (timeValues.runTimeMillis - partReachedTime > 5000){
						timePress80 = partReachedTime;
						
						printf("reached 80kPa\n");
						double usedTime = (timePress80-timePress1)/1000;
						double P = 79 / usedTime;
						
						currentTestPart++;
						partReachedTime = 0;
						partErrorTime = 0;
						printf("used %.0f seconds for pressup from 1kPa(with %.0f°C) to 80kPa(with %.0f°C) => %.2fkPa/sec\n", usedTime, startTemp, adc_values.Temp.valueByOffset, P);
						
						currentCommandValues.mode=MODE_PRESSVENT;
						printf("pressvent!\n");
					}
				} else {
					partReachedTime = 0;
				}
			} else if (currentTestPart == 5){
				HeatOff(&daemon_values);
				setMotorRPM(0, &daemon_values);
				if (settings.shieldVersion < 3){
					setServoOpen(100, 10, 1000, &daemon_values);
				} else {
					setSolenoidOpen(false, &daemon_values);
				}
				currentCommandValues.press = adc_values.Press.valueByOffset;
				currentCommandValues.temp = adc_values.Temp.valueByOffset;
				if (currentCommandValues.press < settings.LowPress) {
					if (timeValues.servoStayEndTime == 0){
						timeValues.servoStayEndTime = timeValues.runTime + i2c_servo_values.i2c_servo_stay_open;
					}
					if (timeValues.servoStayEndTime < timeValues.runTime){
						currentCommandValues.mode=MODE_PRESSURELESS;
						timeValues.servoStayEndTime = 0;
						
						if (settings.shieldVersion < 3){
							setServoOpen(0, 1, 0, &daemon_values);
						} else {
							setSolenoidOpen(false, &daemon_values);
						}
						delay(500);
						state.running = false;
					}
				} else {
					timeValues.servoStayEndTime = 0;
				}
				/*
				if (partReachedTime == 0){
					partReachedTime = timeValues.runTimeMillis;
				} else if (timeValues.runTimeMillis - partReachedTime > 15000){
					if (settings.shieldVersion < 3){
						setServoOpen(0, 1, 0, &daemon_values);
					} else {
						setSolenoidOpen(false, &daemon_values);
					}
					delay(500);
					state.running = false;
				}
				*/
			}
			
			SegmentDisplay();
			delay(state.Delay);
		}
		pthread_join(threadReadADCValues, NULL);
		HeatOff(&daemon_values);
		pthread_join(threadHeaterLedReader, NULL);
	}
	
	SegmentDisplaySimple('S', &state, &i2c_config);
	if (settings.logLines == 0 || state.logLineNr<settings.logLines){
		fclose(state.logFilePointer);
	}
	
	//free allocated memmory
	if (settings.logLines != 0){
		uint32_t i=0;
		for (; i<=state.logLineNr; ++i){
			if (state.logLines[i] != NULL){ free(state.logLines[i]); }
		}
		free(state.logLines);
	}
	printf("program ended normally\n");
	
	exit(0); //To make sure all Threads are closed
	return 0;
}

time_t get_mtime(const char *path){
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        perror(path);
    }
    return statbuf.st_mtime;
}


void printUsage(){
	printf("Usage: ecdaemon [OPTIONS]\r\n");
	printf("programm options:\r\n");
	printf("  -s,  --sim                    start in simulation mode, and will increase weight/temp/press automaticaly\r\n");
	printf("  -s7, --sim7seg                show 7Segment display in simulation Mode\r\n");
	printf("  -d,  --debug                  activate Debug output\r\n");
	printf("  -d2, --debug2                 activate Debug output for basic functions\r\n");
	printf("  -d3, --debug3                 activate Debug output for main logic functions\r\n");
	printf("  -c,  --calibrate              calibration mode, will output raw adc values\r\n");
	printf("  -nm, --no-middleware          don't use middleware, use file\r\n");
	printf("  -mf, --middleware-and-file    use middleware and file(on read as backup)\r\n");
	printf("  -cg, --config <path>          set configfile path to <file>\r\n");
	printf("  -ms, --middleware-server <ip>	set middleware Server to <ip>\r\n");
	printf("  -tn, --test-noise             measure and output random noise\r\n");
	printf("  -t7, --test-7seg              blink 7segments to evaluate correct config\r\n");
	printf("  -ts, --test-servo [from [to]] test servo open value, if from/to is omitted, values are from: %d, to: %d\r\n", settings.test_servo_min, settings.test_servo_max);
	printf("  -th, --test-heat-led          read the heater LED informations\r\n");
	printf("  -tm, --test-motor [from [to]] test motor speed value, if from/to is omitted, values are from: %d, to: %d\r\n", 200, 2000);
	printf("  -tb, --test-buttons           test button press\r\n");
	printf("  -ta, --test-adc [updateRate [offset [fullScale]]\r\n");
	printf("                                test adc, set update rate setting to submitted value, use %d if ommited.\r\n", settings.test_ADC_update_rate);
	printf("                                offset/fullScale calibration values i or s, if omittet no calibration.\r\n");
	printf("  -tp, --test-heating-power     test the power/efficiency of heating\r\n");
	printf("  -tr, --test-heating-press     test the power/efficiency of heating on pressup\r\n");
	printf("  -?,  --help                   show this help\r\n");
}

int parseParams(int argc, const char* argv[]){
	int i;
	for (i=1; i<argc; ++i){ //param 0 is programmname
		if(strcmp(argv[i], "--sim") == 0 || strcmp(argv[i], "-s") == 0){
			runningMode.simulationMode = true;
		} else if(strcmp(argv[i], "--sim7seg") == 0 || strcmp(argv[i], "-s7") == 0){
			runningMode.simulationModeShow7Segment = true;
		} else if(strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0){
			settings.debug_enabled = true;
		} else if(strcmp(argv[i], "--debug2") == 0 || strcmp(argv[i], "-d2") == 0){
			setDebugEnabled(true);
		} else if(strcmp(argv[i], "--debug3") == 0 || strcmp(argv[i], "-d3") == 0){
			settings.debug3_enabled = true;
		} else if(strcmp(argv[i], "--debug4") == 0 || strcmp(argv[i], "-d4") == 0){
			settings.debug4_enabled = true;
		} else if(strcmp(argv[i], "--calibrate") == 0 || strcmp(argv[i], "-c") == 0){
			runningMode.calibration = true;
			runningMode.normalMode = false;
			printf("we are in calibration mode, be careful!\n Heat will turn on automatically if switch is on!\n Use switch to turn heat off.\n");
		} else if(strcmp(argv[i], "--test-noise") == 0 || strcmp(argv[i], "-tn") == 0){
			runningMode.measure_noise = true;
			runningMode.normalMode = false;
			printf("we measure random noise\n");
		} else if(strcmp(argv[i], "--test-7seg") == 0 || strcmp(argv[i], "-t7") == 0){
			runningMode.test_7seg = true;
			runningMode.normalMode = false;
			printf("test 7seg config\n");
		} else if(strcmp(argv[i], "--test-servo") == 0 || strcmp(argv[i], "-ts") == 0){
			runningMode.test_servo = true;
			runningMode.normalMode = false;
			if (argc>i+1 && argv[i+1][0] != '-'){
				++i;
				settings.test_servo_min = StringConvertToNumber(argv[i]);
				if (argc>i+1 && argv[i+1][0] != '-'){
					++i;
					settings.test_servo_max = StringConvertToNumber(argv[i]);
				}
			}
			printf("test servo config from %d to %d\n", settings.test_servo_min, settings.test_servo_max);
			
		} else if(strcmp(argv[i], "--test-heat-led") == 0 || strcmp(argv[i], "-th") == 0){
			runningMode.test_heat_led = true;
			runningMode.normalMode = false;
			printf("test heat leds\n");
		} else if(strcmp(argv[i], "--test-motor") == 0 || strcmp(argv[i], "-tm") == 0){
			runningMode.test_motor = true;
			runningMode.normalMode = false;
			settings.test_servo_min = 200;
			settings.test_servo_max = 2000;
			if (argc>i+1 && argv[i+1][0] != '-'){
				++i;
				settings.test_servo_min = StringConvertToNumber(argv[i]);
				if (argc>i+1 && argv[i+1][0] != '-'){
					++i;
					settings.test_servo_max = StringConvertToNumber(argv[i]);
				}
			}
			printf("test motor config from %d to %d\n", settings.test_servo_min, settings.test_servo_max);
			
		} else if(strcmp(argv[i], "--test-buttons") == 0 || strcmp(argv[i], "-tb") == 0){
			runningMode.test_buttons = true;
			runningMode.normalMode = false;
			printf("test buttons\n");
		} else if(strcmp(argv[i], "--test-adc") == 0 || strcmp(argv[i], "-ta") == 0){
			runningMode.test_adc = true;
			runningMode.normalMode = false;
			if (argc>i+1 && argv[i+1][0] != '-'){
				++i;
				settings.test_ADC_update_rate = StringConvertToNumber(argv[i]);
				if (argc>i+1 && argv[i+1][0] != '-'){
					++i;
					if (argv[i][0] == 'i'){
						settings.test_ADC_offsetCalibration = internalZeroScaleCalibration;
					} else if (argv[i][0] == 's'){
						settings.test_ADC_offsetCalibration = systemZeroScaleCalibration;
					}
					if (argc>i+1 && argv[i+1][0] != '-'){
						++i;
						if (argv[i][0] == 'i'){
							settings.test_ADC_fullScaleCalibration = internalFullScaleCalibration;
						} else if (argv[i][0] == 's'){
							settings.test_ADC_fullScaleCalibration = systemFullScaleCalibration;
						}
					}
				}
			}
			printf("test adc\n");
			
		} else if(strcmp(argv[i], "--test-heating-power") == 0 || strcmp(argv[i], "-tp") == 0){
			runningMode.test_heating_power = true;
			runningMode.normalMode = false;
		} else if(strcmp(argv[i], "--test-heating-press") == 0 || strcmp(argv[i], "-tr") == 0){
			runningMode.test_heating_press = true;
			runningMode.normalMode = false;
		} else if(strcmp(argv[i], "--no-middleware") == 0 || strcmp(argv[i], "-nm") == 0){
			settings.useMiddleware = false;
			settings.useFile = true;
		} else if(strcmp(argv[i], "--middleware-and-file") == 0 || strcmp(argv[i], "-mf") == 0){
			settings.useMiddleware = true;
			settings.useFile = true;
		} else if(strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-cg") == 0){
			if (argv[i+1][0] == '-'){
				printf("Error: after --config/-cg the next param must be a pathname, and must not start with -\n");
				printUsage();
				return 1;
			} else {
				++i;
				settings.configFile = (char *) malloc(strlen(argv[i]) * sizeof(char) + 1);
				strcpy(&settings.configFile[0], argv[i]);
			}
		} else if(strcmp(argv[i], "--middleware-server") == 0 || strcmp(argv[i], "-ms") == 0){
			if (argv[i+1][0] == '-'){
				printf("Error: after --middleware-server/-ms the next param must be a ip, and must not start with -\n");
				printUsage();
				return 1;
			} else {
				++i;
				settings.middlewareHostname = (char *) malloc(strlen(argv[i]) * sizeof(char) + 1);
				strcpy(&settings.middlewareHostname[0], argv[i]);
			}
		} else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-?") == 0){
			printUsage();
			return 0;
		} else {
			printUsage();
			return 1;
		}
	}
	return -1;
}

void defineSignalHandler(){
	signal (SIGTERM, handleSignal); //kill -term <pid>
	signal (SIGINT, handleSignal); //Ctr+c
	signal (SIGQUIT, handleSignal); //Ctrl+???
	signal (SIGKILL, handleSignal); //kill -kill <pid> //handling not possible...
	signal (SIGHUP, handleSignal); //hang-up signal is used to report that the user's terminal is disconnected
	signal (SIGTSTP, handleSignal); //is interactive Job Control stop signal
}

void handleSignal(int signum) {
	/* signum is one of:
	SIGTERM = 15
	SIGINT = 2
	SIGQUIT = 3
	SIGKILL = 9
	SIGHUP = 1
	??? = 20 => ctr+z
	*/
	if (signum == SIGTERM || signum == SIGINT || signum == SIGQUIT || signum == SIGKILL || signum == SIGHUP){
		printf("signal %d recived\n", signum);
		state.running = false;
	} else {
		printf("not handled signal %d recived\n", signum);
	}
}

void clearHourCounter(){
	hourCounter.identifier[0] = 'H';
	hourCounter.identifier[1] = 'C';
	hourCounter.identifier[2] = 'V';
	hourCounter.identifier[3] = HOUR_COUNTER_VERSION;
	hourCounter.daemon = 0;
	hourCounter.heater = 0;
	hourCounter.motor = 0;
}

void initOutputFile(void){
	if (settings.debug_enabled){printf("iniOutputFile\n");}
	FILE *fp;
	fp = fopen(settings.statusFile, "w");
	fputs("{\"T0\":0,\"P0\":0,\"M0RPM\":0,\"M0ON\":0,\"M0OFF\":0,\"W0\":0,\"STIME\":0,\"SMODE\":0,\"SID\":-2}", fp);
	fclose(fp);
	
	char* headerLine = "Time, Temp, Press, MotorRpm, Weight, setTemp, setPress, setMotorRpm, setWeight, setMode, Mode, heaterHasPower, isOn, noPan, lidOpen\n";
	if (settings.logLines != 0){
		state.logLines = (char **)malloc(sizeof(char *) * (settings.logLines+1)); //+1 for headerline/line 0
		state.logLines[0] = (char *) malloc(strlen(headerLine) * sizeof(char) + 1);
		strcpy(state.logLines[0], headerLine);
	}
	if (settings.DeleteLogOnStart){
		state.logFilePointer = fopen(settings.logFile, "w");
		fprintf(state.logFilePointer, headerLine);
	} else {
		if (settings.logLines != 0){
			//TODO read current log content and set state.logLineNr
		}
		state.logFilePointer = fopen(settings.logFile, "a");
	}
	
	FILE *hourCounterFilePointer = fopen(settings.hourCounterFile, "r");
	if (hourCounterFilePointer == NULL){
		printf("error reading hourCounter values, file does not exist %s\n", settings.hourCounterFile);
		clearHourCounter();
	} else {
		if (!feof(hourCounterFilePointer)){
			size_t readAmount = fread(&hourCounter, sizeof(hourCounter), 1, hourCounterFilePointer);
			if (readAmount != 1) {//struct amount not bytes
				printf("error reading hourCounter values, read %d instat of %d structs from file %s\n", readAmount, 1, settings.hourCounterFile);
				clearHourCounter();
			} else if (hourCounter.identifier[0] != 'H' ||hourCounter.identifier[1] != 'C' ||hourCounter.identifier[2] != 'V'){
				printf("error reading hourCounter values, identifier not found, file invalid %s\n", settings.hourCounterFile);
				clearHourCounter();
			} else if (hourCounter.identifier[3] != HOUR_COUNTER_VERSION){
				printf("error reading hourCounter values, unknown hour counter Version, file invalid %s\n", settings.hourCounterFile);
				clearHourCounter();
			} else {
				char lineBracke = fgetc(hourCounterFilePointer);
				if (lineBracke != '\n'){
					printf("error reading hourCounter values, no linebracke after values, file invalid %s\n", settings.hourCounterFile);
					clearHourCounter();
				}
			}
		} else {
			printf("error reading hourCounter values, file is empty %s\n", settings.hourCounterFile);
			clearHourCounter();
		}
		fclose(hourCounterFilePointer);
	}
}

void resetValues(){
	state.isBuzzing = true; //to be sure first send a off to buzzer
	state.oldSegmentDisplay = ' ';
	newCommandValues.stepId=-2;
	currentCommandValues.stepId=-2;
	state.dataChanged = true;
	state.timeChanged = true;
	timeValues.lastFileChangeTime = 0;
	
	currentCommandValues.temp = 0;
	currentCommandValues.press = 0;
	currentCommandValues.weight = 0;
	currentCommandValues.time = 0;
	currentCommandValues.mode = 0;
	
	newCommandValues.mode = 0;
	newCommandValues.temp = 0;
	newCommandValues.press = 0;
	newCommandValues.weight = 0;
	
	state.Delay = settings.LongDelay;
	
	i2c_servo_values.servoOpen=0;
	writeI2CPin(i2c_config.i2c_servo, i2c_servo_values.i2c_servo_closed);
	
	heaterStatus.ledValues[0] = 0;
	heaterStatus.ledValues[1] = 0;
	heaterStatus.ledValues[2] = 0;
	heaterStatus.ledValues[3] = 0;
	heaterStatus.ledValues[4] = 0;
	heaterStatus.ledValues[5] = 0;
}


bool checkForInput(){
	bool valueChanged = false;
	if (settings.useMiddleware){
		uint8_t recivedDataType = 0;
		if (state.sockfd < 0){
			if (timeValues.runTime - timeValues.middlewareConnectTime > 5){
				//only try to connect to middleware every 5 sec.
				timeValues.middlewareConnectTime = timeValues.runTime;
				state.sockfd = connectToMiddleware(settings.middlewareHostname, settings.middlewarePortno);
			}
		}
		if (state.sockfd>=0 && isSocketReady(state.sockfd)){
			//if (reciveFromSock(state.sockfd, state.middlewareBuffer) ){
			if (recivePackageFromSock(state.sockfd, state.middlewareBuffer, &recivedDataType)){
				valueChanged = true;
				if (settings.debug_enabled || settings.debug3_enabled){printf("recived Value(with type %d): %s\n", recivedDataType, state.middlewareBuffer);}
				if (recivedDataType != dataType){
					fprintf(stderr, "ERROR recived dataType unknown\n");
					close(state.sockfd);
					state.sockfd = -15;
				} else {
					parseSockInput(state.middlewareBuffer);
				}
			} else {
				fprintf(stderr, "ERROR reading from socket\n");
				close(state.sockfd);
				state.sockfd = -14;
			}
		}
		if (state.sockfd<0){
			if (settings.useFile && ReadFile()){
				valueChanged = true;
			} else {
				delay(10000);
			}
		} else if (settings.useFile){
			time_t changeTime = get_mtime(settings.commandFile);
			//TODO: changeTime = localtime(changeTime);
			
			if (changeTime > timeValues.lastFileChangeTime){
				if (ReadFile()){
					valueChanged = true;
				}
			} else {
				if (settings.debug_enabled || settings.debug3_enabled){printf("commandFile to old, is:%ld, last:%ld\n", changeTime, timeValues.lastFileChangeTime);}
			}
		}
	} else {
		if (settings.useFile && ReadFile()){
			valueChanged = true;
		} else {
			delay(10000);
		}
	}
	
	return valueChanged;
}

void doOutput(){
	prepareState(state.TotalUpdate);
	if (state.dataChanged || timeValues.lastStatusTime+MIN_STATUS_INTERVAL<timeValues.runTime){// || state.timeChanged){
		if (settings.debug_enabled){printf("dataChanged=%d\n",state.dataChanged);}
		if (settings.useMiddleware && state.sockfd>=0){
			if (!sendToSock(state.sockfd, state.TotalUpdate, dataType)){
				fprintf(stderr, "ERROR writing to socket\n");
				state.sockfd = -13;
			}
		}
		if (settings.useFile) {
			writeStatus(state.TotalUpdate);
		}
		if (settings.debug_enabled || settings.debug3_enabled){printf("%s\n",state.TotalUpdate);}
		state.dataChanged = false;
		state.timeChanged = false;
		timeValues.lastStatusTime=timeValues.runTime;
	}
	writeLog();
}

void ProcessCommand(void){
	if (newCommandValues.stepId != currentCommandValues.stepId){
		if (newCommandValues.stepId < currentCommandValues.stepId){
			//-1 stop/not aus
			if(newCommandValues.stepId == -1){
				i2c_motor_values.motorRpm = i2c_motor_values.i2c_motor_speed_min; //cheating so it will realy stop motor
				setMotorRPM(0, &daemon_values);
				HeatOff(&daemon_values);
			}
			//TODO is reset/stop? or is someone try to begin new recipe before old is ended?
			if(newCommandValues.stepId==0){
				resetValues();
				newCommandValues.stepId=0;
			}
		}
		oldCommandValues.mode = currentCommandValues.mode;
		currentCommandValues.mode = newCommandValues.mode;
		currentCommandValues.stepId = newCommandValues.stepId;
		timeValues.stepStartTime = timeValues.runTime;
		state.dataChanged = true;
		timeValues.stepEndTime = timeValues.stepStartTime + newCommandValues.time;
		
		state.scaleReady = false;
		if (state.Delay == settings.ShortDelay || (oldCommandValues.mode == MODE_SCALE && currentCommandValues.mode == MODE_SCALE)) {
			if(settings.debug3_enabled){printf("new and old step is scale mode, reset values\n");}
			state.referenceForce=0;
			state.Delay=settings.LongDelay;
			currentCommandValues.weight = 0.0;
		}
		
		if (settings.debug_enabled){printf("stepId changed, new mode is: %d\n", currentCommandValues.mode);}
		if (settings.debug_enabled || runningMode.simulationMode || settings.debug3_enabled){printf("ProcessCommand: T0: %.0f, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %.0f, STIME: %d, SMODE: %d, SID: %d\n", newCommandValues.temp, newCommandValues.press, newCommandValues.motorRpm, newCommandValues.motorOn, newCommandValues.motorOff, newCommandValues.weight, newCommandValues.time, newCommandValues.mode, newCommandValues.stepId);}
		
		OptionControl();
		if (state.alwaysReadMode){
			speak(state.actionText);
		}
	}
	TempControl();
	PressControl();
	if (settings.debug3_enabled){
		printf("\n");
	}
	MotorControl();
	ValveControl();
	ScaleFunction();
	
	if (timeValues.stepEndTime > timeValues.runTime) {
		timeValues.remainTime=timeValues.stepEndTime-timeValues.runTime;
	} else if (currentCommandValues.mode<MIN_STATUS_MODE) {
		timeValues.remainTime=0;
		if (currentCommandValues.mode==MODE_COOK || currentCommandValues.mode == MODE_PRESSHOLD){
			currentCommandValues.mode=MODE_COOK_TIMEEND;
			state.dataChanged=true;
			if (settings.BeepStepEnd>0){
				timeValues.beepEndTime = timeValues.runTime+settings.BeepStepEnd;
			}
		} else if (currentCommandValues.mode != MODE_STANDBY){
			currentCommandValues.mode=MODE_STANDBY;
			state.dataChanged=true;
			if (settings.BeepStepEnd>0){
				timeValues.beepEndTime = timeValues.runTime+settings.BeepStepEnd;
			}
		}
	}
	SegmentDisplay();
	Beep();
}

/******************* processing functions **********************/
void OptionControl(){
    if (currentCommandValues.mode>=MODE_OPTIONS_BEGIN){
		if (currentCommandValues.mode==MODE_OPTION_REMEMBER_BEEP_ON){
			settings.doRememberBeep=true;
		} else if (currentCommandValues.mode==MODE_OPTION_REMEMBER_BEEP_OFF){
			settings.doRememberBeep=false;
		} else if (currentCommandValues.mode==MODE_OPTION_7SEGMENT_BLINK){
			SegmentDisplaySimple(' ', &state, &i2c_config);
			blink7Segment(&i2c_config);
			blink7Segment(&i2c_config);
			blink7Segment(&i2c_config);
			blink7Segment(&i2c_config);
		}
		currentCommandValues.mode=MODE_STANDBY;
		state.dataChanged=true;
    }
}

void TempControl(){
	if (currentCommandValues.mode<MIN_COOK_MODE || currentCommandValues.mode>MAX_COOK_MODE) HeatOff(&daemon_values);
	if (currentCommandValues.mode>=MIN_TEMP_MODE && currentCommandValues.mode<=MAX_TEMP_MODE) {
		oldCommandValues.temp = currentCommandValues.temp;
		currentCommandValues.temp = adc_values.Temp.valueByOffset;
		if (settings.debug_enabled || runningMode.calibration || settings.debug3_enabled){printf("Temp %d dig %.0f °C | ", adc_values.Temp.adc_value, adc_values.Temp.valueByOffset);}
		if (runningMode.measure_noise) {printf("NoiseTemp %d | ", adc_noise.DeltaTemp);}
		
		if (oldCommandValues.temp != currentCommandValues.temp){
			state.dataChanged = true;
		}
		
		int deltaT=newCommandValues.temp-currentCommandValues.temp;
		if (currentCommandValues.mode==MODE_HEATUP || currentCommandValues.mode==MODE_COOK) {
			if (deltaT<=0) {
				HeatOff(&daemon_values);
				if (currentCommandValues.mode==MODE_HEATUP) {//heatup function
					timeValues.stepEndTime=timeValues.runTime-2;
					currentCommandValues.mode=MODE_HOT; //we are hot
					state.dataChanged=true;
				}
			}
			else if (deltaT>=5) {HeatOn(&daemon_values);} 
		} else {
			HeatOff(&daemon_values);
		}
		if (currentCommandValues.mode==MODE_HEATUP && state.heatPowerStatus) { //heatup
			timeValues.stepEndTime=timeValues.runTime+1;
		} else if (currentCommandValues.mode==MODE_COOLDOWN && deltaT>=0) { //cooldown function
			timeValues.stepEndTime=timeValues.runTime-2;
			currentCommandValues.mode=MODE_COLD;
			state.dataChanged=true;
		} else if (currentCommandValues.mode==MODE_COOLDOWN) {
			timeValues.stepEndTime=timeValues.runTime+1;
		}
	} else {
		oldCommandValues.temp = currentCommandValues.temp;
		currentCommandValues.temp = adc_values.Temp.valueByOffset;
		if (settings.debug_enabled || runningMode.calibration || settings.debug3_enabled){printf("Temp %d dig %.0f °C | ", adc_values.Temp.adc_value, adc_values.Temp.valueByOffset);}
		if (runningMode.measure_noise) {printf("NoiseTemp %d | ", adc_noise.DeltaTemp);}
		if (oldCommandValues.temp != currentCommandValues.temp){
			state.dataChanged = true;
		}
	}
}

void PressControl(){
	if (currentCommandValues.mode<MIN_COOK_MODE || currentCommandValues.mode>MAX_COOK_MODE) HeatOff(&daemon_values);
	if (currentCommandValues.mode>=MIN_PRESS_MODE && currentCommandValues.mode<=MAX_PRESS_MODE) {
		oldCommandValues.press = currentCommandValues.press;
		currentCommandValues.press = adc_values.Press.valueByOffset;
		if (settings.debug_enabled || runningMode.calibration || settings.debug3_enabled){printf("Press %d digits %.0f kPa | ", adc_values.Press.adc_value, adc_values.Press.valueByOffset);}
		if (runningMode.measure_noise){printf("NoisePress %d |", adc_noise.DeltaPress);}
		
		if (oldCommandValues.press != currentCommandValues.press){
			state.dataChanged = true;
		}
		
		int deltaP=newCommandValues.press-currentCommandValues.press;
		if (currentCommandValues.mode==MODE_PRESSUP || currentCommandValues.mode==MODE_PRESSHOLD) {
			if (deltaP<=0) {
				HeatOff(&daemon_values);
				if (currentCommandValues.mode==MODE_PRESSUP) {//pressup function
					timeValues.stepEndTime=timeValues.runTime-2;
					currentCommandValues.mode=MODE_PRESSURIZED; //we are pressurized
					state.dataChanged=true;
				}
			}
			else if (deltaP>=5) {HeatOn(&daemon_values);}
		} else {
			HeatOff(&daemon_values);
		}
		if (currentCommandValues.mode==MODE_PRESSUP && state.heatPowerStatus) { //pressure up
			timeValues.stepEndTime=timeValues.runTime+1;;
		} else if (currentCommandValues.mode==MODE_PRESSDOWN && deltaP>=0) { //pressure down function
			timeValues.stepEndTime=timeValues.runTime-2;
			currentCommandValues.mode=MODE_PRESSURELESS;
			state.dataChanged=true;
		} else if (currentCommandValues.mode==MODE_PRESSDOWN) {
			timeValues.stepEndTime=timeValues.runTime+1;
		}
	} else {
		oldCommandValues.press = currentCommandValues.press;
		currentCommandValues.press = adc_values.Press.valueByOffset;
		if (settings.debug_enabled || runningMode.calibration || settings.debug3_enabled){printf("Press %d digits %.0f kPa | ", adc_values.Press.adc_value, adc_values.Press.valueByOffset);}
		if (runningMode.measure_noise){printf("NoisePress %d |", adc_noise.DeltaPress);}
		
		if (oldCommandValues.press != currentCommandValues.press){
			state.dataChanged = true;
		}
	}
}

void MotorControl(){
	if (currentCommandValues.mode==MODE_CUT || currentCommandValues.mode>=MIN_TEMP_MODE){
		if (currentCommandValues.mode==MODE_CUT) timeValues.stepEndTime=timeValues.runTime+1;
		if (newCommandValues.motorRpm > 0 && currentCommandValues.mode<=MAX_STATUS_MODE) {
			if  (newCommandValues.motorOn > 0 && newCommandValues.motorOff > 0) {
				if (timeValues.runTime >= timeValues.motorStartTime+newCommandValues.motorOn && timeValues.runTime < timeValues.motorStartTime+newCommandValues.motorOn+newCommandValues.motorOff) { 
					currentCommandValues.motorRpm = 0; 
				} else if (currentCommandValues.motorRpm==0){
					currentCommandValues.motorRpm = newCommandValues.motorRpm;
					timeValues.motorStartTime=timeValues.runTime;
				} else if (currentCommandValues.motorRpm != newCommandValues.motorRpm){
					currentCommandValues.motorRpm = newCommandValues.motorRpm;
				}
			} else {
				currentCommandValues.motorRpm = newCommandValues.motorRpm;
				timeValues.motorStartTime=timeValues.runTime;
			}
		} else {
			currentCommandValues.motorRpm = 0;
		}
	} else {
		currentCommandValues.motorRpm = 0;
	}
	setMotorRPM(currentCommandValues.motorRpm, &daemon_values);
	
	if (i2c_motor_values.motorRpm == 0){
		if(timeValues.motorStopTime < timeValues.motorStartTime){
			timeValues.motorStopTime = timeValues.runTime;
		}
	}
}

//void ValveControl(struct State state, struct Settings settings, struct Command_Values currentCommandValues, struct Time_Values timeValues){
void ValveControl(){
	if (currentCommandValues.mode==MODE_PRESSVENT) {
		HeatOff(&daemon_values);
		if (settings.shieldVersion < 3){
			setServoOpen(100, 10, 1000, &daemon_values);
		} else {
			setSolenoidOpen(true, &daemon_values);
		}
		timeValues.stepEndTime=timeValues.runTime+1;
		if (currentCommandValues.press < settings.LowPress) {
			if (timeValues.servoStayEndTime == 0){
				timeValues.servoStayEndTime = timeValues.runTime + i2c_servo_values.i2c_servo_stay_open;
			}
			if (timeValues.servoStayEndTime < timeValues.runTime){
				currentCommandValues.mode=MODE_PRESSURELESS;
				state.dataChanged=true;
				timeValues.servoStayEndTime = 0;
			}
		} else {
			timeValues.servoStayEndTime = 0;
		}
	} else {
		if (settings.shieldVersion < 3){
			setServoOpen(0, 1, 0, &daemon_values);
		} else {
			setSolenoidOpen(false, &daemon_values);
		}
	}
}

//void ScaleFunction(struct State state, struct Settings settings, struct Command_Values oldCommandValues, struct Command_Values currentCommandValues, struct Command_Values newCommandValues, struct Time_Values timeValues){
void ScaleFunction(){
	if (currentCommandValues.mode==MODE_SCALE || currentCommandValues.mode==MODE_WEIGHT_REACHED){
		timeValues.stepEndTime=timeValues.runTime+2;
		oldCommandValues.weight = currentCommandValues.weight;
		double sumOfForces = adc_values.Weight.value;
		if (settings.debug_enabled || runningMode.calibration){printf("Weight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", adc_values.Weight.adc_value, adc_values.Weight.value, adc_values.Weight.valueByOffset, adc_values.LoadCellFrontLeft.adc_value, adc_values.LoadCellFrontRight.adc_value, adc_values.LoadCellBackLeft.adc_value, adc_values.LoadCellBackRight.adc_value);}			
		if (runningMode.measure_noise){
			printf("NoiseWeightFL %d | ", adc_noise.DeltaWeight1);
			printf("NoiseWeightFR %d | ", adc_noise.DeltaWeight2);
			printf("NoiseWeightBL %d | ", adc_noise.DeltaWeight3);
			printf("NoiseWeightBR %d\n", adc_noise.DeltaWeight4);
		}
		if (settings.debug_enabled){printf("ScaleFunction\n");}
		
		if (!state.scaleReady) { //we are not ready for weighting
			state.referenceForce=sumOfForces;
			oldCommandValues.weight=sumOfForces;
			state.scaleReady=true;
			if (state.Delay == settings.ShortDelay){
				state.Delay=settings.LongDelay;
			}
			if (settings.debug_enabled){printf("\tscaleReady\n");}
		} else { //we have a reference and are ready
			state.Delay=settings.ShortDelay;
			currentCommandValues.weight=(sumOfForces-state.referenceForce);
			
			if (oldCommandValues.weight != currentCommandValues.weight){
				state.dataChanged = true;
			}
			if (currentCommandValues.weight>=(newCommandValues.weight*settings.weightReachedMultiplier)) {//If we have reached the required mass
				if (currentCommandValues.mode != MODE_WEIGHT_REACHED){
					if(settings.BeepWeightReached > 0){
						timeValues.beepEndTime=timeValues.runTime+settings.BeepWeightReached;
					}
					if (settings.debug_enabled || settings.debug3_enabled){printf("\tweight reached!\n");}
				} else {
					if (settings.debug_enabled){printf("\tweight reached...\n");}
				}
				if (currentCommandValues.mode != MODE_WEIGHT_REACHED){
					currentCommandValues.mode=MODE_WEIGHT_REACHED;
					state.dataChanged=true;
				}
				timeValues.remainTime=0;
			}
			if (settings.debug_enabled || settings.debug3_enabled){printf("\t\tweight: %f / old: %f\n", currentCommandValues.weight, oldCommandValues.weight);}
		}
	} else if (state.Delay == settings.ShortDelay || state.scaleReady){
		state.scaleReady=false;
		state.referenceForce=0;
		state.Delay=settings.LongDelay;
		currentCommandValues.weight = 0.0;
	}
}

//void SegmentDisplay(struct State state, struct Settings settings, struct Running_Mode runningMode, struct I2C_Config i2c_config){
void SegmentDisplay(){
	char curSegmentDisplay = ' ';
	if (currentCommandValues.press>=settings.LowPress){
		curSegmentDisplay = 'P';
	} else if (currentCommandValues.temp>=settings.LowTemp){
		curSegmentDisplay = 'H';
	} else {
		curSegmentDisplay = '0';
	}
	if (curSegmentDisplay != state.oldSegmentDisplay){
		if (settings.debug_enabled || runningMode.simulationMode){printf("SegmentDisplay: '%c'\n", curSegmentDisplay);}
		if (!runningMode.simulationMode || runningMode.simulationModeShow7Segment){
			SegmentDisplaySimple(curSegmentDisplay, &state, &i2c_config);
			//SegmentDisplayOptimized(curSegmentDisplay, &state, &i2c_config);
		} else {
			state.oldSegmentDisplay = curSegmentDisplay;
		}
	}
	if (!runningMode.simulationMode || runningMode.simulationModeShow7Segment){
		if (timeValues.lastBlinkTime != timeValues.runTime){
			timeValues.lastBlinkTime = timeValues.runTime;
			//togglePin(i2c_7seg_period);
			if (state.blinkState){
				writeI2CPin(i2c_config.i2c_7seg_period,I2C_7SEG_OFF);
				state.blinkState = false;
				if (settings.debug_enabled || (runningMode.simulationMode && !runningMode.simulationModeShow7Segment)){printf("SegmentDisplay: blink off\n");}
			} else {
				state.blinkState = true;
				writeI2CPin(i2c_config.i2c_7seg_period,I2C_7SEG_ON);
				if (settings.debug_enabled || (runningMode.simulationMode && !runningMode.simulationModeShow7Segment)){printf("SegmentDisplay: blink on\n");}
			}
		}
	}
}


void Beep(){
	if(settings.doRememberBeep==1){
		if (timeValues.runTime>timeValues.stepEndTime){
			if (currentCommandValues.mode>=MIN_STATUS_MODE && currentCommandValues.mode<=MAX_STATUS_MODE){
				int toLateTime = timeValues.runTime-timeValues.stepEndTime;
				if (toLateTime==1){
					timeValues.beepEndTime=timeValues.runTime+1;
				} else if (toLateTime==5){
					timeValues.beepEndTime=timeValues.runTime+1;
				} else if (toLateTime==30){
					timeValues.beepEndTime=timeValues.runTime+1;
				} else if (toLateTime==60){
					timeValues.beepEndTime=timeValues.runTime+1;
				} else if (toLateTime>0 && toLateTime%300==0){ //each 5 min
					timeValues.beepEndTime=timeValues.runTime+5;
				}
			}
		}
	}
	if (timeValues.runTime<timeValues.beepEndTime){
		if (!state.isBuzzing){
			state.isBuzzing = true;
			uint32_t duration = timeValues.beepEndTime-timeValues.runTime;
			char command[100];
			sprintf(command, "speaker-test -f 500 -t sine -p 1000 -l %d 1>/dev/null 2>&1 &", duration);
			//char command[200];
			//sprintf(command, "speaker-test -f 500 -t sine -p 1000 -l %d --wavdir /opt/EveryCook/daemon/sounds --wavfile ding.wav &", duration);
			//sprintf(command, "speaker-test -f 500 -t sine -p 1000 -l %d --wavdir %s --wavfile %s &", duration, settings.wavdir, settings.wavfile);
			printf("%s\n",command);
			system(command);
		}
	} else {
		if (state.isBuzzing || ALLWAYS_STOP_BUZZING){
			state.isBuzzing = false;
		}
	}
}


/*******************PI File read/write Code**********************/
//format: {"T0":000,"P0":000,"M0RPM":0000,"M0ON":000,"M0OFF":000,"W0":0000,"STIME":000000,"SMODE":00,"SID":000}


void prepareState(char* TotalUpdate){
	StringClean(TotalUpdate, 512);
	uint32_t press; //remove negative values for output;
	if (currentCommandValues.press>0){
		press = currentCommandValues.press;
	} else {
		press = 0;
	}
	sprintf(TotalUpdate, "{\"T0\":%.2f,\"P0\":%d,\"M0RPM\":%d,\"M0ON\":%d,\"M0OFF\":%d,\"W0\":%.0f,\"STIME\":%d,\"SMODE\":%d,\"SID\":%d,\"heaterHasPower\":%d,\"isOn\":%d,\"noPan\":%d,\"lidOpen\":%d}",	currentCommandValues.temp, 						press, currentCommandValues.motorRpm, currentCommandValues.motorOn, currentCommandValues.motorOff, currentCommandValues.weight, currentCommandValues.time, currentCommandValues.mode, currentCommandValues.stepId, heaterStatus.hasPower, heaterStatus.isOn, heaterStatus.noPanError, state.lidOpen);
	if (settings.debug_enabled){printf("prepareState: T0: %f, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %f, STIME: %d, SMODE: %d, SID: %d, heaterHasPower: %d, isOn: %d, noPan: %d, lidOpen:%d\n", 		currentCommandValues.temp, currentCommandValues.press, currentCommandValues.motorRpm, currentCommandValues.motorOn, currentCommandValues.motorOff, currentCommandValues.weight, currentCommandValues.time, currentCommandValues.mode, currentCommandValues.stepId, heaterStatus.hasPower, heaterStatus.isOn, heaterStatus.noPanError, state.lidOpen);}
}

void writeStatus(char* data){
	if (settings.debug_enabled){printf("WriteStatus\n");}
	
	FILE *fp;
	fp = fopen(settings.statusFile, "w");
	fputs(data, fp);
	fclose(fp);
	if (settings.debug3_enabled){printf("%s\n", data);}
	
	if (settings.debug_enabled){printf("WriteStatus: after write\n");}
}

void updateMotorTime(){
	//if (currentCommandValues.motorRpm>0){
	if (i2c_motor_values.motorRpm>0){
		if (timeValues.motorStartTime < timeValues.lastLogSaveTime){
			hourCounter.motor = hourCounter.motor + (timeValues.runTime - timeValues.lastLogSaveTime);
		} else {
			hourCounter.motor = hourCounter.motor + (timeValues.runTime - timeValues.motorStartTime);
		}
	} else if (i2c_motor_values.motorRpm == 0){
		if (timeValues.motorStopTime > timeValues.lastLogSaveTime){
			hourCounter.motor = hourCounter.motor + (timeValues.motorStopTime - timeValues.lastLogSaveTime);
		}
	}
}


void updateHeaterTime(){
	//if (currentCommandValues.motorRpm>0){
	if (state.heatPowerStatus){
		if (timeValues.heaterStartTime < timeValues.lastLogSaveTime){
			hourCounter.heater = hourCounter.heater + (timeValues.runTime - timeValues.lastLogSaveTime);
		} else {
			hourCounter.heater = hourCounter.heater + (timeValues.runTime - timeValues.heaterStartTime);
		}
	} else {
		if (timeValues.heaterStopTime > timeValues.lastLogSaveTime){
			hourCounter.heater = hourCounter.heater + (timeValues.heaterStopTime - timeValues.lastLogSaveTime);
		}
	}
}

void writeLog(){
	if (settings.debug_enabled){printf("writeLog\n");}
	timeValues.nowTime = time(NULL);
	timeValues.localTime=localtime(&timeValues.nowTime);
	
	if (timeValues.runTime>=timeValues.lastLogSaveTime+settings.logSaveInterval){
		char tempString[20];
		StringClean(tempString, 20);
		strftime(tempString, 20,"%F %T",timeValues.localTime);
		char logline[200];
		sprintf(logline, "%s, %.1f, %i, %i, %.1f, %.1f, %i, %i, %.1f, %i, %i, %i, %i, %i, %i\n",tempString, currentCommandValues.temp, currentCommandValues.press, i2c_motor_values.motorRpm, currentCommandValues.weight, newCommandValues.temp, newCommandValues.press, newCommandValues.motorRpm, newCommandValues.weight, newCommandValues.mode, currentCommandValues.mode, heaterStatus.hasPower, heaterStatus.isOn, heaterStatus.noPanError, state.lidOpen);
		
		if (settings.logLines == 0){
			fputs(logline, state.logFilePointer);
			//open once and flush, instat open and close it always
			fflush(state.logFilePointer);
		} else {
			if (state.logLineNr<settings.logLines){
				++state.logLineNr;
				state.logLines[state.logLineNr] = (char *) malloc(strlen(logline) * sizeof(char) + 1);
				strcpy(state.logLines[state.logLineNr], logline);
				
				//if not yet reached max line amount only add last line to file.
				fputs(state.logLines[state.logLineNr], state.logFilePointer);
				fflush(state.logFilePointer);
				if (state.logLineNr==settings.logLines){
					//if max amount of lines reached, close file, because it will be open as overwrite in "ringbuffer" logic.
					fclose(state.logFilePointer);
				}
			} else {
				state.logFilePointer = fopen(settings.logFile, "w");
				fputs(state.logLines[0], state.logFilePointer);
				uint32_t i=1;
				if (state.logLines[i] != NULL){ free(state.logLines[i]); }
				for (; i<settings.logLines; ++i){
					state.logLines[i] = state.logLines[i+1];
					fputs(state.logLines[i], state.logFilePointer);
				}
				state.logLines[state.logLineNr] = (char *) malloc(strlen(logline) * sizeof(char) + 1);
				strcpy(state.logLines[state.logLineNr], logline);
				fputs(state.logLines[state.logLineNr], state.logFilePointer);
				fclose(state.logFilePointer);
			}
		}
		
		if (timeValues.lastLogSaveTime>0){
			hourCounter.daemon = hourCounter.daemon + (timeValues.runTime - timeValues.lastLogSaveTime);
			updateHeaterTime();
			updateMotorTime();
			
			FILE *hourCounterFilePointer = fopen(settings.hourCounterFile, "w");
			fwrite(&hourCounter, sizeof(hourCounter), 1, hourCounterFilePointer);
			fprintf(hourCounterFilePointer, "\nhours on: %.2f\nmotor hours: %.2f\nheater hours: %.2f\n", hourCounter.daemon/SECONDS_PER_HOUR, hourCounter.motor/SECONDS_PER_HOUR, hourCounter.heater/SECONDS_PER_HOUR);
			//fprintf(hourCounterFilePointer, "\nhours on: %d\nmotor hours: %d\nheater hours: %d\n", hourCounter.daemon, hourCounter.motor, hourCounter.heater);
			fclose(hourCounterFilePointer);
		}
		timeValues.lastLogSaveTime=timeValues.runTime;
	}
	if (settings.debug_enabled){printf("writeLog: after write\n");}
}

//format: {"T0":000,"P0":000,"M0RPM":0000,"M0ON":000,"M0OFF":000,"W0":0000,"STIME":000000,"SMODE":00,"SID":000}

void parseSockInput(char* input){
	char tempName[10];
	char tempValue[10];
	uint8_t i = 0;
	uint8_t ptr = 0;
	uint8_t inputPos = 0;
	char c;
	
	StringClean(tempName, 10);
	StringClean(tempValue, 10);
	c = input[inputPos];
	++inputPos;
	while (c != 0){
		if (c == '"'){
			c = input[inputPos];
			++inputPos;
			while (c != '"'){
				tempName[i] = c;
				c = input[inputPos];
				++inputPos;
				++i;
			}
			i = 0;
		}
		if (c == ':') {
			c = input[inputPos];
			++inputPos;
			while (c == ' ' || c == 9){
				c = input[inputPos];
				++inputPos;
			}
			if(strcmp(tempName, "TEXT") == 0){
				if (c == '"'){
					c = input[inputPos];
					++inputPos;
				}
				while (c != '"' || (c == '"' && input[inputPos-2] == '\\')){
					state.actionText[i] = c;
					c = input[inputPos];
					++inputPos;
					i++;
				}
				state.actionText[i] = 0;
				if (c == '"'){
					c = input[inputPos];
					++inputPos;
				}
				i = 0;
			} else {
				while (c >= 48 && c <= 58){
					tempValue[i] = c;
					c = input[inputPos];
					++inputPos;
					i++;
				}
				i = 0;
				state.value[ptr] = StringConvertToNumber(tempValue);
			}
			
			//state.names[ptr] = tempName;
			int32_t j = 0;
			for (; j < 10; ++j){
				state.names[ptr][j] = tempName[j];
			}
			
			if (settings.debug_enabled){printf("found %s with value %d\r\n", state.names[ptr], state.value[ptr]);}
			StringClean(tempName, 10);
			StringClean(tempValue, 10);
			++ptr;
		}
		c = input[inputPos];
		++inputPos;
	}
}

bool ReadFile(){
	if (settings.debug_enabled){printf("ReadFile\n");}
	FILE *fp;
	char tempName[10];
	char tempValue[10];
	uint8_t i = 0;
	uint8_t ptr = 0;
	char c;
	
	StringClean(tempName, 10);
	StringClean(tempValue, 10);
	fp = fopen(settings.commandFile, "r");
	if (fp == NULL){
		printf("could not open file %s\n", settings.commandFile);
		return false;
	}
	
	while ((c = fgetc(fp)) != 255){
		if (c == '"'){
			c = fgetc(fp);
			while (c != '"'){
				tempName[i] = c;
				c = fgetc(fp);
				i++;
			}
			i = 0;
		}
		if (c == ':') {
			c = fgetc(fp);
			while (c == ' ' || c == 9){
				c = fgetc(fp);
			}
			
			if(strcmp(tempName, "TEXT") == 0){
				if (c == '"'){
					c = fgetc(fp);
				}
				char lastc = ' ';
				while (c != '"' || (c == '"' && lastc == '\\')){
					state.actionText[i] = c;
					lastc = c;
					c = fgetc(fp);
					i++;
				}
				state.actionText[i] = 0;
				if (c == '"'){
					c = fgetc(fp);
				}
				i = 0;
			} else {
				while (c >= 48 && c <= 58){
					tempValue[i] = c;
					c = fgetc(fp);
					i++;
				}
				i = 0;
				state.value[ptr] = StringConvertToNumber(tempValue);
			}
			
			//state.names[ptr] = tempName;
			int32_t j = 0;
			for (; j < 10; ++j){
				state.names[ptr][j] = tempName[j];
			}
			
			if (settings.debug_enabled){printf("found %s with value %d\r\n", state.names[ptr], state.value[ptr]);}
			
			StringClean(tempName, 10);
			StringClean(tempValue, 10);
			ptr++;
		}
	}
	fclose(fp);
	
	return true;
}


void evaluateInput(){
	//TODO only set "setXXX" values if stepId changed, other wise ignore "new/changed values".
	int i;
	for(i = 0; i < 15; ++i){
		//if (settings.debug_enabled){printf("check %s with value %d\r\n", state.names[i], state.value[i]);}
		if(strcmp(state.names[i], "T0") == 0){
			newCommandValues.temp = state.value[i];
		} else if(strcmp(state.names[i], "P0") == 0){
			newCommandValues.press = state.value[i];
		} else if(strcmp(state.names[i], "M0RPM") == 0){
			newCommandValues.motorRpm = state.value[i];
		} else if(strcmp(state.names[i], "M0ON") == 0){
			newCommandValues.motorOn = state.value[i];
		} else if(strcmp(state.names[i], "M0OFF") == 0){
			newCommandValues.motorOff = state.value[i];
		} else if(strcmp(state.names[i], "W0") == 0){
			newCommandValues.weight = state.value[i];
		} else if(strcmp(state.names[i], "STIME") == 0){
			newCommandValues.time = state.value[i];
		} else if(strcmp(state.names[i], "SMODE") == 0){
			newCommandValues.mode = state.value[i];
		} else if(strcmp(state.names[i], "SID") == 0){
			newCommandValues.stepId = state.value[i];
		}
	}
	if (settings.debug_enabled){printf("evaluateInput: T0: %.0f, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %.0f, STIME: %d, SMODE: %d, SID: %d\n", newCommandValues.temp, newCommandValues.press, newCommandValues.motorRpm, newCommandValues.motorOn, newCommandValues.motorOff, newCommandValues.weight, newCommandValues.time, newCommandValues.mode, newCommandValues.stepId);}
	
	if(newCommandValues.temp>200) {newCommandValues.temp=200;}
	if(newCommandValues.press>200) {newCommandValues.press=200;}
	if (newCommandValues.motorOn>0 && newCommandValues.motorOn<2) newCommandValues.motorOn=2;
	if (newCommandValues.motorOff>0 && newCommandValues.motorOff<2) newCommandValues.motorOff=2;
}




bool readConfigLine(char* keyString, char* valueString, FILE *fp){
	char c;
	char c2;
	
	uint8_t i = 0;
	c = fgetc(fp);
	while (c != 255){
		if (c == '#'){
			if (settings.debug_enabled){printf("\tline with # found\n");}
			while (c != '\r' && c != '\n' && c != 255){
				c = fgetc(fp);
			}
		} else if (c == '\n'){
			//Empty line or end of last line
		} else if (c == '\r'){
			//Empty line
		} else {
			i = 0;
			while (c != '=' && c != '\r' && c != '\n'){
				keyString[i] = c;
				c = fgetc(fp);
				++i;
			}
			i = 0;
			c = fgetc(fp); //currently its '=' so read next
			while (c != '\r' && c != '\n' && c != 255){
				if (c == '#'){
					//allow coment behind value
					break;
				}
				valueString[i] = c;
				c = fgetc(fp);
				++i;
			}
			if (c == '#'){
				//read until line end so comment is readed
				if (settings.debug_enabled){printf("\tline with # at end of value found\n");}
				c = fgetc(fp);
				while (c != '\r' && c != '\n' && c != 255){
					c = fgetc(fp);
				}
			}
			//remove spaces at end of value
			--i;
			if (i>0){
				c2 = valueString[i];
				while (c2 == ' ' || c2 == '\t'){
					valueString[i] = 0x00;
					--i;
					if (i < 0){
						break;
					}
					c2 = valueString[i];
				}
			}
			return true;
		}
		c = fgetc(fp);
	}
	return false;
}

/* Read the configuration
 *
 */
void ReadConfigurationFile(void){
	if (settings.debug_enabled){printf("ReadConfigurationFile...\n");}
	
	bool showReadedConfigs = settings.debug_enabled;
	
	FILE *fp;
	char keyString[30];
	char valueString[100];
	StringClean(keyString, 30);
	StringClean(valueString, 100);
	fp = fopen(settings.configFile, "r");
	if (fp != NULL){
		while (readConfigLine(&keyString[0], &valueString[0], fp)){
			if (settings.debug_enabled){printf("\tkey: '%s', value: '%s'\n", keyString, valueString);}
			
			//ParseConfigValue
			if(strcmp(keyString, "BeepWeightReached") == 0){
				settings.BeepWeightReached = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tBeepWeightReached: %d\n", settings.BeepWeightReached);} // (old: %d)
			} else if(strcmp(keyString, "BeepStepEnd") == 0){
				settings.BeepStepEnd = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tBeepStepEnd: %d\n", settings.BeepStepEnd);} // (old: %d)
			} else if(strcmp(keyString, "doRememberBeep") == 0){
				settings.doRememberBeep = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tdoRememberBeep: %d\n", settings.doRememberBeep);} // (old: %d)
			} else if(strcmp(keyString, "DeleteLogOnStart") == 0){
				settings.DeleteLogOnStart = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tDeleteLogOnStart: %d\n", settings.DeleteLogOnStart);} // (old: %d)
			} else if(strcmp(keyString, "calibrationFile") == 0){
				settings.calibrationFile = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.calibrationFile[0], valueString);
				if (showReadedConfigs){printf("\tcalibrationFile: %s\n", settings.calibrationFile);} // (old: %s)
			} else if(strcmp(keyString, "LogFile") == 0){
				settings.logFile = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.logFile[0], valueString);
				if (showReadedConfigs){printf("\tLogFile: %s\n", settings.logFile);} // (old: %s)
			} else if(strcmp(keyString, "CommandFile") == 0){
				settings.commandFile = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.commandFile[0], valueString);
				if (showReadedConfigs){printf("\tCommandFile: %s\n", settings.commandFile);} // (old: %s)
			} else if(strcmp(keyString, "StatusFile") == 0){
				settings.statusFile = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.statusFile[0], valueString);
				if (showReadedConfigs){printf("\tStatusFile: %s\n", settings.statusFile);} // (old: %s)
			} else if(strcmp(keyString, "hourCounterFile") == 0){
				settings.hourCounterFile = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.hourCounterFile[0], valueString);
				if (showReadedConfigs){printf("\thourCounterFile: %s\n", settings.hourCounterFile);}
			} else if(strcmp(keyString, "installPath") == 0){
				settings.installPath = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.installPath[0], valueString);
				if (showReadedConfigs){printf("\tinstallPath %s\n", settings.installPath);} // (old: %s)
			} else if(strcmp(keyString, "speakLanguage") == 0){
				state.language = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&state.language[0], valueString);
				if (showReadedConfigs){printf("\tspeakLanguage %s\n", state.language);} // (old: %s)
				
				
			} else if(strcmp(keyString, "LowTemp") == 0){
				settings.LowTemp = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tLowTemp: %d\n", settings.LowTemp);} // (old: %d)
			} else if(strcmp(keyString, "LowPress") == 0){
				settings.LowPress = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tLowPress: %d\n", settings.LowPress);} // (old: %d)
			} else if(strcmp(keyString, "LongDelay") == 0){
				settings.LongDelay = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tLongDelay: %d\n", settings.LongDelay);} // (old: %d)
			} else if(strcmp(keyString, "ShortDelay") == 0){
				settings.ShortDelay = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tShortDelay: %d\n", settings.ShortDelay);} // (old: %d)
				
			} else if(strcmp(keyString, "logSaveInterval") == 0){
				settings.logSaveInterval = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tlogSaveInterval: %d\n", settings.logSaveInterval);} // (old: %d)
			} else if(strcmp(keyString, "logLines") == 0){
				settings.logLines = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tlogLines %d\n", settings.logLines);} // (old: %d)
				
			} else if(strcmp(keyString, "weightReachedMultiplier") == 0){
				settings.weightReachedMultiplier = StringConvertToDouble(valueString);
				if (showReadedConfigs){printf("\tweightReachedMultiplier %.2f\n", settings.weightReachedMultiplier);} // (old: %d)	
				
			} else if(strcmp(keyString, "shieldVersion") == 0){
				settings.shieldVersion = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tshieldVersion: %d\n", settings.shieldVersion);}
			
			
			} else if(strcmp(keyString, "middlewareHostname") == 0){
				settings.middlewareHostname = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.middlewareHostname[0], valueString);
				if (showReadedConfigs){printf("\tmiddlewareHostname: %s\n", settings.middlewareHostname);}
			} else if(strcmp(keyString, "middlewarePortno") == 0){
				settings.middlewarePortno = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tmiddlewarePortno: %d\n", settings.middlewarePortno);}
			
			//motor values
			} else if(strcmp(keyString, "i2c_motor_speed_min") == 0){
				i2c_motor_values.i2c_motor_speed_min = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_motor_speed_min: %d\n", i2c_motor_values.i2c_motor_speed_min);}
			} else if(strcmp(keyString, "i2c_motor_speed_ramp") == 0){
				i2c_motor_values.i2c_motor_speed_ramp = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_motor_speed_ramp %d\n", i2c_motor_values.i2c_motor_speed_ramp);}
			
			//servo values
			} else if(strcmp(keyString, "i2c_servo_stay_open") == 0){
				i2c_servo_values.i2c_servo_stay_open = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_servo_stay_open: %d\n", i2c_servo_values.i2c_servo_stay_open);}
			
			//ADC values
			} else if(strcmp(keyString, "ADC_ref") == 0){
				adc_config.ADC_ref = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tADC_ref: %d\n", adc_config.ADC_ref);}
			} else if(strcmp(keyString, "ADC_update_rate") == 0){
				adc_config.ADC_update_rate = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tADC_update_rate %d\n", adc_config.ADC_update_rate);}
			
			} else {
				if (settings.debug_enabled){printf("\tkey not Found\n");}
			}
			StringClean(keyString, 30);
			StringClean(valueString, 100);
		}
	} else {
		printf("config file '%s' not found!", settings.configFile);
	}
	
	fclose(fp);
	
	if (settings.shieldVersion < 1 && settings.shieldVersion > 3){
		printf("Shield version %d unknown stop daemon.\n", settings.shieldVersion);
		exit(1);
	}
	
	setADCModeReg(adc_config.ADC_update_rate & 0x000F);
	
	if (settings.debug_enabled){printf("done.\n");}
}

void ReadCalibrationFile(void){
	if (settings.debug_enabled){printf("ReadCalibrationFile...\n");}
	
	bool showReadedConfigs = settings.debug_enabled || runningMode.calibration;
	
	FILE *fp;
	char keyString[30];
	char valueString[100];
	uint8_t ptr = 0;
	StringClean(keyString, 30);
	StringClean(valueString, 100);
	fp = fopen(settings.calibrationFile, "r");
	if (fp != NULL){
		while (readConfigLine(&keyString[0], &valueString[0], fp)){
			if (settings.debug_enabled){printf("\tkey: '%s', value: '%s'\n", keyString, valueString);}
			
			//ParseConfigValue
			//scale
			if(strcmp(keyString, "ForceADC1") == 0){
				forceCalibration.ADC1 = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tForceADC1: %d\n", forceCalibration.ADC1);} // (old: %d)
			} else if(strcmp(keyString, "ForceValue1") == 0){
				forceCalibration.Value1 = StringConvertToDouble(valueString);
				if (showReadedConfigs){printf("\tForceValue1: %f\n", forceCalibration.Value1);} // (old: %d)
			} else if(strcmp(keyString, "ForceADC2") == 0){
				forceCalibration.ADC2 = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tForceADC2: %d\n", forceCalibration.ADC2);} // (old: %d)
			} else if(strcmp(keyString, "ForceValue2") == 0){
				forceCalibration.Value2 = StringConvertToDouble(valueString);
				if (showReadedConfigs){printf("\tForceValue2: %f\n", forceCalibration.Value2);} // (old: %d)
				
			//pressure
			} else if(strcmp(keyString, "PressADC1") == 0){
				pressCalibration.ADC1 = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tPressADC1: %d\n", pressCalibration.ADC1);} // (old: %d)
			} else if(strcmp(keyString, "PressValue1") == 0){
				pressCalibration.Value1 = StringConvertToDouble(valueString);
				if (showReadedConfigs){printf("\tPressValue1: %f\n", pressCalibration.Value1);} // (old: %d)
			} else if(strcmp(keyString, "PressADC2") == 0){
				pressCalibration.ADC2 = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tPressADC2: %d\n", pressCalibration.ADC2);} // (old: %d)
			} else if(strcmp(keyString, "PressValue2") == 0){
				pressCalibration.Value2 = StringConvertToDouble(valueString);
				if (showReadedConfigs){printf("\tPressValue2: %f\n", pressCalibration.Value2);} // (old: %d)
				
			//temperature
			} else if(strcmp(keyString, "TempADC1") == 0){
				tempCalibration.ADC1 = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tTempADC1: %d\n", tempCalibration.ADC1);} // (old: %d)
			} else if(strcmp(keyString, "TempValue1") == 0){
				tempCalibration.Value1 = StringConvertToDouble(valueString);
				if (showReadedConfigs){printf("\tTempValue1: %f\n", tempCalibration.Value1);} // (old: %d)
			} else if(strcmp(keyString, "TempADC2") == 0){
				tempCalibration.ADC2 = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tTempADC2: %d\n", tempCalibration.ADC2);} // (old: %d)
			} else if(strcmp(keyString, "TempValue2") == 0){
				tempCalibration.Value2 = StringConvertToDouble(valueString);
				if (showReadedConfigs){printf("\tTempValue2: %f\n", tempCalibration.Value2);} // (old: %d)
			
			//adc channels
			} else if(strcmp(keyString, "ADC_LoadCellFrontLeft") == 0){
				adc_config.ADC_LoadCellFrontLeft = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tADC_LoadCellFrontLeft: %d\n", adc_config.ADC_LoadCellFrontLeft);}
			} else if(strcmp(keyString, "ADC_LoadCellFrontRight") == 0){
				adc_config.ADC_LoadCellFrontRight = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tLADC_oadCellFrontRight: %d\n", adc_config.ADC_LoadCellFrontRight);}
			} else if(strcmp(keyString, "ADC_LoadCellBackLeft") == 0){
				adc_config.ADC_LoadCellBackLeft = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tADC_LoadCellBackLeft: %d\n", adc_config.ADC_LoadCellBackLeft);}
			} else if(strcmp(keyString, "ADC_LoadCellBackRight") == 0){
				adc_config.ADC_LoadCellBackRight = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tADC_LoadCellBackRight: %d\n", adc_config.ADC_LoadCellBackRight);}
			} else if(strcmp(keyString, "ADC_Press") == 0){
				adc_config.ADC_Press = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tADC_Press: %d\n", adc_config.ADC_Press);}
			} else if(strcmp(keyString, "ADC_Temp") == 0){
				adc_config.ADC_Temp = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tADC_Temp: %d\n", adc_config.ADC_Temp);}
			
			//adc gain
			} else if(strcmp(keyString, "Gain_LoadCellFrontLeft") == 0){
				ptr = adc_config.ADC_LoadCellFrontLeft;
				adc_config.ADC_ConfigReg[ptr] = StringConvertToNumber(valueString);
				adc_config.ADC_ConfigReg[ptr] = POWNTimes(adc_config.ADC_ConfigReg[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
				if (showReadedConfigs){printf("\tGain_LoadCellFrontLeft: %04X\n", adc_config.ADC_ConfigReg[ptr]);} // (old: %d)
			} else if(strcmp(keyString, "Gain_LoadCellFrontRight") == 0){
				ptr = adc_config.ADC_LoadCellFrontRight;
				adc_config.ADC_ConfigReg[ptr] = StringConvertToNumber(valueString);
				adc_config.ADC_ConfigReg[ptr] = POWNTimes(adc_config.ADC_ConfigReg[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
				if (showReadedConfigs){printf("\tGain_LoadCellFrontRight: %04X\n", adc_config.ADC_ConfigReg[ptr]);} // (old: %d)
			} else if(strcmp(keyString, "Gain_LoadCellBackLeft") == 0){
				ptr = adc_config.ADC_LoadCellBackLeft;
				adc_config.ADC_ConfigReg[ptr] = StringConvertToNumber(valueString);
				adc_config.ADC_ConfigReg[ptr] = POWNTimes(adc_config.ADC_ConfigReg[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
				if (showReadedConfigs){printf("\tGain_LoadCellBackLeft: %04X\n", adc_config.ADC_ConfigReg[ptr]);} // (old: %d)
			} else if(strcmp(keyString, "Gain_LoadCellBackRight") == 0){
				ptr = adc_config.ADC_LoadCellBackRight;
				adc_config.ADC_ConfigReg[ptr] = StringConvertToNumber(valueString);
				adc_config.ADC_ConfigReg[ptr] = POWNTimes(adc_config.ADC_ConfigReg[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
				if (showReadedConfigs){printf("\tGain_LoadCellBackRight: %04X\n", adc_config.ADC_ConfigReg[ptr]);} // (old: %d)
			} else if(strcmp(keyString, "Gain_Press") == 0){
				ptr = adc_config.ADC_Press;
				adc_config.ADC_ConfigReg[ptr] = StringConvertToNumber(valueString);
				adc_config.ADC_ConfigReg[ptr] = POWNTimes(adc_config.ADC_ConfigReg[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
				if (showReadedConfigs){printf("\tGain_Press: %04X\n", adc_config.ADC_ConfigReg[ptr]);} // (old: %d)
			} else if(strcmp(keyString, "Gain_Temp") == 0){
				ptr = adc_config.ADC_Temp;
				adc_config.ADC_ConfigReg[ptr] = StringConvertToNumber(valueString);
				adc_config.ADC_ConfigReg[ptr] = POWNTimes(adc_config.ADC_ConfigReg[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
				if (showReadedConfigs){printf("\tGain_Temp: %04X\n", adc_config.ADC_ConfigReg[ptr]);} // (old: %d)
			
			//adc inversee
			} else if(strcmp(keyString, "inverse_LoadCellFrontLeft") == 0){
				adc_config.inverse[adc_config.ADC_LoadCellFrontLeft] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tinverse_LoadCellFrontLeft: %d\n", adc_config.inverse[adc_config.ADC_LoadCellFrontLeft]);}
			} else if(strcmp(keyString, "inverse_LoadCellFrontRight") == 0){
				adc_config.inverse[adc_config.ADC_LoadCellFrontRight] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tinverse_LoadCellFrontRight: %d\n", adc_config.inverse[adc_config.ADC_LoadCellFrontRight]);}
			} else if(strcmp(keyString, "inverse_LoadCellBackLeft") == 0){
				adc_config.inverse[adc_config.ADC_LoadCellBackLeft] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tinverse_LoadCellBackLeft: %d\n", adc_config.inverse[adc_config.ADC_LoadCellBackLeft]);}
			} else if(strcmp(keyString, "inverse_LoadCellBackRight") == 0){
				adc_config.inverse[adc_config.ADC_LoadCellBackRight] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tinverse_LoadCellBackRight: %d\n", adc_config.inverse[adc_config.ADC_LoadCellBackRight]);}
			} else if(strcmp(keyString, "inverse_Press") == 0){
				adc_config.inverse[adc_config.ADC_Press] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tinverse_Press: %d\n", adc_config.inverse[adc_config.ADC_Press]);}
			} else if(strcmp(keyString, "inverse_Temp") == 0){
				adc_config.inverse[adc_config.ADC_Temp] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tinverse_Temp: %d\n", adc_config.inverse[adc_config.ADC_Temp]);}
			
			//7seg pins
			} else if(strcmp(keyString, "i2c_7seg_top") == 0){
				i2c_config.i2c_7seg_top = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_7seg_top: %d\n", i2c_config.i2c_7seg_top);}
			} else if(strcmp(keyString, "i2c_7seg_top_left") == 0){
				i2c_config.i2c_7seg_top_left = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_7seg_top_left: %d\n", i2c_config.i2c_7seg_top_left);}
			} else if(strcmp(keyString, "i2c_7seg_top_right") == 0){
				i2c_config.i2c_7seg_top_right = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_7seg_top_right: %d\n", i2c_config.i2c_7seg_top_right);}
			} else if(strcmp(keyString, "i2c_7seg_center") == 0){
				i2c_config.i2c_7seg_center = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_7seg_center: %d\n", i2c_config.i2c_7seg_center);}
			} else if(strcmp(keyString, "i2c_7seg_bottom_left") == 0){
				i2c_config.i2c_7seg_bottom_left = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_7seg_bottom_left: %d\n", i2c_config.i2c_7seg_bottom_left);}
			} else if(strcmp(keyString, "i2c_7seg_bottom_right") == 0){
				i2c_config.i2c_7seg_bottom_right = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_7seg_bottom_right: %d\n", i2c_config.i2c_7seg_bottom_right);}
			} else if(strcmp(keyString, "i2c_7seg_bottom") == 0){
				i2c_config.i2c_7seg_bottom = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_7seg_bottom: %d\n", i2c_config.i2c_7seg_bottom);}
			} else if(strcmp(keyString, "i2c_7seg_period") == 0){
				i2c_config.i2c_7seg_period = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_7seg_period: %d\n", i2c_config.i2c_7seg_period);}
				
			} else if(strcmp(keyString, "i2c_motor") == 0){
				i2c_config.i2c_motor = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_motor: %d\n", i2c_config.i2c_motor);}
			} else if(strcmp(keyString, "i2c_servo") == 0){
				i2c_config.i2c_servo = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_servo %d\n", i2c_config.i2c_servo);}
			
			//Servo values
			} else if(strcmp(keyString, "i2c_servo_open") == 0){
				i2c_servo_values.i2c_servo_open = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_servo_open: %d\n", i2c_servo_values.i2c_servo_open);}
			} else if(strcmp(keyString, "i2c_servo_closed") == 0){
				i2c_servo_values.i2c_servo_closed = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_servo_closed %d\n", i2c_servo_values.i2c_servo_closed);}
			
			//Solonoid values
			} else if(strcmp(keyString, "i2c_solenoid_open") == 0){
				i2c_solenoid_values.i2c_solenoid_open = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_solenoid_open: %d\n", i2c_solenoid_values.i2c_solenoid_open);}
			} else if(strcmp(keyString, "i2c_solenoid_closed") == 0){
				i2c_solenoid_values.i2c_solenoid_closed = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\ti2c_solenoid_closed %d\n", i2c_solenoid_values.i2c_solenoid_closed);}
				
			//Button pins
			} else if(strcmp(keyString, "pin_button_0") == 0){
				buttonConfig.button_pin[0] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tbutton_pin_0 %d\n", buttonConfig.button_pin[0]);}
			} else if(strcmp(keyString, "pin_button_1") == 0){
				buttonConfig.button_pin[1] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tbutton_pin_1 %d\n", buttonConfig.button_pin[1]);}
			} else if(strcmp(keyString, "pin_button_2") == 0){
				buttonConfig.button_pin[2] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tbutton_pin_2 %d\n", buttonConfig.button_pin[2]);}
			
			//Button inverse
			} else if(strcmp(keyString, "inverse_button_0") == 0){
				buttonConfig.button_inverse[0] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tbutton_inverse_0 %d\n", buttonConfig.button_inverse[0]);}
			} else if(strcmp(keyString, "inverse_button_1") == 0){
				buttonConfig.button_inverse[1] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tbutton_inverse_1 %d\n", buttonConfig.button_inverse[1]);}
			} else if(strcmp(keyString, "inverse_button_2") == 0){
				buttonConfig.button_inverse[2] = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tbutton_inverse_2 %d\n", buttonConfig.button_inverse[2]);}
				
			} else {
				if (settings.debug_enabled){printf("\tkey not Found\n");}
			}
			StringClean(keyString, 30);
			StringClean(valueString, 100);
		}
	} else {
		printf("calibration file '%s' not found!", settings.calibrationFile);
	}
	
	fclose(fp);
	
	forceCalibration.scaleFactor=(forceCalibration.Value2-forceCalibration.Value1)/((double)forceCalibration.ADC2-(double)forceCalibration.ADC1);
	forceCalibration.offset=forceCalibration.Value1 - forceCalibration.ADC1*forceCalibration.scaleFactor ;  //offset in g //for default reference force in calibration mode only
	
	pressCalibration.scaleFactor=(pressCalibration.Value2-pressCalibration.Value1)/((double)pressCalibration.ADC2-(double)pressCalibration.ADC1);
	pressCalibration.offset=pressCalibration.Value1 - pressCalibration.ADC1*pressCalibration.scaleFactor ;  //offset in pa
	
	tempCalibration.scaleFactor=(tempCalibration.Value2-tempCalibration.Value1)/((double)tempCalibration.ADC2-(double)tempCalibration.ADC1);
	tempCalibration.offset=tempCalibration.Value1 - tempCalibration.ADC1*tempCalibration.scaleFactor ;  //offset in °C
	
	setADCConfigReg(adc_config.ADC_ConfigReg);
	
	if (showReadedConfigs){printf("ForceScaleFactor: %.4f (, ForceOffset: %f), PressScaleFactor: %.4f, PressOffset: %f, TempScaleFactor: %.4f, TempOffset: %f\n", forceCalibration.scaleFactor, forceCalibration.offset, pressCalibration.scaleFactor, pressCalibration.offset, tempCalibration.scaleFactor, tempCalibration.offset);}
	
	if (settings.debug_enabled){printf("done.\n");}
}

bool checkIsHeaterState(uint32_t* leds, uint8_t errNo, bool* state, uint32_t* lastTime, uint32_t runTime){
	bool isSame = true;
	uint8_t i;
	for (i = 0; i<6; ++i){
		if (HeaterErrorPattern[errNo][i] != leds[i]){
			isSame = false;
			break;
		}
	}
	if (isSame){
		*lastTime = runTime;
		*state = true;
	} else {
		if (*state && *lastTime + HeaterErrorTimeout[errNo] < runTime){
			*state = false;
		}
	}
	return *state;
}

void *heaterLedEvaluation(void *ptr){
	if (runningMode.simulationMode){
		heaterStatus.hasPower = true;
		heaterStatus.noPanError = false;
		while (state.running){
			heaterStatus.isOn = state.heatPowerStatus;
			delay(500);
		}
		return 0;
	}
	if (settings.shieldVersion == 1){
		uint32_t lastTime_V1[7];
		while (state.running){
			uint32_t runTime = time(NULL);
			
			heaterStatus.ledValues[0] = readSignPin(0);
			heaterStatus.ledValues[1] = readSignPin(1);
			heaterStatus.ledValues[2] = readSignPin(2);
			heaterStatus.ledValues[3] = readSignPin(3);
			heaterStatus.ledValues[4] = readSignPin(4);
			heaterStatus.ledValues[5] = readSignPin(5);
			
			if (heaterStatus.ledValues[0] && heaterStatus.ledValues[1] && heaterStatus.ledValues[2]  && heaterStatus.ledValues[3] && heaterStatus.ledValues[4]  && heaterStatus.ledValues[5]){
				if (heaterStatus.hasPower && heaterStatus.hasPowerLedOnLastTime + 4 < timeValues.runTime){
					heaterStatus.hasPower = false;
				}
			} else {
				heaterStatus.hasPowerLedOnLastTime = timeValues.runTime;
				if (!heaterStatus.hasPower){
					heaterStatus.hasPower = true;
				}
			}
			
			if (heaterStatus.ledValues[3] == 0){
				heaterStatus.isOnLastTime = timeValues.runTime;
				if (!heaterStatus.isOn){
					heaterStatus.isOn = true;
				}
			} else if (heaterStatus.isOn && heaterStatus.isOnLastTime + 4 < timeValues.runTime){
				heaterStatus.isOn = false;
			}
			
			
			bool errorFound = false;
			if(checkIsHeaterState(&heaterStatus.ledValues[0], 0, &heaterStatus.noPanError, &lastTime_V1[0], runTime)){
				heaterStatus.errorMsg = HeaterErrorCodes[0];
				errorFound = true;
			}
			
			if(checkIsHeaterState(&heaterStatus.ledValues[0], 1, &heaterStatus.tempSensorError, &lastTime_V1[1], runTime)){
				heaterStatus.errorMsg = HeaterErrorCodes[1];
				errorFound = true;
			}
			if(checkIsHeaterState(&heaterStatus.ledValues[0], 2, &heaterStatus.IGBTSensorError, &lastTime_V1[2], runTime)){
				heaterStatus.errorMsg = HeaterErrorCodes[2];
				errorFound = true;
			}
			if(checkIsHeaterState(&heaterStatus.ledValues[0], 3, &heaterStatus.voltageToHeightError, &lastTime_V1[3], runTime)){
				heaterStatus.errorMsg = HeaterErrorCodes[3];
				errorFound = true;
			}
			if(checkIsHeaterState(&heaterStatus.ledValues[0], 4, &heaterStatus.voltageToLowError, &lastTime_V1[4], runTime)){
				heaterStatus.errorMsg = HeaterErrorCodes[4];
				errorFound = true;
			}
			if(checkIsHeaterState(&heaterStatus.ledValues[0], 5, &heaterStatus.bowOutOfWaterError, &lastTime_V1[5], runTime)){
				heaterStatus.errorMsg = HeaterErrorCodes[5];
				errorFound = true;
			}
			if(checkIsHeaterState(&heaterStatus.ledValues[0], 6, &heaterStatus.IGBTTempToHeightError, &lastTime_V1[6], runTime)){
				heaterStatus.errorMsg = HeaterErrorCodes[6];
				errorFound = true;
			}
			
			if (!errorFound){
				heaterStatus.errorMsg = NULL;
			}
			//if (settings.debug_enabled){
			//	printf("leds:\t%d\t%d\t%d\t%d\t%d\t%d\t|\thas power:%d\tis heating:%d\tNoPan:%d\t\tError:%s\n",heaterStatus.ledValues[0],heaterStatus.ledValues[1],heaterStatus.ledValues[2],heaterStatus.ledValues[3],heaterStatus.ledValues[4],heaterStatus.ledValues[5], heaterStatus.hasPower,heaterStatus.isHeating,heaterStatus.noPan, heaterStatus.errorMsg);
			//}
			delay(10);
		}
	} else if (settings.shieldVersion == 2){
		uint32_t led[6];
		led[0] = 0;
		led[1] = 0;
		led[2] = 0;
		led[3] = 0;
		led[4] = 0;
		led[5] = 0;
		
		uint32_t leds, led1,led2,led3;
		uint32_t updateDelay = 10;
		uint32_t runTimeMillis;
		while (state.running){
			bool different = false;
			uint8_t i = 0;
			bool newHasError = false;
			
			uint32_t multiplexer = readSignPin(0);
			delayMicroseconds(200);
			leds = readSignPin(0);
			led1 = readSignPin(1);
			led2 = readSignPin(2);
			led3 = readSignPin(3);
			delayMicroseconds(200);
			runTimeMillis = millis();
			if (multiplexer == leds && leds == readSignPin(0) && led1 == readSignPin(1) && led2 == readSignPin(2) && led3 == readSignPin(3)){
				if (multiplexer==0){
					led[0] = led1;
					led[1] = led2;
					led[2] = led3;
					if (heaterStatus.hasPower && (runTimeMillis - heaterStatus.multiplexerLastTime1) > 200){
						//multiplexer value not changed for a amount of time -> No Power
						led[3] = 0;
						led[4] = 0;
						led[5] = 0;
					}
				} else {
					led[3] = led1;
					led[4] = led2;
					led[5] = led3;
					heaterStatus.multiplexerLastTime1 = runTimeMillis;
				}
				
				if (settings.debug_enabled){printf("%d\t%d\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",runTimeMillis,multiplexer, led[0],led[1],led[2],led[3],led[4],led[5]);}
			} else {
				delay(updateDelay);
				continue;
			}
			bool multiplexerChanged = false;
			if (multiplexer != heaterStatus.lastMultiplexer){
				heaterStatus.lastMultiplexer = multiplexer;
				multiplexerChanged = true;
			}
			
			for (; i<6; ++i){
				if (heaterStatus.ledValues[i] != led[i]){
					different = true;
					break;
				}
			}
			
			if (!different && heaterStatus.lastDiffMultiplexer != multiplexer){
				//OK
				if (settings.debug_enabled){printf("diff changed complete %d\n", multiplexer);}
				heaterStatus.lastDiffMultiplexer = multiplexer;
			} else {
				if (different){
					heaterStatus.lastDiffMultiplexer = multiplexer;
					if (settings.debug_enabled){
						printf("there was a difference on multiplexer %d\n", multiplexer);
						printf("before\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",heaterStatus.ledValues[0],heaterStatus.ledValues[1],heaterStatus.ledValues[2],heaterStatus.ledValues[3],heaterStatus.ledValues[4],heaterStatus.ledValues[5]);
						printf("after\tleds:\t%d\t%d\t%d\t%d\t%d\t%d\n",led[0],led[1],led[2],led[3],led[4],led[5]);
					}
					i = 0;
					for (; i<6; ++i){
						heaterStatus.ledValues[i] = led[i];
					}
					delay(updateDelay);
					continue;
				} else if (!multiplexerChanged){
					//if (settings.debug_enabled){printf("multiplexer not changed %d\n", multiplexer);}
					delay(updateDelay);
					continue;
				}
			}
			
			if (settings.debug_enabled){printf("do check\n");}
			if (led[IND_LED_POWER]){
				heaterStatus.hasPowerLedOnLastTime = runTimeMillis;
				if (!heaterStatus.hasPower){
					heaterStatus.hasPower = true;
					different = true;
				}
			} else {
				heaterStatus.hasPowerLedOffLastTime = runTimeMillis;
				if (heaterStatus.hasPower && ((runTimeMillis - heaterStatus.hasPowerLedOnLastTime) > 1000 || (runTimeMillis - heaterStatus.multiplexerLastTime1) > 200)){ //if not heating, blinking in about 500ms cycles
					heaterStatus.hasPower = false;
					different = true;
					
					//reset all values, there could not be any state if no power
					heaterStatus.isOn = false;
					heaterStatus.isModeHeating = false;
					heaterStatus.isModeKeepwarm = false;
					heaterStatus.level = 0;
				}
			}
			if (heaterStatus.hasPower){
				if(led[IND_LED_TEMP_MAX] ^ led[IND_LED_TEMP_MIDDLE] ^ led[IND_LED_TEMP_MIN]){
					//one(only one!) of the level leds are on
					if (led[IND_LED_MODE_HEATING] ^ led[IND_LED_MODE_KEEPWARM]){
						heaterStatus.isOnLastTime = runTimeMillis;
						heaterStatus.errorMsg = NULL;
						if (!heaterStatus.isOn || heaterStatus.hasError){
							heaterStatus.isOn = true;
							heaterStatus.isModeHeating = led[IND_LED_MODE_HEATING];
							heaterStatus.isModeKeepwarm = led[IND_LED_MODE_KEEPWARM];
							different = true;
						}
						if(led[IND_LED_TEMP_MAX]){
							heaterStatus.level = 3;
						} else if(led[IND_LED_TEMP_MIDDLE]){
							heaterStatus.level = 2;
						} else if(led[IND_LED_TEMP_MIN]){
							heaterStatus.level = 1;
						}
					} else {
						newHasError = true;
					}
				} else {
					newHasError = true;
				}
			}
			
			if(newHasError){
				if(heaterStatus.hasPower){
					if (heaterStatus.isOn && (runTimeMillis - heaterStatus.hasPowerLedOffLastTime) < 1000){ //if not heating, blinking in about 500ms cycles
						if (settings.debug_enabled){printf("Set isOn to false\n");}
						heaterStatus.isOn = false;
						different = true;
					} else if (!heaterStatus.isOn && (runTimeMillis - heaterStatus.hasPowerLedOffLastTime) > 1000){
						//This one is needed for correct error handling, if it is set to false in couse of error, but is still on
						if (settings.debug_enabled){printf("Set isOn to true\n");}
						heaterStatus.isOn = true;
						different = true;
					}
				}
				if (heaterStatus.isOn){
					if(different || !heaterStatus.hasError || (runTimeMillis - heaterStatus.errorLastTime) > 5000){
						if(!led[IND_LED_MODE_HEATING] && !led[IND_LED_MODE_KEEPWARM] && !led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
							//Error leds off blink state
							if (settings.debug_enabled){printf("Error leds off blink state\n");}
							heaterStatus.ledsOffBlinkState = true;
						} else {
							//There could only be on error at a time
							heaterStatus.hasError = true;
							heaterStatus.ledsOffBlinkState = false;
							heaterStatus.level = 0;
							heaterStatus.isModeHeating = false;
							heaterStatus.isModeKeepwarm = false;
							different = true;
							
							if (settings.debug_enabled){printf("check witch error it is...\n");}
							if(led[IND_LED_MODE_HEATING] && led[IND_LED_MODE_KEEPWARM] && led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
								heaterStatus.noPanError = true;
								heaterStatus.errorMsg = "No Pan";
							} else if(led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
								heaterStatus.IGBTTempToHeightError = true;
								heaterStatus.errorMsg = "IGBT Temperature To High";
							} else if(led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
								heaterStatus.tempSensorError = true;
								heaterStatus.errorMsg = "Temperature Sensor out of work";
							} else if(!led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
								heaterStatus.IGBTSensorError = true;
								heaterStatus.errorMsg = "IGBT Sensor out of work";
							} else if(!led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
								heaterStatus.voltageToHeightError = true;
								heaterStatus.errorMsg = "Voltage To High(>260V)";
							} else if(!led[IND_LED_TEMP_MAX] && !led[IND_LED_TEMP_MIDDLE] && led[IND_LED_TEMP_MIN]){
								heaterStatus.voltageToLowError = true;
								heaterStatus.errorMsg = "Voltage to Low(<85V)";
							} else if(led[IND_LED_TEMP_MAX] && led[IND_LED_TEMP_MIDDLE] && !led[IND_LED_TEMP_MIN]){
								heaterStatus.bowOutOfWaterError = true;
								heaterStatus.errorMsg = "The bow out of water";
							}
							heaterStatus.errorLastTime = runTimeMillis;
						}
					}
				}
			} else {
				if(heaterStatus.hasError){
					heaterStatus.hasError = false;
					heaterStatus.noPanError = false;
					heaterStatus.IGBTTempToHeightError = false;
					heaterStatus.tempSensorError = false;
					heaterStatus.IGBTSensorError = false;
					heaterStatus.voltageToHeightError = false;
					heaterStatus.voltageToLowError = false;
					heaterStatus.bowOutOfWaterError = false;
					heaterStatus.ledsOffBlinkState = false;
				}
			}
			delay(updateDelay);
		}
	} else if (settings.shieldVersion >= 3){
		//4 pins
	}
	return 0;
}

void *readADCValues(void *ptr){
	uint32_t delayWeight;
	uint32_t delayTempPess;
	
	if (runningMode.simulationMode){
		state.lidOpen = false;
		delayWeight = 300;
		delayTempPess = 1000;
		while (state.running){		
			if (currentCommandValues.mode == MODE_SCALE){
				double weightValue;
				if (!state.scaleReady) {
					weightValue = 0.0;
				} else {
					if (timeValues.runTime <= timeValues.simulationUpdateTime){
						delay(delayWeight);
						continue;
					} else {
						int deltaW = newCommandValues.weight-oldCommandValues.weight;
						weightValue = oldCommandValues.weight;
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
						weightValue += state.referenceForce;
						timeValues.simulationUpdateTime = timeValues.runTime;
					}
				}
				if (adc_values.Weight.valueByOffset != weightValue){
					adc_values.Weight.valueByOffset = weightValue;
					adc_values.Weight.value = adc_values.Weight.valueByOffset - state.referenceForce;
					adc_values.Weight.adc_value = adc_values.Weight.value / forceCalibration.scaleFactor;
					
					adc_values.LoadCellFrontLeft.adc_value = adc_values.Weight.adc_value;
					adc_values.LoadCellFrontLeft.value = adc_values.Weight.value;
					adc_values.LoadCellFrontLeft.valueByOffset = adc_values.Weight.valueByOffset;
					
					adc_values.LoadCellFrontRight.adc_value = adc_values.Weight.adc_value;
					adc_values.LoadCellFrontRight.value = adc_values.Weight.value;
					adc_values.LoadCellFrontRight.valueByOffset = adc_values.Weight.valueByOffset;
					
					adc_values.LoadCellBackLeft.adc_value = adc_values.Weight.adc_value;
					adc_values.LoadCellBackLeft.value = adc_values.Weight.value;
					adc_values.LoadCellBackLeft.valueByOffset = adc_values.Weight.valueByOffset;
					
					adc_values.LoadCellBackRight.adc_value = adc_values.Weight.adc_value;
					adc_values.LoadCellBackRight.value = adc_values.Weight.value;
					adc_values.LoadCellBackRight.valueByOffset = adc_values.Weight.valueByOffset;
				}
				delay(delayWeight);
			}
			if (currentCommandValues.mode != MODE_SCALE){
				double tempValue = oldCommandValues.temp;
				int deltaT=newCommandValues.temp-oldCommandValues.temp;
				if (currentCommandValues.mode==MODE_HEATUP || currentCommandValues.mode==MODE_COOK) {
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
				} else if (currentCommandValues.mode==MODE_COOLDOWN){
					tempValue = tempValue-1;
				}
				adc_values.Temp.valueByOffset = tempValue;
				adc_values.Temp.value = adc_values.Temp.valueByOffset - tempCalibration.offset;
				adc_values.Temp.adc_value = adc_values.Temp.value / tempCalibration.scaleFactor;
				
				int32_t pressValue = oldCommandValues.press;
				int deltaP=newCommandValues.press - oldCommandValues.press;
				if (currentCommandValues.mode == MODE_PRESSUP || currentCommandValues.mode == MODE_PRESSHOLD) {
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
				} else if (currentCommandValues.mode == MODE_PRESSDOWN){
					pressValue = pressValue-1;
				}
				adc_values.Press.valueByOffset = pressValue;
				adc_values.Press.value = pressValue - pressCalibration.offset;
				adc_values.Press.adc_value = adc_values.Press.value / pressCalibration.scaleFactor;
				delay(delayTempPess);
			}
		}
		return 0;
	}
	
	delayWeight = 10;
	delayTempPess = 100;
	while (state.running){
		if (currentCommandValues.mode == MODE_SCALE || millis() - timeValues.lastWeightUpdateTime > 2000 || runningMode.calibration || runningMode.measure_noise){
			adc_values.LoadCellFrontLeft.adc_value = readADC(adc_config.ADC_LoadCellFrontLeft);
			if (adc_config.inverse[adc_config.ADC_LoadCellFrontLeft]){
				adc_values.LoadCellFrontLeft.adc_value = 0x00FFFFFF - adc_values.LoadCellFrontLeft.adc_value;
			}
			adc_values.LoadCellFrontLeft.value = (double)adc_values.LoadCellFrontLeft.adc_value * forceCalibration.scaleFactor;
			adc_values.LoadCellFrontLeft.valueByOffset=adc_values.LoadCellFrontLeft.value + state.referenceForce;
			adc_values.LoadCellFrontLeft.valueByOffset = round(adc_values.LoadCellFrontLeft.valueByOffset);
			
			adc_values.LoadCellFrontRight.adc_value = readADC(adc_config.ADC_LoadCellFrontRight);
			if (adc_config.inverse[adc_config.ADC_LoadCellFrontRight]){
				adc_values.LoadCellFrontRight.adc_value = 0x00FFFFFF - adc_values.LoadCellFrontRight.adc_value;
			}
			adc_values.LoadCellFrontRight.value = (double)adc_values.LoadCellFrontRight.adc_value * forceCalibration.scaleFactor;
			adc_values.LoadCellFrontRight.valueByOffset=adc_values.LoadCellFrontRight.value + state.referenceForce;
			adc_values.LoadCellFrontRight.valueByOffset = round(adc_values.LoadCellFrontRight.valueByOffset);
			
			adc_values.LoadCellBackLeft.adc_value = readADC(adc_config.ADC_LoadCellBackLeft);
			if (adc_config.inverse[adc_config.ADC_LoadCellBackLeft]){
				adc_values.LoadCellBackLeft.adc_value = 0x00FFFFFF - adc_values.LoadCellBackLeft.adc_value;
			}
			adc_values.LoadCellBackLeft.value = (double)adc_values.LoadCellBackLeft.adc_value * forceCalibration.scaleFactor;
			adc_values.LoadCellBackLeft.valueByOffset=adc_values.LoadCellBackLeft.value + state.referenceForce;
			adc_values.LoadCellBackLeft.valueByOffset = round(adc_values.LoadCellBackLeft.valueByOffset);
			
			adc_values.LoadCellBackRight.adc_value = readADC(adc_config.ADC_LoadCellBackRight);
			if (adc_config.inverse[adc_config.ADC_LoadCellBackRight]){
				adc_values.LoadCellBackRight.adc_value = 0x00FFFFFF - adc_values.LoadCellBackRight.adc_value;
			}
			adc_values.LoadCellBackRight.value = (double)adc_values.LoadCellBackRight.adc_value * forceCalibration.scaleFactor;
			adc_values.LoadCellBackRight.valueByOffset=adc_values.LoadCellBackRight.value + state.referenceForce;
			adc_values.LoadCellBackRight.valueByOffset = round(adc_values.LoadCellBackRight.valueByOffset);
			
			adc_values.Weight.adc_value = (adc_values.LoadCellFrontLeft.adc_value + adc_values.LoadCellFrontRight.adc_value + adc_values.LoadCellBackLeft.adc_value + adc_values.LoadCellBackRight.adc_value) / 4;
			adc_values.Weight.value = (double)adc_values.Weight.adc_value * forceCalibration.scaleFactor;
			adc_values.Weight.valueByOffset=adc_values.Weight.value + state.referenceForce;
			adc_values.Weight.valueByOffset = round(adc_values.Weight.valueByOffset);
			
			timeValues.lastWeightUpdateTime = millis();
			
			if (currentCommandValues.mode != MODE_SCALE){
				//check isLidOpen
				double front = (adc_values.LoadCellFrontLeft.valueByOffset + adc_values.LoadCellFrontRight.valueByOffset) / 2 - adc_values.Weight.valueByOffset;
				double back = (adc_values.LoadCellBackLeft.valueByOffset + adc_values.LoadCellBackRight.valueByOffset) / 2 - adc_values.Weight.valueByOffset;
				
				double diff = front - back;
				state.lidOpen = diff < -1000;
				if (settings.debug3_enabled) {printf("lidopen front: %2.f, back: %2.f, diff: %2.f, isOpen: %d\n", front, back, diff, state.lidOpen);}
			}
			
			if (runningMode.measure_noise){
				if (adc_values.LoadCellFrontLeft.adc_value > adc_noise.MaxWeight1) adc_noise.MaxWeight1 = adc_values.LoadCellFrontLeft.adc_value;
				if (adc_values.LoadCellFrontLeft.adc_value < adc_noise.MinWeight1) adc_noise.MinWeight1 = adc_values.LoadCellFrontLeft.adc_value;
				adc_noise.DeltaWeight1 = adc_noise.MaxWeight1 - adc_noise.MinWeight1;
				
				if (adc_values.LoadCellFrontRight.adc_value > adc_noise.MaxWeight2) adc_noise.MaxWeight2 = adc_values.LoadCellFrontRight.adc_value;
				if (adc_values.LoadCellFrontRight.adc_value < adc_noise.MinWeight2) adc_noise.MinWeight2 = adc_values.LoadCellFrontRight.adc_value;
				adc_noise.DeltaWeight2 = adc_noise.MaxWeight2 - adc_noise.MinWeight2;
				
				if (adc_values.LoadCellBackLeft.adc_value > adc_noise.MaxWeight3) adc_noise.MaxWeight3 = adc_values.LoadCellBackLeft.adc_value;
				if (adc_values.LoadCellBackLeft.adc_value < adc_noise.MinWeight3) adc_noise.MinWeight3 = adc_values.LoadCellBackLeft.adc_value;
				adc_noise.DeltaWeight3 = adc_noise.MaxWeight3 - adc_noise.MinWeight3;
				
				if (adc_values.LoadCellBackRight.adc_value > adc_noise.MaxWeight4) adc_noise.MaxWeight4 = adc_values.LoadCellBackRight.adc_value;
				if (adc_values.LoadCellBackRight.adc_value < adc_noise.MinWeight4) adc_noise.MinWeight4 = adc_values.LoadCellBackRight.adc_value;
				adc_noise.DeltaWeight4 = adc_noise.MaxWeight4 - adc_noise.MinWeight4;
			}
			
			delay(delayWeight);
		}
		if (currentCommandValues.mode != MODE_SCALE || runningMode.calibration || runningMode.measure_noise){
			adc_values.Temp.adc_value = readADC(adc_config.ADC_Temp);
			if (adc_config.inverse[adc_config.ADC_Temp]){
				adc_values.Temp.adc_value = 0x00FFFFFF - adc_values.Temp.adc_value;
			}
			adc_values.Temp.value = (double)adc_values.Temp.adc_value * tempCalibration.scaleFactor;
			adc_values.Temp.valueByOffset=adc_values.Temp.value + tempCalibration.offset;
			adc_values.Temp.valueByOffset = round(adc_values.Temp.valueByOffset);
			
			if (runningMode.measure_noise) {
				if (adc_values.Temp.adc_value > adc_noise.MaxTemp) adc_noise.MaxTemp = adc_values.Temp.adc_value;
				if (adc_values.Temp.adc_value < adc_noise.MinTemp) adc_noise.MinTemp = adc_values.Temp.adc_value;
				adc_noise.DeltaTemp = adc_noise.MaxTemp - adc_noise.MinTemp;
			}
			
			adc_values.Press.adc_value = readADC(adc_config.ADC_Press);
			if (adc_config.inverse[adc_config.ADC_Press]){
				adc_values.Press.adc_value = 0x00FFFFFF - adc_values.Press.adc_value;
			}
			adc_values.Press.value = (double)adc_values.Press.adc_value * pressCalibration.scaleFactor;
			adc_values.Press.valueByOffset=adc_values.Press.value + pressCalibration.offset;
			adc_values.Press.valueByOffset = round(adc_values.Press.valueByOffset);
			if (runningMode.measure_noise){
				if (adc_values.Press.adc_value > adc_noise.MaxPress) adc_noise.MaxPress = adc_values.Press.adc_value;
				if (adc_values.Press.adc_value < adc_noise.MinPress) adc_noise.MinPress = adc_values.Press.adc_value;
				adc_noise.DeltaPress = adc_noise.MaxPress - adc_noise.MinPress;
			}
			
			delay(delayTempPess);
		}
	}
	return 0;
}


void *handleButtons(void *ptr){
	int buttonDelay = 10;
	printf("handleButtons\n");
	uint32_t buttonCount = sizeof(buttonValues.button) / sizeof(buttonValues.button[0]);
	printf("buttonCount: %d\n", buttonCount);
	printf("-------------------> initial delay...\n");
	delay(30000);
	printf("-------------------> initial delay done!\n");
	uint32_t but[buttonCount];
	uint8_t i = 0;
	uint32_t runTimeMillis = 0;
	bool changed;
	while (state.running){
		runTimeMillis = millis();
		i=0;
		changed = false;
		for(; i<buttonCount; ++i){
			but[i] = readButton(i);
			if (but[i]){
				if (but[i] != buttonValues.button[i]){
					buttonValues.buttonOnTime[i] = runTimeMillis;
					changed = true;
				}
				buttonValues.buttonPressedTime[i] = runTimeMillis - buttonValues.buttonOnTime[i];
			} else {
				if (but[i] != buttonValues.button[i]){
					buttonValues.buttonOffTime[i] = runTimeMillis;
					changed = true;
				}
			}
			buttonValues.button[i] = but[i];
			if (changed){
				printf("\t%d (pressed: %d, ontime: %d, offtime: %d)", but[i], buttonValues.buttonPressedTime[i], buttonValues.buttonOnTime[i], buttonValues.buttonOffTime[i]);
			}
		}
		if (changed){
			printf("\n");
		}
		
		//Check button functions
		uint8_t buttonNumber = 2;
		if (buttonValues.button[buttonNumber]){
			//if button is (still) pressed
			if (buttonValues.buttonPressedTime[buttonNumber] > 60000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speak("no more commands");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 50000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speak("release for shutdown");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 40000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speak("release for ???");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 30000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speak("release for read IP");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 20000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speak("release for restart network");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 10000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 5000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speak("release for restart deamon & middleware");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 5000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 5000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speak("release for switch WLAN mode");
				}
			}
		} else if (buttonValues.actionStartedTime[buttonNumber] > 0 && runTimeMillis - buttonValues.actionStartedTime[buttonNumber] < 60000) {
			buttonValues.actionStartedTime[buttonNumber] = 0;
			//if button is not pressed (any more) and release is less then 60 seconds ago
			if (buttonValues.buttonPressedTime[buttonNumber] > 50000){
				char command[400];
				sprintf(command, "sudo halt");
				if (settings.debug_enabled || settings.debug3_enabled || settings.debug4_enabled){printf(command);}
				int result = system( command );
				if (result != 0){
					printf("ERROR: return code of command '%s' was %d", command, result);
				} else {
					exit(0);
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 40000){
				speak("release for ???");
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 30000){
				char command[400];
				//sprintf(command, "%s/getServerIp.sh | espeak -a 200 -v %s 2>&1", settings.installPath, state.language);
				sprintf(command, "%s/getServerIp.sh | espeak -a 200 2>&1 > /dev/nul", settings.installPath);
				if (settings.debug_enabled || settings.debug3_enabled || settings.debug4_enabled){printf(command);}
				int result = system( command );
				if (result != 0){
					printf("ERROR: return code of command '%s' was %d", command, result);
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 20000){
				char command[400];
				sprintf(command, "%s/installSettings restartNetwork", settings.installPath);
				if (settings.debug_enabled || settings.debug3_enabled || settings.debug4_enabled){printf(command);}
				int result = system( command );
				if (result != 0){
					printf("ERROR: return code of command '%s' was %d", command, result);
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 10000){
				char command[400];
				sprintf(command, "%s/installSettings restartDaemonAndMiddleware", settings.installPath);
				if (settings.debug_enabled || settings.debug3_enabled || settings.debug4_enabled){printf(command);}
				int result = system( command );
				if (result != 0){
					printf("ERROR: return code of command '%s' was %d", command, result);
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 5000){
				char command[400];
				sprintf(command, "%s/installSettings change_wlanmode toggle", settings.installPath);
				if (settings.debug_enabled || settings.debug3_enabled || settings.debug4_enabled){printf(command);}
				int result = system( command );
				if (result != 0){
					printf("ERROR: return code of command '%s' was %d", command, result);
				}
			}
		}
		
		buttonNumber = 0;
		if (buttonValues.button[buttonNumber]){
			if (buttonValues.buttonPressedTime[buttonNumber] > 20000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speak("no more commands");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 10000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 5000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speak("release for switch always read mode");
				}
			} else {
				if (buttonValues.buttonPressedTime[buttonNumber] < 500 && runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 5000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speak(state.actionText);
				}
			}
		} else if (buttonValues.actionStartedTime[buttonNumber] > 0 && runTimeMillis - buttonValues.actionStartedTime[buttonNumber] < 20000) {
			buttonValues.actionStartedTime[buttonNumber] = 0;
			if (buttonValues.buttonPressedTime[buttonNumber] > 10000){
				state.alwaysReadMode = !state.alwaysReadMode;
				if (state.alwaysReadMode){
					speak("always read mode is now on");
				} else {
					speak("always read mode is now off");
				}
			} else if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] <= 5000){
				speak(state.actionText);
			}
		}
		delay(buttonDelay);
	}
	return 0;
}

void speak(char* text){
/*
	if (fork() == 0){
		char[400] command;
		sprintf(command, "echo \"%s\" | espeak -a 200 -v %s 2>&1", text, state.language);
		int result = system( command );	
		exit(0);
	}
*/
	if (settings.debug_enabled || settings.debug3_enabled || settings.debug4_enabled){printf("start speak thread\n");}
	pthread_t threadSpeak;
	//pthread_create(&threadSpeak, NULL, speakThreadFunc, NULL);
	char* textpointer = &text[0];
	pthread_create(&threadSpeak, NULL, speakThreadFunc, (void *) textpointer);
	//pthread_join(threadSpeak, NULL);
}

void *speakThreadFunc(void *ptr){
	char* text = (char *) ptr;
	char command[400];
	//sprintf(command, "echo \"%s\" | espeak -a 200 -v %s 2>&1", text, state.language);
	sprintf(command, "echo \"%s\" | espeak -a 200 -v german 2>&1 > /dev/nul", text);
	//sprintf(command, "echo \"%s\" | espeak -a 200 2>&1 > /dev/nul", text);
	if (settings.debug_enabled || settings.debug3_enabled || settings.debug4_enabled){printf("%s\n", command);}
	int result = system( command );
	pthread_exit(0);
	return (void *)result;
}
