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
//#include "atmel.h"
//#include <wiringSerial.h>
#include "virtualspiAtmel.h"

#include <stdarg.h>
#include <unistd.h>

const uint32_t MIN_STATUS_INTERVAL = 20;

const uint8_t dataType = TYPE_TEXT;

const double SECONDS_PER_HOUR = 3600; //60*60

const uint8_t HOUR_COUNTER_VERSION = 0;

struct ADC_Config adc_config = {0,1,2,3,4,5, 0,8, {0x710, 0x711, 0x712, 0x713, 0x714, 0x115}, {0,0,0,0,0,0}, 2, 2, false};
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

struct Settings settings = {0, 5,0,1, 1.0, 1,1, 40,10, 500,1, false, "127.0.0.1",8000,true,true, false,false,false,false, 100,800, 8,0x0000,0x0000, "config","/opt/EveryCook/daemon/calibration","/dev/shm/command","/dev/shm/status","/var/log/EveryCook_Daemon.log","/opt/EveryCook/daemon/hourCounter","/opt/EveryCook/","/dev/ttyUSB9"};

struct State state = {true,true,true, 1/*setting.ShortDelay*/, 0,false, false, true, ' ',false, -1,"", 0};

struct Heater_Led_Values heaterStatusEx = {};

struct Daemon_Values daemon_values;

//struct HourCounter hourCounter = {['H','C','V', HOUR_COUNTER_VERSION], 0.0, 0.0, 0.0};
struct HourCounter hourCounter = {"HCV\0", 0.0, 0.0, 0.0};

struct Button_Config buttonConfig = {{17,27,22},{0,0,0}};
struct Button_Values buttonValues;

bool weightSlowly=false;

pthread_t threadReadADCValues;
pthread_t threadHandleButtons;

//for the new mode
#define HELP 0
#define QUIT 1

#define TEST_TEMPPRESS 3
#define TEST_VALVE 4
#define TEST_BOUTON 5
#define TEST_DISPLAY 6
#define TEST_FAN 7
#define TEST_INDUCTION 8
#define TEST_MOTOR 9
#define TEST_WEIGHT 10
#define TEST_SPEAKER 11

#define WIDHTCOLUMN 10
pthread_t readInput;// thread that read input on keyboard

int newModeMain();
void printColumnVAr( uint16_t widthColumn, uint16_t nbColumn,...);
void printMiddle(uint16_t space, uint32_t nb);
void *readInputFunction(void *ptr);
void helpPrintf();
void quitPrintf();
void testServoPrintf();
void testTempPressPrintf();
void testValvePrintf();
void testBoutonPrintf();
void testInductionPrintf();
void testSDisplayPrintf();
void testFanPrintf();
void testMotorPrintf();
void testWeightPrintf();
void testSpeakerPrintf();

char * modeNom = "mode test"; //name of the mode for the display
int modeNum = 0; //number of mode for the switch
int running=1;
//TEST_FAN
uint16_t fanPwm=0;
int fantemp=0;
//TEST_MOTOR
uint16_t motorPWMDesired=0;
uint16_t motorRPMTrued=0;
uint16_t motorRPMDesired=0;
uint16_t motorSensord=0;
//TEST_INDUCTION
uint16_t inductionPwm=0;
bool inductionIsRunning=false;
//TEST_BOUTON
bool boutonState[6]={false,false,false,false,false,false};
//TEST_DISPLAY
bool displayLedShined[8]={false,false,false,false,false,false,false,false};
uint16_t displayMode=0;
//TEST_VALVE
uint16_t solenoidPwm=0;
bool solenoidIsOpen=false;
//TEST_WEIGHT
#define NBWEIGHTLINE 15
struct Weight_Data {
	uint32_t frontL;
	uint32_t frontR;
	uint32_t backL;
	uint32_t backR;
	uint32_t average;
	uint32_t averageG;
};
struct Weight_Data_tab{
	struct Weight_Data max;
	struct	Weight_Data min;
	struct Weight_Data delta;
	struct	Weight_Data avg;
	uint32_t nbData;
};
struct Weight_Data allWeightData[NBWEIGHTLINE]; 
struct Weight_Data newWeightData;
struct Weight_Data_tab tabWeightData;
//TEST_TEMPPRESS
#define NBTEMPPRESSLINE 15
struct Temppress_Data{
	uint32_t tempInput;
	uint32_t tempDeg;
	uint32_t pressInput;
	uint32_t pressPasc;
};
struct Temppress_Data_tab{
	struct Temppress_Data max;
	struct	Temppress_Data min;
	struct Temppress_Data delta;
	struct	Temppress_Data avg;
	uint32_t nbData;
};
struct Temppress_Data allTemppressData[NBTEMPPRESSLINE]; 
struct Temppress_Data newTemppressData;
struct Temppress_Data_tab tabTemppressData;
//DEBUG
uint8_t Vdebug;
//end for the new mode

/** @brief program different mode according to what is chosen during the launch
 *  @param argc : mode number selected
 *  @param argv : mode which has been selected
*/
int parseParams(int argc, const char* argv[]);
/** @brief initialization signals
 */
void defineSignalHandler();

void handleSignal(int signum);

/** @brief clear HourCounter
*/
void clearHourCounter();
/** @brief creation of file
 */
void initOutputFile(void);
/** @brief check if new information  were send by web
*/
bool checkForInput();

/** @ write all output file to communicate with web
*/
void doOutput();


/** @brief calculate the status of heater in according to the shield Version
*/

void *readADCValues(void *ptr);
/** @brief return the value of buttons and speak
*/
void *handleButtons(void *ptr);

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
	daemon_values.i2c_solenoid_values = &i2c_solenoid_values;
	daemon_values.i2c_motor_values = &i2c_motor_values;
	daemon_values.newCommandValues = &newCommandValues;
	daemon_values.currentCommandValues = &currentCommandValues;
	daemon_values.oldCommandValues = &oldCommandValues;
	daemon_values.timeValues = &timeValues;
	daemon_values.runningMode = &runningMode;
	daemon_values.settings = &settings;
	daemon_values.state = &state;
	daemon_values.heaterStatus = &heaterStatusEx;
	daemon_values.hourCounter = &hourCounter;
	daemon_values.buttonConfig = &buttonConfig;
	daemon_values.buttonValues = &buttonValues;
	daemon_values.adc_values = &adc_values;
	
	printf("starting EveryCook daemon...\n");
	if (daemonGetSettingsDebug_enabled()){printf("main\n");}
	
	int result=parseParams(argc, argv); // reading modes that were selected
	if (result != -1){   // end of the program depending on the mode
		return result;
	}
	defineSignalHandler(); //initialization signals
	state.alwaysReadMode = false;
	
	StringClean(state.TotalUpdate, 512);//clean state.TotalUpdate
	ReadConfigurationFile();// read the file configuration
	ReadCalibrationFile(); // read the file calibration
	
	initHardware(settings.shieldVersion, buttonConfig.button_pin, buttonConfig.button_inverse); // initilalization PIN
	delay(30);
	
	if (settings.shieldVersion >= 4){
		VirtualSPIAtmelInit(daemonGetSettingsDebug_enabled()); // initilalization PIN (SPI)
		virtualAtmelStartSPI();
	}
	
	initOutputFile();// création of the file wich is use for web
	state.Delay = settings.LongDelay;
	
	resetValues(); //reset all value
	state.referenceForce = forceCalibration.offset; //for default reference Force in calibration mode
	if (settings.shieldVersion < 4){
		SegmentDisplaySimple(' ', &state, &i2c_config);
	}
	
	if (daemonGetSettingsDebug_enabled()){printf("commandFile is: %s\n", settings.commandFile);}
	if (daemonGetSettingsDebug_enabled()){printf("statusFile is: %s\n", settings.statusFile);}
	
	//remove commandfile so no unexpectet action will be done
	if (remove(settings.commandFile) != 0){
		printf("Error while remove old commandfile: %s\n", settings.commandFile);
	}
		
	timeValues.runTime = time(NULL); //TODO WIA CHANGE THIS
	printf("runtime is now: %d \n", daemonGetTimeValuesRunTime());
	timeValues.stepStartTime = daemonGetTimeValuesRunTime(); //TODO WIA CHANGE THIS
	
	
	if (daemonGetSettingsDebug_enabled()){printf("runningModes: normalMode:%d, calibration:%d, measure_noise:%d, test_7seg:%d, test_servo:%d, test_heat_led:%d, test_motor:%d, test_buttons:%d, test_adc:%d, test_heating_power:%d, test_heating_press:%d, test_serial:%d\n", runningMode.normalMode, runningMode.calibration, runningMode.measure_noise, runningMode.test_7seg, runningMode.test_servo, runningMode.test_heat_led, runningMode.test_motor, runningMode.test_buttons, runningMode.test_adc, runningMode.test_heating_power, runningMode.test_heating_press, runningMode.test_serial);}
	
	
	if (runningMode.normalMode){
		if (settings.shieldVersion < 4){// only with shield version 1,2 and 3
			heaterStartThreadLedReader();
		}
		pthread_create(&threadReadADCValues, NULL, readADCValues, NULL);// return temp,press and loadcell
		if(settings.shieldVersion != 1){// only if shield version is not 1
			pthread_create(&threadHandleButtons, NULL, handleButtons, NULL);//gestionndes bouttons
		}
		
		//Show Smilly / startscreen
		if (settings.shieldVersion >= 4){
									//00##############
			uint16_t picture[9] = {	0b0000000111110000,
									0b0000001000001000,
									0b0000010000000100,
									0b0000010100010100,
									0b0000010000000100,
									0b0000010100010100,
									0b0000010011100100,
									0b0000001000001000,
									0b0000000111110000};
			displayShowPicture(&picture[0]);
		}
		
		while (state.running){
			if (daemonGetSettingsDebug_enabled()){printf("main loop...\n");}
			if (timeValues.lastRunTime != daemonGetTimeValuesRunTime()){// if new start
				state.timeChanged = true;
				timeValues.lastRunTime = daemonGetTimeValuesRunTime();//new time start
			}
			bool valueChanged = checkForInput();//check if new value
			
			timeValues.runTime = time(NULL); //TODO WIA CHANGE THIS
			timeValues.runTimeMillis = millis();
			
			if (valueChanged){//if new information
				if (daemonGetSettingsDebug_enabled()){printf("valueChanged=true\n");}
				//TODO: timeValues.lastFileChangeTime = timeValues.runTime;
				evaluateInput();//look what have change
			}
			ProcessCommand();//control all output electronique system
			currentCommandValues.time = daemonGetTimeValuesRunTime() - timeValues.stepStartTime;
			
			doOutput();// control all output for web
			
			if (daemonGetSettingsDebug_enabled()){printf("main loop end.\n");}
			delay(state.Delay);
		}
		heaterOff();
		//cheating so it will realy stop motor
		motorSetI2cValuesMotorRpm(motorGetI2cValuesSpeedMin());
		motorSetI2cValuesDestRpm(motorGetI2cValuesMotorRpm());
		motorSetCommandRPM(0);
		
		if (settings.shieldVersion < 3){
			setServoOpen(0, 1, 0, &daemon_values);//open valve with servo 
		} else {
			solenoidSetOpen(false);//open solenoide valve
		}
		if(settings.shieldVersion != 1){
			pthread_join(threadHandleButtons, NULL);// wait end of thread
		}
		pthread_join(threadReadADCValues, NULL);// wait end of thread
		if (settings.shieldVersion < 4){
			heaterStopThreadLedReader();// wait end of thread
		}
	} else if (runningMode.calibration || runningMode.measure_noise){
		if (runningMode.calibration) {
			heaterStartThreadLedReader();
		}
		pthread_create(&threadReadADCValues, NULL, readADCValues, NULL);// return temp,press and loadcell
		while (state.running){
			timeValues.runTime = time(NULL);
			timeValues.runTimeMillis = millis();
			//printf("we are in calibration mode, be careful!\n Heat will turn on automatically if switch is on!\n Use switch to turn heat off.\n");	
			if (runningMode.calibration) heaterOn();// heat turn on
			if (daemonGetSettingsDebug_enabled() || runningMode.calibration || daemonGetSettingsDebug3_enabled() || runningMode.measure_noise) {	
				printf("time %8d | ", timeValues.runTimeMillis);
			}
			
			if (daemonGetSettingsDebug_enabled() || runningMode.calibration || daemonGetSettingsDebug3_enabled()){printf("Temp %d dig %.0f °C | ", adc_values.Temp.adc_value, adc_values.Temp.valueByOffset);}
			if (runningMode.measure_noise) {printf("NoiseTemp %d | ", adc_noise.DeltaTemp);}
			
			if (daemonGetSettingsDebug_enabled() || runningMode.calibration || daemonGetSettingsDebug3_enabled()){printf("Press %d digits %.0f kPa | ", adc_values.Press.adc_value, adc_values.Press.valueByOffset);}
			if (runningMode.measure_noise){printf("NoisePress %d |", adc_noise.DeltaPress);}
			
			if (daemonGetSettingsDebug_enabled() || runningMode.calibration){printf("Weight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", adc_values.Weight.adc_value, adc_values.Weight.value, adc_values.Weight.valueByOffset, adc_values.LoadCellFrontLeft.adc_value, adc_values.LoadCellFrontRight.adc_value, adc_values.LoadCellBackLeft.adc_value, adc_values.LoadCellBackRight.adc_value);}			
			if (runningMode.measure_noise){
				printf("NoiseWeightFL %d | ", adc_noise.DeltaWeight1);
				printf("NoiseWeightFR %d | ", adc_noise.DeltaWeight2);
				printf("NoiseWeightBL %d | ", adc_noise.DeltaWeight3);
				printf("NoiseWeightBR %d\n", adc_noise.DeltaWeight4);
			}
			SegmentDisplay();
			delay(state.Delay);
		}
		pthread_join(threadReadADCValues, NULL);// wait he end of thread
		if (runningMode.calibration){
			if (runningMode.calibration) heaterOff();
			heaterStopThreadLedReader();
		}
	} else if (runningMode.test_7seg){
		//newMode
		
		printf("\nnew mode is going to start");
		if (settings.shieldVersion < 4){// only with shield version 1,2 and 3
			heaterStartThreadLedReader();
		}		
		pthread_create(&threadReadADCValues, NULL, readADCValues, NULL);// return temp,press and loadcell
		if(settings.shieldVersion != 1){// only if shield version is not 1
			pthread_create(&threadHandleButtons, NULL, handleButtons, NULL);//gestionndes bouttons
		}
		printf("\nnewModeMain is going to start");
		newModeMain();
		printf("\nend of newModeMain");
		heaterOff();
		printf("\nheater is stopped");
		//cheating so it will realy stop motor
		motorSetI2cValuesMotorRpm(motorGetI2cValuesSpeedMin());
		motorSetI2cValuesDestRpm(motorGetI2cValuesMotorRpm());
		motorSetCommandRPM(0);
		printf("\nmotor is stopped");
		if (settings.shieldVersion < 3){
			setServoOpen(0, 1, 0, &daemon_values);//open valve with servo 
		} else {
			solenoidSetOpen(false);//open solenoide valve
		}
		if(settings.shieldVersion != 1){
			pthread_join(threadHandleButtons, NULL);// wait end of thread
		}
		pthread_join(threadReadADCValues, NULL);// wait end of thread
		if (settings.shieldVersion < 4){
			heaterStopThreadLedReader();// wait end of thread
		}
		//End newMode
		
		/*if (settings.shieldVersion < 4){//there is a 7 seg
			while (state.running){
				blink7Segment(&i2c_config);
				delay(state.Delay);
			}
		} else {// 7 seg is not implemented
			printf("There is no 7seg display in shieldVersion %d\n", settings.shieldVersion);
		}*/
	} else if (runningMode.test_servo){
		if (settings.shieldVersion < 4){// there is a servo
			int16_t diff = settings.test_servo_max-settings.test_servo_min;
			int16_t step = diff / 20;
			uint16_t value=settings.test_servo_min;
			if (step == 0){//if step=0 servo go at une position
				printf("%d\n",value);
				writeI2CPin(i2c_config.i2c_servo, value);
			} else if (step>0){//if step>0 servo begin at min position and go to max step by step
				printf(">0 step: %d\n",step);
				while (value<=settings.test_servo_max && state.running){
					printf("%d\n",value);
					writeI2CPin(i2c_config.i2c_servo, value);
					delay(500);
					value=value+step;
				}
			} else {//if step<0 servo begin at max position and go to min step by step
				printf("<0 step: %d\n",step);
				while (value>=settings.test_servo_max && state.running){
					printf("%d\n",value);
					writeI2CPin(i2c_config.i2c_servo, value);
					delay(500);
					value=value+step;
				}
			}
			delay(2000);
		} else {
			printf("There is no servo in shieldVersion %d, so open/close solenoid if min value is >0/=0\n", settings.shieldVersion);
			solenoidSetOpen(settings.test_servo_min>0);
		}
	} else if (runningMode.test_heat_led){
		heaterTestHeatLed();
	} else if (runningMode.test_motor){
		int16_t diff = settings.test_servo_max-settings.test_servo_min;
		int16_t step = diff / 10;
		uint16_t value=settings.test_servo_min;
		printf("motor is i2c pin: %d\n", motorGetI2cConfig());
		if (step == 0){// turn the motor at one speed
			printf("%d\n",value);
			motorSetSpeedRPM(value);
		} else if (step>0){//the motor speed will increase from minimum to maximum requested
			printf(">0 step: %d\n",step);
			while (value<=settings.test_servo_max && state.running){
				printf("%d\n",value);
				motorSetSpeedRPM(value);
				delay(500);
				value=value+step;
			}
		} else {//the motor speed will increase from maximum to minimum requested
			printf("<0 step: %d\n",step);
			while (value>=settings.test_servo_max && state.running){
				printf("%d\n",value);
				motorSetSpeedRPM(value);
				delay(500);
				value=value+step;
			}
		}
		delay(2000);
		printf("set motor back to 0\n");
		motorSetSpeedRPM(0);//turn off
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
				if (but[i]){//button pressed
					if (but[i] != buttonValues.button[i]){//button pressed first time
						buttonValues.buttonOnTime[i] = timeValues.runTimeMillis;
					}
					buttonValues.buttonPressedTime[i] = timeValues.runTimeMillis - buttonValues.buttonOnTime[i];
				} else {//button released
					if (but[i] != buttonValues.button[i]){//button is releasing
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
		if (settings.use_spi_dev){//SPI configuration
			ad7794_reset(&adc);
			delay(30);
		
			ad7794_write_data(&adc, AD7794_MODE, settings.test_ADC_update_rate & 0x000F);
			ad7794_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
		} else {//SPI configuration
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
					if (daemonGetSettingsDebug_enabled()){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
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
					if (daemonGetSettingsDebug_enabled()){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
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
					if (daemonGetSettingsDebug_enabled()){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
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
					if (daemonGetSettingsDebug_enabled()){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
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
			if (daemonGetSettingsDebug_enabled()){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
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
		heaterStartThreadLedReader();
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
		
		beeperBeepEndStep();
		//time for initialize weight
		currentCommandValues.mode = MODE_SCALE;
		delay(2000);
		currentCommandValues.mode = MODE_STANDBY;
		
		printf("Open lid (and remove stirrer if inserted)\n");
		while (state.running){
			timeValues.runTime = time(NULL);
			timeValues.runTimeMillis = millis();
			if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled()){printf("time: %9d\tWeight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", timeValues.runTimeMillis,  adc_values.Weight.adc_value, adc_values.Weight.value, adc_values.Weight.valueByOffset, adc_values.LoadCellFrontLeft.adc_value, adc_values.LoadCellFrontRight.adc_value, adc_values.LoadCellBackLeft.adc_value, adc_values.LoadCellBackRight.adc_value);}
			if (currentTestPart == 0){
				if (!state.lidClosed){
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
						beeperBeepSeconde(1);
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
				if (state.lidClosed){
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
			} else if (currentTestPart == 3){//until 40 degre
				motorSetCommandRPM(50);
				heaterOn();
				currentCommandValues.press = adc_values.Press.valueByOffset;
				currentCommandValues.temp = adc_values.Temp.valueByOffset;
				
				if (lastTemp != adc_values.Temp.valueByOffset || timeValues.runTimeMillis - lastOutputTime > 5000) {//if temperature change or 5s
					printf("time: %5.3f\tcurrent temp: %.0f\n", (double)timeValues.runTimeMillis / 1000, adc_values.Temp.valueByOffset);
					lastOutputTime = timeValues.runTimeMillis;
					lastTemp = adc_values.Temp.valueByOffset;
				}
				if (adc_values.Temp.valueByOffset >= 40){//if temp > 40
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
			} else if (currentTestPart == 4){//until 90 degre
				motorSetCommandRPM(50);
				heaterOn();
				currentCommandValues.press = adc_values.Press.valueByOffset;
				currentCommandValues.temp = adc_values.Temp.valueByOffset;
				
				if (lastTemp != adc_values.Temp.valueByOffset || timeValues.runTimeMillis - lastOutputTime > 5000) {//if temperature change or 5s
					printf("time: %5.3f\tcurrent temp: %.0f\n", (double)timeValues.runTimeMillis / 1000, adc_values.Temp.valueByOffset);
					lastOutputTime = timeValues.runTimeMillis;
					lastTemp = adc_values.Temp.valueByOffset;
				} 
				if (adc_values.Temp.valueByOffset >= 90){//if temp > 40
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
			} else if (currentTestPart == 5){// heat and motor turn off
				heaterOff();
				motorSetCommandRPM(0);
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
		heaterOff();
		heaterStopThreadLedReader();
	} else if (runningMode.test_heating_press){
		heaterStartThreadLedReader();
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
		
		beeperBeepEndStep();
		//time for initialize weight
		currentCommandValues.mode = MODE_SCALE;
		delay(2000);
		currentCommandValues.mode = MODE_STANDBY;
		
		printf("Open lid (and remove stirrer if inserted)\n");
		while (state.running){
			timeValues.runTime = time(NULL);
			timeValues.runTimeMillis = millis();
			if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled()){printf("time: %9d\tWeight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", timeValues.runTimeMillis,  adc_values.Weight.adc_value, adc_values.Weight.value, adc_values.Weight.valueByOffset, adc_values.LoadCellFrontLeft.adc_value, adc_values.LoadCellFrontRight.adc_value, adc_values.LoadCellBackLeft.adc_value, adc_values.LoadCellBackRight.adc_value);}
			if (currentTestPart == 0){//put 0,5 liter of watter
				if (!state.lidClosed){
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
			} else if (currentTestPart == 1){// insert stirrer, close lid, add and lock pusher
				printf("time: %5.3f\tWeight: %.0fg\n", (double)timeValues.runTimeMillis / 1000, adc_values.Weight.valueByOffset);
				if (adc_values.Weight.valueByOffset >= 500){
					if (partReachedTime == 0){
						beeperBeepSeconde(1);
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
			} else if (currentTestPart == 2){// verifi stirrer is in and lid is closed
				if (state.lidClosed){
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
			} else if (currentTestPart == 3){//until PA > 1
				motorSetCommandRPM(50);
				heaterOn();
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
			} else if (currentTestPart == 4){//until PA > 1,8
				motorSetCommandRPM(50);
				heaterOn();
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
			} else if (currentTestPart == 5){// heat and motor turn off
				heaterOff();
				motorSetCommandRPM(0);
				if (settings.shieldVersion < 3){
					setServoOpen(100, 10, 1000, &daemon_values);
				} else {
					solenoidSetOpen(false);
				}
				currentCommandValues.press = adc_values.Press.valueByOffset;
				currentCommandValues.temp = adc_values.Temp.valueByOffset;
				if (currentCommandValues.press < settings.LowPress) {
					if (timeValues.servoStayEndTime == 0){
						timeValues.servoStayEndTime = daemonGetTimeValuesRunTime() + i2c_servo_values.i2c_servo_stay_open;
					}
					if (timeValues.servoStayEndTime < daemonGetTimeValuesRunTime()){
						currentCommandValues.mode=MODE_PRESSURELESS;
						timeValues.servoStayEndTime = 0;
						
						if (settings.shieldVersion < 3){
							setServoOpen(0, 1, 0, &daemon_values);
						} else {
							solenoidSetOpen(false);
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
						solenoidSetOpen(false);
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
		heaterOff();
		heaterStopThreadLedReader();
	} else if (runningMode.test_serial){
		if (settings.shieldVersion < 4){
			printf("there is no ATMega644 on shieldVersion %d\n", settings.shieldVersion);
		} else {
			printf("test serial / Atmel communication\n");
			SPIAtmelReset();
			atmelSetMaintenance(true);
			
			uint8_t stepNr = 0;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){//count from 0 to 255
				printf("count from 0 to 255\n");
				uint8_t value=0;
				for(value=10;value<255 && state.running;value++){
					//delay(200);
					if (value % 10 == 0){
						delay(1000);
					}
				}
				if (!state.running){
					exit(0);
				}
				delay(250);
			}
			
			stepNr++;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){//ShowPercent(100)
				displayShowPercent(100);
				delay(2000);
			}
			
			stepNr++;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){//show 123456789
				//Show Text
				printf("Show Numbers\n");
				displayShowText("1234567890");
				delay(6000);
			}
			
			stepNr++;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){// Show Smilly
				printf("Show Smilly\n");
				//Show Smilly
										//00##############
				uint16_t picture[9] = {	0b0000000111110000,
										0b0000001000001000,
										0b0000010000000100,
										0b0000010100010100,
										0b0000010000000100,
										0b0000010100010100,
										0b0000010011100100,
										0b0000001000001000,
										0b0000000111110000};
				displayShowPicture(&picture[0]);
				delay(2000);
			}
			
			stepNr++;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){// Show chess
				printf("Show chess\n");
				uint16_t picture2[9]  = {	0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010};
				displayShowPicture(&picture2[0]);
				delay(2000);
			}
			
			stepNr++;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){// show percent 0-140
				printf("Show percent progress\n");
				// show percent 0-140
				uint8_t i=0;
				for (; i<140 && state.running; i=i+10) {
					displayShowPercent(i);
				}
				delay(2000);
				printf("clear\n");
				displayClear();
			}
			
			stepNr++;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){// "Hallo Welt! Ich komme vom Raspi!"
				printf("Show text\n");
				//Show Text
				displayShowText("Hallo Welt! Ich komme vom Raspi!");
				//delay(18400);
				delay(14000);
				printf("clear\n");
				displayClear();
			}
			
			stepNr++;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){//open solenoid valve during 5 sec
				printf("atmelSetSolenoidOpen, for 5 seconds\n");
				uint8_t count=0;
				solenoidSetOpen(true);
				while(state.running && count<10){
					delay(500);
					count++;
				}
				solenoidSetOpen(false);
			}
			
			stepNr++;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){//turn on heater during 10 sec
				printf("enable heater, for 10 seconds\n");
				uint8_t count=0;
				heaterOn();
				while(state.running && count<20){
					delay(500);
					count++;
				}
				heaterOff();
			}
			
			stepNr++;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){//turn on motor in full speed during 10 sec
				printf("enable motor in full speed, for 10 seconds\n");
				uint8_t count=0;
				motorSetSpeedRPM(200);
				while(state.running && count<20){
					delay(500);
					count++;
				}
				motorSetSpeedRPM(0);
			}
			
			stepNr++;
			if (settings.test_servo_min<=stepNr && settings.test_servo_max>=stepNr){//read status until programm end Ctrl+C
				printf("read status until programm end Ctrl+C\n");
				while(state.running){
					delay(500);
				}
			}
			atmelSetMaintenance(false);
		}
	}
	if (settings.shieldVersion < 4){
		SegmentDisplaySimple('S', &state, &i2c_config);
	} else {
		//atmel show bye
		uint16_t picture_bye[9] = {
			0b00000000000000,
			0b01110010101110,
			0b01001010101000,
			0b01001010101000,
			0b01110001001110,
			0b01001001001000,
			0b01001001001000,
			0b01110001001110,
			0b00000000000000
		};
		displayShowPicture(&picture_bye[0]);
		virtualAtmelStopSPI();
	}
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

/** @brief shows the different modes that can be chosen
 */
void printUsage(){
	printf("Usage: ecdaemon [OPTIONS]\r\n");
	printf("programm options:\r\n");
	printf("  -t7, --test-7seg              New calibration mode\r\n");
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
	printf("  -ts, --test-servo [from [to]] test servo open value, if from/to is omitted, values are from: %d, to: %d\r\n", settings.test_servo_min, settings.test_servo_max);
	printf("  -th, --test-heat-led          read the heater LED informations\r\n");
	printf("  -tm, --test-motor [from [to]] test motor speed value, if from/to is omitted, values are from: %d, to: %d\r\n", 200, 2000);
	printf("  -tb, --test-buttons           test button press\r\n");
	printf("  -ta, --test-adc [updateRate [offset [fullScale]]\r\n");
	printf("                                test adc, set update rate setting to submitted value, use %d if ommited.\r\n", settings.test_ADC_update_rate);
	printf("                                offset/fullScale calibration values i or s, if omittet no calibration.\r\n");
	printf("  -tp, --test-heating-power     test the power/efficiency of heating\r\n");
	printf("  -tr, --test-heating-press     test the power/efficiency of heating on pressup\r\n");
	printf("  -te, --test-serial            test the serial interface to ATMEL/Arduino\r\n");
	printf("  -?,  --help                   show this help\r\n");
}

/** @brief program different mode according to what is chosen during the launch
 *  @param argc : mode number selected
 *  @param argv : mode which has been selected
*/
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
			settings.test_servo_min = 200;// même variable que au dessus, et meme fonction
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
		} else if(strcmp(argv[i], "--test-serial") == 0 || strcmp(argv[i], "-te") == 0){
			runningMode.test_serial = true;
			runningMode.normalMode = false;
			settings.test_servo_min = 1;
			settings.test_servo_max = 10;
			if (argc>i+1 && argv[i+1][0] != '-'){
				++i;
				settings.test_servo_min = StringConvertToNumber(argv[i]);
				if (argc>i+1 && argv[i+1][0] != '-'){
					++i;
					settings.test_servo_max = StringConvertToNumber(argv[i]);
				}
			}
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

/** @brief initialization signals
 */
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

/** @brief clear HourCounter
*/
void clearHourCounter(){
	hourCounter.identifier[0] = 'H';
	hourCounter.identifier[1] = 'C';
	hourCounter.identifier[2] = 'V';
	hourCounter.identifier[3] = HOUR_COUNTER_VERSION;
	hourCounter.daemon = 0;
	heaterSetHourCounter(0);
	motorSetHourCounter(0);
}

/** @brief creation of file
 */
void initOutputFile(void){
	if (daemonGetSettingsDebug_enabled()){printf("iniOutputFile\n");}
	FILE *fp;
	fp = fopen(settings.statusFile, "w");
	fputs("{\"T0\":0,\"P0\":0,\"M0RPM\":0,\"M0ON\":0,\"M0OFF\":0,\"W0\":0,\"STIME\":0,\"SMODE\":0,\"SID\":-2}", fp);
	fclose(fp);
	
	char* headerLine = "Time, Temp, Press, MotorRpm, Weight, setTemp, setPress, setMotorRpm, setWeight, setMode, Mode, heaterHasPower, isOn, noPan, lidClosed, lidLocked, pusherLocked\n";
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

/** @brief reset all value
 */
void resetValues(){
	beeperSetIsBuzzing(true); //to be sure first send a off to buzzer
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
	if (settings.shieldVersion < 4){
		writeI2CPin(i2c_config.i2c_servo, i2c_servo_values.i2c_servo_closed); // send the value by the I2C
	}
	
	heaterSetStatusLedValuesI(0,0);
	heaterSetStatusLedValuesI(1,0);
	heaterSetStatusLedValuesI(2,0);
	heaterSetStatusLedValuesI(3,0);
	heaterSetStatusLedValuesI(4,0);
	heaterSetStatusLedValuesI(5,0);
}

/** @brief check if new information  were send by web
*/
bool checkForInput(){
	bool valueChanged = false;
	if (settings.useMiddleware){
		uint8_t recivedDataType = 0;
		if (state.sockfd < 0){
			if (daemonGetTimeValuesRunTime() - timeValues.middlewareConnectTime > 5){
				//only try to connect to middleware every 5 sec.
				timeValues.middlewareConnectTime = daemonGetTimeValuesRunTime();
				state.sockfd = connectToMiddleware(settings.middlewareHostname, settings.middlewarePortno);
			}
		}
		if (state.sockfd>=0 && isSocketReady(state.sockfd)){
			//if (reciveFromSock(state.sockfd, state.middlewareBuffer) ){
			if (recivePackageFromSock(state.sockfd, state.middlewareBuffer, &recivedDataType)){
				valueChanged = true;
				if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled()){printf("recived Value(with type %d): %s\n", recivedDataType, state.middlewareBuffer);}
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
				if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled()){printf("commandFile to old, is:%ld, last:%ld\n", changeTime, timeValues.lastFileChangeTime);}
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

/** @ write all output file to communicate with web
*/
void doOutput(){
	prepareState(state.TotalUpdate);// state.TotalUpdate contain all value
	if (state.dataChanged || timeValues.lastStatusTime+MIN_STATUS_INTERVAL<daemonGetTimeValuesRunTime()){// || state.timeChanged){
		if (daemonGetSettingsDebug_enabled()){printf("dataChanged=%d\n",state.dataChanged);}
		if (settings.useMiddleware && state.sockfd>=0){
			if (!sendToSock(state.sockfd, state.TotalUpdate, dataType)){
				fprintf(stderr, "ERROR writing to socket\n");
				state.sockfd = -13;
			}
		}
		if (settings.useFile) {
			writeStatus(state.TotalUpdate);
		}
		if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled()){printf("%s\n",state.TotalUpdate);}
		state.dataChanged = false;
		state.timeChanged = false;
		timeValues.lastStatusTime=daemonGetTimeValuesRunTime();
	}
	writeLog();
}

/** @brief controle temp,press,motor,valve,beep, display
*/
void ProcessCommand(void){
	if (newCommandValues.stepId != currentCommandValues.stepId){
		if (newCommandValues.stepId < currentCommandValues.stepId){
			//-1 stop/not aus
			if(newCommandValues.stepId == -1){
				motorSetI2cValuesMotorRpm(motorGetI2cValuesSpeedMin()); //cheating so it will realy stop motor
				motorSetCommandRPM(0);
				heaterOff();
			}
			//TODO is reset/stop? or is someone try to begin new recipe before old is ended?
			if(newCommandValues.stepId==0){
				resetValues();
				newCommandValues.stepId=0;
			}
		}
		oldCommandValues.mode = daemonGetCurrentCommandValuesMode();//save old mode
		currentCommandValues.mode = newCommandValues.mode;//save new mode
		currentCommandValues.stepId = newCommandValues.stepId;//save new stepId
		timeValues.stepStartTime = daemonGetTimeValuesRunTime();//save time
		state.dataChanged = true;// save the fact that the date have changed
		timeValues.stepEndTime = timeValues.stepStartTime + newCommandValues.time;
		
		state.scaleReady = false;
		if (state.Delay == settings.ShortDelay || (oldCommandValues.mode == MODE_SCALE && daemonGetCurrentCommandValuesMode() == MODE_SCALE)) {
			if(daemonGetSettingsDebug3_enabled()){printf("new and old step is scale mode, reset values\n");}
			state.referenceForce=0;
			state.Delay=settings.LongDelay;
			currentCommandValues.weight = 0.0;
		}
		
		if (daemonGetSettingsDebug_enabled()){printf("stepId changed, new mode is: %d\n", daemonGetCurrentCommandValuesMode());}
		if (daemonGetSettingsDebug_enabled() || runningMode.simulationMode || daemonGetSettingsDebug3_enabled()){printf("ProcessCommand: T0: %.0f, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %.0f, STIME: %d, SMODE: %d, SID: %d\n", newCommandValues.temp, newCommandValues.press, motorGetNewCommandValuesRpm(), motorGetNewCommandValuesOn(), motorGetNewCommandValuesOff(), newCommandValues.weight, newCommandValues.time, newCommandValues.mode, newCommandValues.stepId);}
		
		OptionControl();//bep and blink 7seg
		if (settings.shieldVersion >= 4){// if display, show image per mode
			if (daemonGetCurrentCommandValuesMode()==MODE_STANDBY) {
				/*
										//00##############
				uint16_t picture[9] = {	0b0000000111110000,
										0b0000001000001000,
										0b0000010000000100,
										0b0000010100010100,
										0b0000010000000100,
										0b0000010100010100,
										0b0000010011100100,
										0b0000001000001000,
										0b0000000111110000};
				displayShowPicture(&picture[0]);
				*/
				displayClear();
			} else if (daemonGetCurrentCommandValuesMode()==MODE_CUT) {
										//00##############
				uint16_t picture[9] = {	0b0000000000000000,
										0b0000000000000000,
										0b0001111111111100,
										0b0000100000111110,
										0b0000011111111100,
										0b0000000000000000,
										0b0000000000000000,
										0b0000000000000000,
										0b0000000000000000};
				displayShowPicture(&picture[0]);
			} else if (daemonGetCurrentCommandValuesMode()==MODE_SCALE) {
										//00##############
				uint16_t picture[9] = { 0b0000000000000000,
										0b0000000000000000,
										0b0001110000011100,
										0b0000100000001000,
										0b0000011000110000,
										0b0000000111000000,
										0b0000000010000000,
										0b0000001111100000,
										0b0000000000000000};
				displayShowPicture(&picture[0]);
			} else if (daemonGetCurrentCommandValuesMode()==MODE_HEATUP) {
										//00##############
				uint16_t picture[9] = { 0b0000000000000000,
										0b0000000000000000,
										0b0000000010000000,
										0b0000100111001000,
										0b0000110111011000,
										0b0001110111011100,
										0b0001110111011100,
										0b0000100010001000,
										0b0000000000000000};
				displayShowPicture(&picture[0]);
			} else if (daemonGetCurrentCommandValuesMode()==MODE_COOK) {
										//00##############
				uint16_t picture[9] = { 0b0000000000000000,
										0b0000001010000000,
										0b0000010001000000,
										0b0000001010000000,
										0b0000010001000000,
										0b0000000000000000,
										0b0000111111111110,
										0b0000011111100000,
										0b0000000000000000};
				displayShowPicture(&picture[0]);
			} else if (daemonGetCurrentCommandValuesMode()==MODE_COOLDOWN) {
										//00##############
				uint16_t picture[9] = { 0b0000000000000000,
										0b0000000000000000,
										0b0000000000000000,
										0b0000000000000000,
										0b0000000000000000,
										0b0000000000000000,
										0b0000111111111110,
										0b0000011111100000,
										0b0000000000000000};
				displayShowPicture(&picture[0]);
			} else if (daemonGetCurrentCommandValuesMode()==MODE_PRESSUP) {
										//00##############
				uint16_t picture[9] = { 0b0000000010000000,
										0b0000000111000000,
										0b0000000101000000,
										0b0000000101000000,
										0b0000000111000000,
										0b0000000101000000,
										0b0000001111100000,
										0b0000001111100000,
										0b0000000000000000};
				displayShowPicture(&picture[0]);
			} else if (daemonGetCurrentCommandValuesMode()==MODE_PRESSHOLD) {
										//00##############
				uint16_t picture[9] = { 0b0000000000000000,
										0b0000100010001000,
										0b0001110001010000,
										0b0001010010001000,
										0b0001110001010000,
										0b0001010000000000,
										0b0001110111111110,
										0b0001110011111000,
										0b0000000000000000};
				displayShowPicture(&picture[0]);
			} else if (daemonGetCurrentCommandValuesMode()==MODE_PRESSDOWN) {
										//00##############
				uint16_t picture[9] = { 0b0000000000000000,
										0b0000000000000000,
										0b0000000010000000,
										0b0000000111000000,
										0b0000000101000000,
										0b0000000101000000,
										0b0000001111100000,
										0b0000001111100000,
										0b0000000000000000};
				displayShowPicture(&picture[0]);
			} else if (daemonGetCurrentCommandValuesMode()==MODE_PRESSVENT) {
										//00##############
				uint16_t picture[9] = { 0b001000000000100,
										0b000100000001000,
										0b000010010010000,
										0b000100111001000,
										0b000010101010000,
										0b000000101000000,
										0b000001111100000,
										0b000001111100000,
										0b000000000000000};
				displayShowPicture(&picture[0]);
			}
			/* TODO
			MODE_HOT
			MODE_PRESSURIZED
			MODE_COLD
			MODE_PRESSURELESS
			MODE_WEIGHT_REACHED
			MODE_COOK_TIMEEND
			MODE_RECIPE_END
			*/
		}	
		if (state.alwaysReadMode){
			speakerSpeak(state.actionText);//speak the action
		}
	}
	if (settings.shieldVersion >= 4){
		
	}
	TempControl();//temperature
	PressControl();//pression
	if (daemonGetSettingsDebug3_enabled()){
		printf("\n");
	}
	motorControl();//motor
	ValveControl();//valve
	
	if (daemonGetSettingsDebug3_enabled()){
		printf("after ValveControl\n");
	}
	ScaleFunction();//weight
	if (daemonGetCurrentCommandValuesMode()==MODE_SCALE || daemonGetCurrentCommandValuesMode()==MODE_WEIGHT_REACHED){
		if (state.dataChanged){
			if (settings.shieldVersion >= 4){
				displayShowPercent(state.weightPercent);// show percent on display
			}
		}
	}
	
	if (daemonGetSettingsTimeValuesStepEndTime() > daemonGetTimeValuesRunTime()) {
		timeValues.remainTime=daemonGetSettingsTimeValuesStepEndTime()-daemonGetTimeValuesRunTime();
	} else if (daemonGetCurrentCommandValuesMode()<MIN_STATUS_MODE) {
		timeValues.remainTime=0;
		if (daemonGetCurrentCommandValuesMode()==MODE_COOK || daemonGetCurrentCommandValuesMode() == MODE_PRESSHOLD){
			currentCommandValues.mode=MODE_COOK_TIMEEND;
			state.dataChanged=true;
			if (beeperGetSettingsBeepStepEnd()>0){
				beeperSetBeepEndTime(daemonGetTimeValuesRunTime()+beeperGetSettingsBeepStepEnd());
			}
		} else if (daemonGetCurrentCommandValuesMode() != MODE_STANDBY){
			currentCommandValues.mode=MODE_STANDBY;
			state.dataChanged=true;
			if (beeperGetSettingsBeepStepEnd()>0){
				beeperSetBeepEndTime(daemonGetTimeValuesRunTime()+beeperGetSettingsBeepStepEnd());
			}
		}
	}
	SegmentDisplay();
	beeperBeepEndStep();
}

/******************* processing functions **********************/
/** @brief beep, stop beep or blinck 7segment in according to currentCommandValues.mode
*/
void OptionControl(){
    if (daemonGetCurrentCommandValuesMode()>=MODE_OPTIONS_BEGIN){
		if (daemonGetCurrentCommandValuesMode()==MODE_OPTION_REMEMBER_BEEP_ON){
			beeperSetDoRememberBeep(true);
		} else if (daemonGetCurrentCommandValuesMode()==MODE_OPTION_REMEMBER_BEEP_OFF){
			beeperSetDoRememberBeep(false);
		} else if (daemonGetCurrentCommandValuesMode()==MODE_OPTION_7SEGMENT_BLINK){
			if(settings.shieldVersion < 4){
				SegmentDisplaySimple(' ', &state, &i2c_config);
				blink7Segment(&i2c_config);
				blink7Segment(&i2c_config);
				blink7Segment(&i2c_config);
				blink7Segment(&i2c_config);
			}
		}
		currentCommandValues.mode=MODE_STANDBY;
		state.dataChanged=true;
    }
}


//TODO: state erweitern um neue sensor werte
//TODO: beim start atmel, prüfen ob motor an richtiger stelle(und läuft nicht), ansonnsten bis dahin drehen.

#define SB_CommandOK		0
#define SB_LidClosed		1
#define SB_LidLocked		2
#define SB_PusherLocked		3
#define SB_isIHOn			4
#define SB_IHFanOn			5
#define SB_MotorStoped		6
#define SB_CommandError		7
/** @brief fonction is not finish
*/


/** @brief Controle temperature
*/
void TempControl(){
	if (daemonGetCurrentCommandValuesMode()<MIN_COOK_MODE || daemonGetCurrentCommandValuesMode()>MAX_COOK_MODE) heaterOff();
	if (daemonGetCurrentCommandValuesMode()>=MIN_TEMP_MODE && daemonGetCurrentCommandValuesMode()<=MAX_TEMP_MODE) {
		oldCommandValues.temp = currentCommandValues.temp;
		currentCommandValues.temp = adc_values.Temp.valueByOffset;
		if (daemonGetSettingsDebug_enabled() || runningMode.calibration || daemonGetSettingsDebug3_enabled()){printf("Temp %d dig %.0f °C | ", adc_values.Temp.adc_value, adc_values.Temp.valueByOffset);}
		if (runningMode.measure_noise) {printf("NoiseTemp %d | ", adc_noise.DeltaTemp);}
		
		if (oldCommandValues.temp != currentCommandValues.temp){
			state.dataChanged = true;
		}
		
		int deltaT=newCommandValues.temp-currentCommandValues.temp;
		if (daemonGetCurrentCommandValuesMode()==MODE_HEATUP || daemonGetCurrentCommandValuesMode()==MODE_COOK) {
			if (deltaT<=0) {
				heaterOff();
				if (daemonGetCurrentCommandValuesMode()==MODE_HEATUP) {//heatup function
					timeValues.stepEndTime=daemonGetTimeValuesRunTime()-2;
					currentCommandValues.mode=MODE_HOT; //we are hot
					state.dataChanged=true;
				}
			}
			else if (deltaT>=adc_config.TempHystereis) {heaterOn();}
		} else {
			heaterOff();
		}
		if (daemonGetCurrentCommandValuesMode()==MODE_HEATUP && heaterGetPowerStatus()) { //heatup
			timeValues.stepEndTime=daemonGetTimeValuesRunTime()+1;
		} else if (daemonGetCurrentCommandValuesMode()==MODE_COOLDOWN && deltaT>=0) { //cooldown function
			timeValues.stepEndTime=daemonGetTimeValuesRunTime()-2;
			currentCommandValues.mode=MODE_COLD;
			state.dataChanged=true;
		} else if (daemonGetCurrentCommandValuesMode()==MODE_COOLDOWN) {
			timeValues.stepEndTime=daemonGetTimeValuesRunTime()+1;
		}
	} else {
		oldCommandValues.temp = currentCommandValues.temp;
		currentCommandValues.temp = adc_values.Temp.valueByOffset;
		if (daemonGetSettingsDebug_enabled() || runningMode.calibration || daemonGetSettingsDebug3_enabled()){printf("Temp %d dig %.0f °C | ", adc_values.Temp.adc_value, adc_values.Temp.valueByOffset);}
		if (runningMode.measure_noise) {printf("NoiseTemp %d | ", adc_noise.DeltaTemp);}
		if (oldCommandValues.temp != currentCommandValues.temp){
			state.dataChanged = true;
		}
	}
}

/** @brief Controle pression
*/
void PressControl(){
	if (daemonGetCurrentCommandValuesMode()<MIN_COOK_MODE || daemonGetCurrentCommandValuesMode()>MAX_COOK_MODE) heaterOff();
	if (daemonGetCurrentCommandValuesMode()>=MIN_PRESS_MODE && daemonGetCurrentCommandValuesMode()<=MAX_PRESS_MODE) {
		oldCommandValues.press = currentCommandValues.press;
		currentCommandValues.press = adc_values.Press.valueByOffset;
		if (daemonGetSettingsDebug_enabled() || runningMode.calibration || daemonGetSettingsDebug3_enabled()){printf("Press %d digits %.0f kPa | ", adc_values.Press.adc_value, adc_values.Press.valueByOffset);}
		if (runningMode.measure_noise){printf("NoisePress %d |", adc_noise.DeltaPress);}
		
		if (oldCommandValues.press != currentCommandValues.press){
			state.dataChanged = true;
		}
		
		int deltaP=newCommandValues.press-currentCommandValues.press;
		if (daemonGetCurrentCommandValuesMode()==MODE_PRESSUP || daemonGetCurrentCommandValuesMode()==MODE_PRESSHOLD) {
			if (deltaP<=0) {
				heaterOff();
				if (daemonGetCurrentCommandValuesMode()==MODE_PRESSUP) {//pressup function
					timeValues.stepEndTime=daemonGetTimeValuesRunTime()-2;
					currentCommandValues.mode=MODE_PRESSURIZED; //we are pressurized
					state.dataChanged=true;
				}
			}
			else if (deltaP>=adc_config.PressHystereis) {heaterOn();}
		} else {
			heaterOff();
		}
		if (daemonGetCurrentCommandValuesMode()==MODE_PRESSUP && heaterGetPowerStatus()) { //pressure up
			timeValues.stepEndTime=daemonGetTimeValuesRunTime()+1;;
		} else if (daemonGetCurrentCommandValuesMode()==MODE_PRESSDOWN && deltaP>=0) { //pressure down function
			timeValues.stepEndTime=daemonGetTimeValuesRunTime()-2;
			currentCommandValues.mode=MODE_PRESSURELESS;
			state.dataChanged=true;
		} else if (daemonGetCurrentCommandValuesMode()==MODE_PRESSDOWN) {
			timeValues.stepEndTime=daemonGetTimeValuesRunTime()+1;
		}
	} else {
		oldCommandValues.press = currentCommandValues.press;
		currentCommandValues.press = adc_values.Press.valueByOffset;
		if (daemonGetSettingsDebug_enabled() || runningMode.calibration || daemonGetSettingsDebug3_enabled()){printf("Press %d digits %.0f kPa | ", adc_values.Press.adc_value, adc_values.Press.valueByOffset);}
		if (runningMode.measure_noise){printf("NoisePress %d |", adc_noise.DeltaPress);}
		
		if (oldCommandValues.press != currentCommandValues.press){
			state.dataChanged = true;
		}
	}
}


//void ValveControl(struct State state, struct Settings settings, struct Command_Values currentCommandValues, struct Time_Values timeValues){
/** @brief Controle Valve
*/
void ValveControl(){
	if (daemonGetCurrentCommandValuesMode()==MODE_PRESSVENT) {
		heaterOff();
		if (settings.shieldVersion < 3){
			setServoOpen(100, 10, 1000, &daemon_values);
		} else {
			solenoidSetOpen(true);
		}
		timeValues.stepEndTime=daemonGetTimeValuesRunTime()+1;
		if (currentCommandValues.press < settings.LowPress) {
			if (timeValues.servoStayEndTime == 0){
				timeValues.servoStayEndTime = daemonGetTimeValuesRunTime() + i2c_servo_values.i2c_servo_stay_open;
			}
			if (timeValues.servoStayEndTime < daemonGetTimeValuesRunTime()){
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
			solenoidSetOpen(false);
		}
	}
}

//void ScaleFunction(struct State state, struct Settings settings, struct Command_Values oldCommandValues, struct Command_Values currentCommandValues, struct Command_Values newCommandValues, struct Time_Values timeValues){
/** @brief control weight
*/
void ScaleFunction(){
	if (daemonGetCurrentCommandValuesMode()==MODE_SCALE || daemonGetCurrentCommandValuesMode()==MODE_WEIGHT_REACHED){
		timeValues.stepEndTime=daemonGetTimeValuesRunTime()+2;
		oldCommandValues.weight = currentCommandValues.weight;
		double sumOfForces = adc_values.Weight.value;
		if (daemonGetSettingsDebug_enabled() || runningMode.calibration){printf("Weight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", adc_values.Weight.adc_value, adc_values.Weight.value, adc_values.Weight.valueByOffset, adc_values.LoadCellFrontLeft.adc_value, adc_values.LoadCellFrontRight.adc_value, adc_values.LoadCellBackLeft.adc_value, adc_values.LoadCellBackRight.adc_value);}			
		if (runningMode.measure_noise){
			printf("NoiseWeightFL %d | ", adc_noise.DeltaWeight1);
			printf("NoiseWeightFR %d | ", adc_noise.DeltaWeight2);
			printf("NoiseWeightBL %d | ", adc_noise.DeltaWeight3);
			printf("NoiseWeightBR %d\n", adc_noise.DeltaWeight4);
		}
		if (daemonGetSettingsDebug_enabled()){printf("ScaleFunction\n");}
		
		if (!state.scaleReady) { //we are not ready for weighting
			state.referenceForce=sumOfForces;
			oldCommandValues.weight=sumOfForces;
			state.scaleReady=true;
			state.weightPercent = 0;
			if (state.Delay == settings.ShortDelay){
				state.Delay=settings.LongDelay;
			}
			if (daemonGetSettingsDebug_enabled()){printf("\tscaleReady\n");}
		} else { //we have a reference and are ready
			state.Delay=settings.ShortDelay;
			currentCommandValues.weight=(sumOfForces-state.referenceForce);
			
			if (oldCommandValues.weight != currentCommandValues.weight){
				state.dataChanged = true;
				double percent = currentCommandValues.weight / newCommandValues.weight;
				if (percent>255){
					state.weightPercent = 255;
				} else {
					state.weightPercent = (uint8_t) percent;
				}
			}
			if(weightSlowly==false && currentCommandValues.weight<((newCommandValues.weight*settings.weightReachedMultiplier*70)/100)){
			weightSlowly=false;
			}
			if(weightSlowly==false && currentCommandValues.weight>=((newCommandValues.weight*settings.weightReachedMultiplier*70)/100)){
				speakerSpeakLanguage("slowly");
				weightSlowly=true;
			}
			if (currentCommandValues.weight>=(newCommandValues.weight*settings.weightReachedMultiplier)) {//If we have reached the required mass
				if (daemonGetCurrentCommandValuesMode() != MODE_WEIGHT_REACHED){
					if(settings.BeepWeightReached > 0){
						speakerSpeakLanguage("stop");
						
						//beeperSetBeepEndTime(daemonGetTimeValuesRunTime()+settings.BeepWeightReached);
					}
					if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled()){printf("\tweight reached!\n");}
				} else {
					if (daemonGetSettingsDebug_enabled()){printf("\tweight reached...\n");}
				}
				if (daemonGetCurrentCommandValuesMode() != MODE_WEIGHT_REACHED){
					currentCommandValues.mode=MODE_WEIGHT_REACHED;
					state.dataChanged=true;
				}
				timeValues.remainTime=0;
			}
			if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled()){printf("\t\tweight: %f / old: %f\n", currentCommandValues.weight, oldCommandValues.weight);}
		}
	} else if (state.Delay == settings.ShortDelay || state.scaleReady){
		state.scaleReady=false;
		state.referenceForce=0;
		state.Delay=settings.LongDelay;
		currentCommandValues.weight = 0.0;
	}
}

//void SegmentDisplay(struct State state, struct Settings settings, struct Running_Mode runningMode, struct I2C_Config i2c_config){
/** @brief show the new status (P/O/H) or blink it
*/
void SegmentDisplay(){
	if (settings.shieldVersion >= 4){
		return;
	}
	char curSegmentDisplay = ' ';
	if (currentCommandValues.press>=settings.LowPress){// if pression
		curSegmentDisplay = 'P';
	} else if (currentCommandValues.temp>=settings.LowTemp){//if hot
		curSegmentDisplay = 'H';
	} else {
		curSegmentDisplay = '0';
	}
	if (curSegmentDisplay != state.oldSegmentDisplay){
		if(!adc_config.restarting_adc){
			if (daemonGetSettingsDebug_enabled() || runningMode.simulationMode){printf("SegmentDisplay: '%c'\n", curSegmentDisplay);}
			if (!runningMode.simulationMode || runningMode.simulationModeShow7Segment){
				SegmentDisplaySimple(curSegmentDisplay, &state, &i2c_config);// show new statu (H/P/O)
				//SegmentDisplayOptimized(curSegmentDisplay, &state, &i2c_config);
			} else {
				state.oldSegmentDisplay = curSegmentDisplay;
			}
		}
	}
	if (!runningMode.simulationMode || runningMode.simulationModeShow7Segment){
		if (timeValues.lastBlinkTime != daemonGetTimeValuesRunTime()){//if no change, blink 7seg
			timeValues.lastBlinkTime = daemonGetTimeValuesRunTime();
			//togglePin(i2c_7seg_period);
			if (state.blinkState){
				displaySetPeriod(false);
				state.blinkState = false;
				if (daemonGetSettingsDebug_enabled() || (runningMode.simulationMode && !runningMode.simulationModeShow7Segment)){printf("SegmentDisplay: blink off\n");}
			} else {
				state.blinkState = true;
				displaySetPeriod(true);
				if (daemonGetSettingsDebug_enabled() || (runningMode.simulationMode && !runningMode.simulationModeShow7Segment)){printf("SegmentDisplay: blink on\n");}
			}
		}
	}
}


/*******************PI File read/write Code**********************/
//format: {"T0":000,"P0":000,"M0RPM":0000,"M0ON":000,"M0OFF":000,"W0":0000,"STIME":000000,"SMODE":00,"SID":000}
/** @brief put all value in char TotalUpdate with the good format
 *  @param * TotalUpdate : char witch will have
 */
void prepareState(char* TotalUpdate){
	StringClean(TotalUpdate, 512);
	uint32_t press; //remove negative values for output;
	if (currentCommandValues.press>0){
		press = currentCommandValues.press;
	} else {
		press = 0;
	}
	sprintf(TotalUpdate, "{\"T0\":%.2f,\"P0\":%d,\"M0RPM\":%d,\"M0ON\":%d,\"M0OFF\":%d,\"W0\":%.0f,\"STIME\":%d,\"SMODE\":%d,\"SID\":%d,\"heaterHasPower\":%d,\"isOn\":%d,\"noPan\":%d,\"lidClosed\":%d,\"lidLocked\":%d,\"pusherLocked\":%d}",	currentCommandValues.temp, 						press, motorGetCurrentCommandValuesRpm(), motorGetCurrentCommandValuesOn(), motorGetCurrentCommandValuesOff(), currentCommandValues.weight, currentCommandValues.time, daemonGetCurrentCommandValuesMode(), currentCommandValues.stepId, heaterGetStatusHasPower(), heaterGetStatusIsOn(), heaterGetStatusNoPanError(), state.lidClosed, state.lidLocked, state.pusherLocked);
	if (daemonGetSettingsDebug_enabled()){printf("prepareState: T0: %f, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %f, STIME: %d, SMODE: %d, SID: %d, heaterHasPower: %d, isOn: %d, noPan: %d, lidClosed:%d, lidLocked:%d, pusherLocked:%d\n", 		currentCommandValues.temp, currentCommandValues.press, motorGetCurrentCommandValuesRpm(), motorGetCurrentCommandValuesOn(), motorGetCurrentCommandValuesOff(), currentCommandValues.weight, currentCommandValues.time, daemonGetCurrentCommandValuesMode(), currentCommandValues.stepId, heaterGetStatusHasPower(), heaterGetStatusIsOn(), heaterGetStatusNoPanError(), state.lidClosed, state.lidLocked, state.pusherLocked);}
}

/** @brief write data in settings.statusFile
 *  @param * data
*/
void writeStatus(char* data){
	if (daemonGetSettingsDebug_enabled()){printf("WriteStatus\n");}
	
	FILE *fp;
	fp = fopen(settings.statusFile, "w");
	fputs(data, fp);
	fclose(fp);
	if (daemonGetSettingsDebug3_enabled()){printf("%s\n", data);}
	
	if (daemonGetSettingsDebug_enabled()){printf("WriteStatus: after write\n");}
}

/** @ brief write on file settings.logFile and settings.hourCounterFile
*/
void writeLog(){
	if (daemonGetSettingsDebug_enabled()){printf("writeLog\n");}
	timeValues.nowTime = time(NULL);
	timeValues.localTime=localtime(&timeValues.nowTime);
	
	if (daemonGetTimeValuesRunTime()>=timeValues.lastLogSaveTime+settings.logSaveInterval){
		char tempString[20];
		StringClean(tempString, 20);
		strftime(tempString, 20,"%F %T",timeValues.localTime);
		char logline[200];
		sprintf(logline, "%s, %.1f, %i, %i, %.1f, %.1f, %i, %i, %.1f, %i, %i, %i, %i, %i, %i, %i, %i\n",tempString, currentCommandValues.temp, currentCommandValues.press, motorGetI2cValuesMotorRpm(), currentCommandValues.weight, newCommandValues.temp, newCommandValues.press, motorGetNewCommandValuesRpm(), newCommandValues.weight, newCommandValues.mode, daemonGetCurrentCommandValuesMode(), heaterGetStatusHasPower(), heaterGetStatusIsOn(), heaterGetStatusNoPanError(), state.lidClosed, state.lidLocked, state.pusherLocked);
		
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
			hourCounter.daemon = hourCounter.daemon + (daemonGetTimeValuesRunTime() - timeValues.lastLogSaveTime);
			heaterUpdateTime();
			motorUpdateTime();
			
			FILE *hourCounterFilePointer = fopen(settings.hourCounterFile, "w");
			fwrite(&hourCounter, sizeof(hourCounter), 1, hourCounterFilePointer);
			fprintf(hourCounterFilePointer, "\nhours on: %.2f\nmotor hours: %.2f\nheater hours: %.2f\n", hourCounter.daemon/SECONDS_PER_HOUR, motorGetHourCounter()/SECONDS_PER_HOUR, heaterGetHourCounter()/SECONDS_PER_HOUR);
			//fprintf(hourCounterFilePointer, "\nhours on: %d\nmotor hours: %d\nheater hours: %d\n", hourCounter.daemon, hourCounter.motor, hourCounter.heater);
			fclose(hourCounterFilePointer);
		}
		timeValues.lastLogSaveTime=daemonGetTimeValuesRunTime();
	}
	if (daemonGetSettingsDebug_enabled()){printf("writeLog: after write\n");}
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
	state.actionText[0] = 0;
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
			
			if (daemonGetSettingsDebug_enabled()){printf("found %s with value %d\r\n", state.names[ptr], state.value[ptr]);}
			StringClean(tempName, 10);
			StringClean(tempValue, 10);
			++ptr;
		}
		c = input[inputPos];
		++inputPos;
	}
}

bool ReadFile(){
	if (daemonGetSettingsDebug_enabled()){printf("ReadFile\n");}
	FILE *fp;
	char tempName[10];
	char tempValue[10];
	uint8_t i = 0;
	uint8_t ptr = 0;
	char c;
	
	StringClean(tempName, 10);
	StringClean(tempValue, 10);
	state.actionText[0] = 0;
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
			
			if (daemonGetSettingsDebug_enabled()){printf("found %s with value %d\r\n", state.names[ptr], state.value[ptr]);}
			
			StringClean(tempName, 10);
			StringClean(tempValue, 10);
			ptr++;
		}
	}
	fclose(fp);
	
	return true;
}

/** @brief change the newCommandeValue with state
*/
void evaluateInput(){
	//TODO only set "setXXX" values if stepId changed, other wise ignore "new/changed values".
	int i;
	for(i = 0; i < 15; ++i){
		//if (daemonGetSettingsDebug_enabled()){printf("check %s with value %d\r\n", state.names[i], state.value[i]);}
		if(strcmp(state.names[i], "T0") == 0){
			newCommandValues.temp = state.value[i];
		} else if(strcmp(state.names[i], "P0") == 0){
			newCommandValues.press = state.value[i];
		} else if(strcmp(state.names[i], "M0RPM") == 0){
			motorSetNewCommandValuesRpm(state.value[i]);
		} else if(strcmp(state.names[i], "M0ON") == 0){
			motorSetNewCommandValuesOn(state.value[i]);
		} else if(strcmp(state.names[i], "M0OFF") == 0){
			motorSetNewCommandValuesOff(state.value[i]);
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
	if (daemonGetSettingsDebug_enabled()){printf("evaluateInput: T0: %.0f, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %.0f, STIME: %d, SMODE: %d, SID: %d\n", newCommandValues.temp, newCommandValues.press, motorGetNewCommandValuesRpm(), motorGetNewCommandValuesOn(), motorGetNewCommandValuesOff(), newCommandValues.weight, newCommandValues.time, newCommandValues.mode, newCommandValues.stepId);}
	
	if(newCommandValues.temp>200) {newCommandValues.temp=200;}
	if(newCommandValues.press>80) {newCommandValues.press=80;}
	if (motorGetNewCommandValuesOn()>0 && motorGetNewCommandValuesOn()<2) motorSetNewCommandValuesOn(2);
	if (motorGetNewCommandValuesOff()>0 && motorGetNewCommandValuesOff()<2) motorSetNewCommandValuesOff(2);
}

/** @brief read one ligne of the file fp
 *  @param keyString : name of value
 *  @param valueString : value
 *  @param fp : file to read
 *  @return true line was read or false in a other case
 */
bool readConfigLine(char* keyString, char* valueString, FILE *fp){
	char c;
	char c2;
	
	uint8_t i = 0;
	c = fgetc(fp);
	while (c != 255){ // 255=nothing
		if (c == '#'){ 
			if (daemonGetSettingsDebug_enabled()){printf("\tline with # found\n");}
			while (c != '\r' && c != '\n' && c != 255){
				c = fgetc(fp);
			}
		} else if (c == '\n'){
			//Empty line or end of last line
		} else if (c == '\r'){
			//Empty line
		} else {// good line
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
				if (daemonGetSettingsDebug_enabled()){printf("\tline with # at end of value found\n");}
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

/** @brief Read the configuration
 */
void ReadConfigurationFile(void){
	if (daemonGetSettingsDebug_enabled()){printf("ReadConfigurationFile...\n");}
	
	bool showReadedConfigs = daemonGetSettingsDebug_enabled();
	
	FILE *fp;
	char keyString[30];
	char valueString[100];
	StringClean(keyString, 30);
	StringClean(valueString, 100);
	fp = fopen(settings.configFile, "r");
	if (fp != NULL){// if file exist
		while (readConfigLine(&keyString[0], &valueString[0], fp)){
			if (daemonGetSettingsDebug_enabled()){printf("\tkey: '%s', value: '%s'\n", keyString, valueString);}
			
			//ParseConfigValue
			if(strcmp(keyString, "BeepWeightReached") == 0){
				settings.BeepWeightReached = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tBeepWeightReached: %d\n", settings.BeepWeightReached);} // (old: %d)
			} else if(strcmp(keyString, "BeepStepEnd") == 0){
				beeperSetSettingsBeepStepEnd(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\tBeepStepEnd: %d\n", beeperGetSettingsBeepStepEnd());} // (old: %d)
			} else if(strcmp(keyString, "doRememberBeep") == 0){
				beeperSetDoRememberBeep(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\tdoRememberBeep: %d\n", beeperGetDoRememberBeep());} // (old: %d)
			} else if(strcmp(keyString, "DeleteLogOnStart") == 0){
				settings.DeleteLogOnStart = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tDeleteLogOnStart: %d\n", settings.DeleteLogOnStart);} // (old: %d)
				
			} else if(strcmp(keyString, "calibrationFile") == 0){
//				free(settings.calibrationFile);
				settings.calibrationFile = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.calibrationFile[0], valueString);
				if (showReadedConfigs){printf("\tcalibrationFile: %s\n", settings.calibrationFile);} // (old: %s)
			} else if(strcmp(keyString, "LogFile") == 0){
//				free(settings.logFile);
				settings.logFile = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.logFile[0], valueString);
				if (showReadedConfigs){printf("\tLogFile: %s\n", settings.logFile);} // (old: %s)
			} else if(strcmp(keyString, "CommandFile") == 0){
//				free(settings.commandFile);
				settings.commandFile = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.commandFile[0], valueString);
				if (showReadedConfigs){printf("\tCommandFile: %s\n", settings.commandFile);} // (old: %s)
			} else if(strcmp(keyString, "StatusFile") == 0){
//				free(settings.statusFile);
				settings.statusFile = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.statusFile[0], valueString);
				if (showReadedConfigs){printf("\tStatusFile: %s\n", settings.statusFile);} // (old: %s)
			} else if(strcmp(keyString, "hourCounterFile") == 0){
//				free(settings.hourCounterFile);
				settings.hourCounterFile = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.hourCounterFile[0], valueString);
				if (showReadedConfigs){printf("\thourCounterFile: %s\n", settings.hourCounterFile);}
			} else if(strcmp(keyString, "installPath") == 0){
//				free(settings.installPath);
				settings.installPath = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.installPath[0], valueString);
				if (showReadedConfigs){printf("\tinstallPath %s\n", settings.installPath);} // (old: %s)
			} else if(strcmp(keyString, "atmelDevicePath") == 0){
//				free(settings.atmelDevicePath);
				settings.atmelDevicePath = (char *) malloc(strlen(valueString) * sizeof(char) + 1);
				strcpy(&settings.atmelDevicePath[0], valueString);
				if (showReadedConfigs){printf("\tatmelDevicePath %s\n", settings.atmelDevicePath);} // (old: %s)
			} else if(strcmp(keyString, "speakLanguage") == 0){
//				free(state.language);
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
				motorSetI2cValuesSpeedMin(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_motor_speed_min: %d\n", motorGetI2cValuesSpeedMin());}
			} else if(strcmp(keyString, "i2c_motor_speed_ramp") == 0){
				motorSetI2cValuesSpeedRamp(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_motor_speed_ramp %d\n", motorGetI2cValuesSpeedRamp());}
			
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
				if (daemonGetSettingsDebug_enabled()){printf("\tkey not Found\n");}
			}
			StringClean(keyString, 30);
			StringClean(valueString, 100);
		}
	} else { // if file doesn't exist
		printf("config file '%s' not found!", settings.configFile);
	}
	
	fclose(fp);
	
	if (settings.shieldVersion < 1 && settings.shieldVersion > 4){
		printf("Shield version %d unknown stop daemon.\n", settings.shieldVersion);
		exit(1);
	}
	
	setADCModeReg(adc_config.ADC_update_rate & 0x000F);
	
	if (daemonGetSettingsDebug_enabled()){printf("done.\n");}
}

/** @brief Read the calibration
 */
void ReadCalibrationFile(void){
	if (daemonGetSettingsDebug_enabled()){printf("ReadCalibrationFile...\n");}
	
	bool showReadedConfigs = daemonGetSettingsDebug_enabled() || runningMode.calibration;
	
	FILE *fp;
	char keyString[30];
	char valueString[100];
	uint8_t ptr = 0;
	StringClean(keyString, 30);
	StringClean(valueString, 100);
	fp = fopen(settings.calibrationFile, "r");
	if (fp != NULL){
		while (readConfigLine(&keyString[0], &valueString[0], fp)){
			if (daemonGetSettingsDebug_enabled()){printf("\tkey: '%s', value: '%s'\n", keyString, valueString);}
			
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
			
			//Hystereis
			} else if(strcmp(keyString, "PressHystereis") == 0){
				adc_config.PressHystereis = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tPressHystereis: %d\n", adc_config.PressHystereis);}
			} else if(strcmp(keyString, "TempHystereis") == 0){
				adc_config.TempHystereis = StringConvertToNumber(valueString);
				if (showReadedConfigs){printf("\tTempHystereis: %d\n", adc_config.TempHystereis);}
			
			//7seg pins
			} else if(strcmp(keyString, "i2c_7seg_top") == 0){
				displaySetI2c_7seg_top(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_7seg_top: %d\n", displayGetI2c_7seg_top());}
			} else if(strcmp(keyString, "i2c_7seg_top_left") == 0){
				displaySetI2c_7seg_top_left(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_7seg_top_left: %d\n", displayGetI2c_7seg_top_left());}
			} else if(strcmp(keyString, "i2c_7seg_top_right") == 0){
				displaySetI2c_7seg_top_right(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_7seg_top_right: %d\n", displayGetI2c_7seg_top_right());}
			} else if(strcmp(keyString, "i2c_7seg_center") == 0){
				displaySetI2c_7seg_center(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_7seg_center: %d\n", displayGetI2c_7seg_center());}
			} else if(strcmp(keyString, "i2c_7seg_bottom_left") == 0){
				displaySetI2c_7seg_bottom_left(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_7seg_bottom_left: %d\n", displayGetI2c_7seg_bottom_left());}
			} else if(strcmp(keyString, "i2c_7seg_bottom_right") == 0){
				displaySetI2c_7seg_bottom_right(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_7seg_bottom_right: %d\n", displayGetI2c_7seg_bottom_right());}
			} else if(strcmp(keyString, "i2c_7seg_bottom") == 0){
				displaySetI2c_7seg_bottom(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_7seg_bottom: %d\n", displayGetI2c_7seg_bottom());}
			} else if(strcmp(keyString, "i2c_7seg_period") == 0){
				displaySetI2c_7seg_period(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_7seg_period: %d\n", displayGetI2c_7seg_period());}
				
			} else if(strcmp(keyString, "i2c_motor") == 0){
				motorSetI2cConfig(StringConvertToNumber(valueString));
				if (showReadedConfigs){printf("\ti2c_motor: %d\n", motorGetI2cConfig());}
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
			
			//Solenoid values
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
				if (daemonGetSettingsDebug_enabled()){printf("\tkey not Found\n");}
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
	
	if (daemonGetSettingsDebug_enabled()){printf("done.\n");}
}


/** @brief return the valu of temperaure, pression and loadcell
*/
void *readADCValues(void *ptr){
	uint32_t delayWeight;
	uint32_t delayTempPess;
	
	if (runningMode.simulationMode){// if simulation mode
		state.lidClosed = true;
		delayWeight = 300;
		delayTempPess = 1000;
		while (state.running){		
			if (daemonGetCurrentCommandValuesMode() == MODE_SCALE){
				double weightValue;
				if (!state.scaleReady) {
					weightValue = 0.0;
				} else {
					if (daemonGetTimeValuesRunTime() <= timeValues.simulationUpdateTime){
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
						timeValues.simulationUpdateTime = daemonGetTimeValuesRunTime();
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
			if (daemonGetCurrentCommandValuesMode() != MODE_SCALE){
				double tempValue = oldCommandValues.temp;
				int deltaT=newCommandValues.temp-oldCommandValues.temp;
				if (daemonGetCurrentCommandValuesMode()==MODE_HEATUP || daemonGetCurrentCommandValuesMode()==MODE_COOK) {
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
				} else if (daemonGetCurrentCommandValuesMode()==MODE_COOLDOWN){
					tempValue = tempValue-1;
				}
				adc_values.Temp.valueByOffset = tempValue;
				adc_values.Temp.value = adc_values.Temp.valueByOffset - tempCalibration.offset;
				adc_values.Temp.adc_value = adc_values.Temp.value / tempCalibration.scaleFactor;
				
				int32_t pressValue = oldCommandValues.press;
				int deltaP=newCommandValues.press - oldCommandValues.press;
				if (daemonGetCurrentCommandValuesMode() == MODE_PRESSUP || daemonGetCurrentCommandValuesMode() == MODE_PRESSHOLD) {
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
				} else if (daemonGetCurrentCommandValuesMode() == MODE_PRESSDOWN){
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
		if (daemonGetCurrentCommandValuesMode() == MODE_SCALE || millis() - timeValues.lastWeightUpdateTime > 2000 || runningMode.calibration || runningMode.measure_noise){
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
			
			if (settings.shieldVersion < 4){//if shield Version 1, 2 or 3
				if (daemonGetCurrentCommandValuesMode() != MODE_SCALE){
					//check isLidClosed
					double front = (adc_values.LoadCellFrontLeft.valueByOffset + adc_values.LoadCellFrontRight.valueByOffset) / 2 - adc_values.Weight.valueByOffset;
					double back = (adc_values.LoadCellBackLeft.valueByOffset + adc_values.LoadCellBackRight.valueByOffset) / 2 - adc_values.Weight.valueByOffset;
					
					double diff = front - back;
					state.lidClosed = diff >= -1000;// close/open
					if (daemonGetSettingsDebug3_enabled()) {printf("lidClosed front: %2.f, back: %2.f, diff: %2.f, lidClosed: %d\n", front, back, diff, state.lidClosed);}
				}
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
		if (daemonGetCurrentCommandValuesMode() != MODE_SCALE || runningMode.calibration || runningMode.measure_noise){
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
			
			//Validate Temp/press is valid value
			if (!runningMode.calibration && !runningMode.measure_noise && (adc_values.Temp.valueByOffset < 0 || adc_values.Temp.valueByOffset > 300 || adc_values.Press.valueByOffset < -10 || adc_values.Press.valueByOffset > 100)){
				adc_config.restarting_adc = true;
				if (settings.shieldVersion < 4){
					SegmentDisplaySimple('E', &state, &i2c_config);
				} else {
					//TODO atmel show error
				}
				//adc_values.Temp.valueByOffset = 0;
				//adc_values.Press.valueByOffset = 0;
				heaterOff();
				AD7794Init();
				adc_config.restarting_adc = false;
			}
			
			delay(delayTempPess);
		}
	}
	return 0;
}

/** @brief return the value of buttons and speak
*/
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
		runTimeMillis = millis();//save time
		i=0;
		changed = false;
		for(; i<buttonCount; ++i){
			but[i] = readButton(i);
			if (but[i]){//si bouton on
				if (but[i] != buttonValues.button[i]){//value change
					buttonValues.buttonOnTime[i] = runTimeMillis;//save time
					changed = true;
				}
				buttonValues.buttonPressedTime[i] = runTimeMillis - buttonValues.buttonOnTime[i];
			} else {// if button off
				if (but[i] != buttonValues.button[i]){//value change
					buttonValues.buttonOffTime[i] = runTimeMillis;//save time
					changed = true;
				}
			}
			buttonValues.button[i] = but[i];//save new button's value
			if (changed){
				printf("\t%d (pressed: %d, ontime: %d, offtime: %d)", but[i], buttonValues.buttonPressedTime[i], buttonValues.buttonOnTime[i], buttonValues.buttonOffTime[i]);
			}
		}
		if (changed){
			printf("\n");
		}
		
		//Check button functions
		uint8_t buttonNumber = 2;
		if (buttonValues.button[buttonNumber]){// button back
			//if button is (still) pressed
			if (buttonValues.buttonPressedTime[buttonNumber] > 60000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speakerSpeak("no more commands");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 50000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speakerSpeak("release for shutdown");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 40000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speakerSpeak("release for ???");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 30000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speakerSpeak("release for read IP");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 20000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 10000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speakerSpeak("release for restart network");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 10000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 5000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speakerSpeak("release for restart deamon & middleware");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 5000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 5000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speakerSpeak("release for switch WLAN mode");
				}
			}
		} else if (buttonValues.actionStartedTime[buttonNumber] > 0 && runTimeMillis - buttonValues.actionStartedTime[buttonNumber] < 60000) {
			buttonValues.actionStartedTime[buttonNumber] = 0;
			//if button is not pressed (any more) and release is less then 60 seconds ago
			if (buttonValues.buttonPressedTime[buttonNumber] > 50000){
				char command[400];
				sprintf(command, "sudo halt");
				if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled() || daemonGetSettingsDebug4_enabled()){printf(command);}
				int result = system( command );
				if (result != 0){
					printf("ERROR: return code of command '%s' was %d", command, result);
				} else {
					exit(0);
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 40000){
				speakerSpeak("release for ???");
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 30000){
				char command[400];
				//sprintf(command, "%s/getServerIp.sh | espeak -a 200 -v %s 2>&1", settings.installPath, state.language);
				sprintf(command, "%s/getServerIp.sh | espeak -a 200 2>&1 > /dev/nul", settings.installPath);
				if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled() || daemonGetSettingsDebug4_enabled()){printf(command);}
				int result = system( command );
				if (result != 0){
					printf("ERROR: return code of command '%s' was %d", command, result);
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 20000){
				char command[400];
				sprintf(command, "%s/installSettings restartNetwork", settings.installPath);
				if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled() || daemonGetSettingsDebug4_enabled()){printf(command);}
				int result = system( command );
				if (result != 0){
					printf("ERROR: return code of command '%s' was %d", command, result);
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 10000){
				char command[400];
				sprintf(command, "%s/installSettings restartDaemonAndMiddleware", settings.installPath);
				if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled() || daemonGetSettingsDebug4_enabled()){printf(command);}
				int result = system( command );
				if (result != 0){
					printf("ERROR: return code of command '%s' was %d", command, result);
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 5000){
				char command[400];
				sprintf(command, "%s/installSettings change_wlanmode toggle", settings.installPath);
				if (daemonGetSettingsDebug_enabled() || daemonGetSettingsDebug3_enabled() || daemonGetSettingsDebug4_enabled()){printf(command);}
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
					speakerSpeak("no more commands");
				}
			} else if (buttonValues.buttonPressedTime[buttonNumber] > 10000){
				if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 5000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speakerSpeak("release for switch always read mode");
				}
			} else {
				if (buttonValues.buttonPressedTime[buttonNumber] < 500 && runTimeMillis - buttonValues.actionStartedTime[buttonNumber] > 5000){
					buttonValues.actionStartedTime[buttonNumber] = runTimeMillis;
					speakerSpeak(state.actionText);
				}
			}
		} else if (buttonValues.actionStartedTime[buttonNumber] > 0 && runTimeMillis - buttonValues.actionStartedTime[buttonNumber] < 20000) {
			buttonValues.actionStartedTime[buttonNumber] = 0;
			if (buttonValues.buttonPressedTime[buttonNumber] > 10000){
				state.alwaysReadMode = !state.alwaysReadMode;
				if (state.alwaysReadMode){
					speakerSpeak("always read mode is now on");
				} else {
					speakerSpeak("always read mode is now off");
				}
			} else if (runTimeMillis - buttonValues.actionStartedTime[buttonNumber] <= 5000){
				speakerSpeak(state.actionText);
			}
		}
		delay(buttonDelay);
	}
	return 0;
}
//function for the new mode
int newModeMain(){
	Vdebug=0;
	srand(time(NULL));
	int t=pthread_create(&readInput, NULL, readInputFunction, NULL);
	while(state.running){
		if(daemonGetSettingsShieldVersion()>3){
		}
		system("clear");
		printf("\n********************************************************************************");
		printf("\n Help : h  ***** version : %d ***** Mode actuel : %s ***** debug : %d",daemonGetSettingsShieldVersion(),modeNom,Vdebug);
		printf("\n********************************************************************************");
		switch (modeNum){
			case HELP :
				helpPrintf();
			break;
			case QUIT  :
				quitPrintf();
			break;
			case TEST_TEMPPRESS  :
				testTempPressPrintf();
			break;
			case TEST_VALVE  :
				testValvePrintf();
			break;
			case TEST_BOUTON  :
				testBoutonPrintf();
			break;
			case TEST_DISPLAY  :
				testSDisplayPrintf();
			break;
			case TEST_FAN  :
				testFanPrintf();
			break;
			case TEST_INDUCTION  :
				testInductionPrintf();
			break;
			case TEST_MOTOR  :
				testMotorPrintf();
			break;
			case TEST_WEIGHT  :
				testWeightPrintf();
			break;
			case TEST_SPEAKER  :
				testSpeakerPrintf();
			break;
			default :
				printf("ERROR = No modeNum");
			break;
			}
		printf("\nWhat do you want to do : ");
		printf("\n");
		usleep(1000000);
	};
	pthread_join(readInput, NULL);// wait end of thread
	return 0;
}

void *readInputFunction(void *ptr){
	uint16_t isNumber;
	while(state.running){
	char str[50]; // 50 caractères + '\0' terminal
	fgets(str, 50, stdin);
	str[strlen(str)-1]='\0'; //enlève le '\n'
	char* temp= &str;
	 switch(*temp)
    {
		case 'h' :
        case 'H' :
			modeNom="help";
			modeNum=HELP;
            break;
        case 't' :
        case 'T' :
			modeNom="test temp/press";
			modeNum=TEST_TEMPPRESS;
			tabTemppressData.nbData=0;
            break;
        case 'v' :
        case 'V' :
			modeNom="test valve";
			modeNum=TEST_VALVE;
            break;
        case 'b' :
        case 'B' :
			modeNom="test button";
			modeNum=TEST_BOUTON;
            break;
        case 's' :
        case 'S' :
			modeNom="test speaker";
			modeNum=TEST_SPEAKER;
            break;
        case 'd' :
        case 'D' :
			if(modeNum==TEST_SPEAKER){
				speakerLanguageDeutsch();
			}else{
				modeNom="test display";
				modeNum=TEST_DISPLAY;
			}
            break;
        case 'e' :
        case 'E' :
			if(modeNum==TEST_SPEAKER){
				speakerLanguageEnglish();
			}
            break;
        case 'q' :
        case 'Q' :
			modeNom="quit";
			modeNum=QUIT;
            break;
		case 'f' :
        case 'F' :
			if(modeNum==TEST_SPEAKER){
				speakerLanguageFrancais();
			}else{
				modeNom="test fan";
				modeNum=TEST_FAN;
			}
			break;
		case 'i' :
        case 'I' :
			modeNom="test induction";
			modeNum=TEST_INDUCTION;
			break;
		case 'm' :
        case 'M' :
			modeNom="test motor";
			modeNum=TEST_MOTOR;
			break;
		case 'w' :
        case 'W' :
			modeNom="test weight";
			modeNum=TEST_WEIGHT;
			tabWeightData.nbData=0;
            break;
        case 'p' :
        case 'P' :
			displayMode=4;
			displayShowText("I love everycook");
        break;
		case 'y' :
        case 'Y' :
			switch (modeNum){
				case QUIT :
					state.running=false;
				break;
				case TEST_DISPLAY :
					displayMode=1;
					displayFill();
				break;
				case TEST_INDUCTION :
					heaterOn();
					inductionIsRunning=true;
				break;
				case TEST_TEMPPRESS :
					heaterOn();
					inductionIsRunning=true;
				break;
				default :
				break;
			}
		break;
		case 'n' :
        case 'N' :
			switch (modeNum){
				case QUIT :
					modeNom="help";
					modeNum=HELP;
				break;
				case TEST_DISPLAY :
					displayMode=0;
					displayClear();
				break;
				case TEST_INDUCTION :
					heaterOff();
					inductionIsRunning=false;
				break;
				case TEST_TEMPPRESS :
					heaterOff();
					inductionIsRunning=false;
				break;
				case TEST_SPEAKER :
					speakerSpeakLanguage("blalbal");
				break;
				default :
				break;
			}
		break;
        case 'o' :
        case 'O' :
			switch (modeNum){
				case TEST_VALVE :
					solenoidSetOpen(true);
					solenoidIsOpen=true;
				break;
				default :
				break;
			}
		break;
		case 'c' :
		case 'C' :
			switch (modeNum){
				case TEST_VALVE :
					solenoidSetOpen(false);
					solenoidIsOpen=false;
				break;
				default :
				break;
			}
		break;
		case '0':
			switch (modeNum){
				case TEST_INDUCTION :
					inductionPwm=0;
					inductionIsRunning=false;
					heaterOff();
				break;
				case TEST_FAN :
					fanPwm=0;
					//PWM 0 of Fan
				break;
				case TEST_MOTOR:
						motorRPMDesired=0;
						motorPWMDesired=0;
						motorSetCommandRPM(motorRPMDesired);
						motorSetCommandRPM(0);
				break;
				case TEST_VALVE :
					solenoidPwm=0;
					writeI2CPin(i2c_config.i2c_servo, solenoidPwm);
					solenoidIsOpen=false;
				default:
				break;
			}
		
		break;
		default :
		break;
    }
		isNumber=atoi(temp);
		if(isNumber!=0){
			switch (modeNum){
				case TEST_DISPLAY:
					if(isNumber==1){
						if(displayLedShined[isNumber-1]){
							displaySetTop(false);
						}else{
							displaySetTop(true);
						}
						displayMode=2;
						if(daemonGetSettingsShieldVersion()>3){
							uint16_t picture2[9]  = {	0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010,
											0b0001010101010101,
											0b0010101010101010};
							displayShowPicture(&picture2[0]);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==2){
						if(displayLedShined[isNumber-1]){
							displaySetTopLeft(false);
						}else{
							displaySetTopLeft(true);
						}
						displayMode=3;
						if(daemonGetSettingsShieldVersion()>3){
							uint16_t picture[9] = {	0b0000000111110000,
										0b0000001000001000,
										0b0000010000000100,
										0b0000010100010100,
										0b0000010000000100,
										0b0000010100010100,
										0b0000010011100100,
										0b0000001000001000,
										0b0000000111110000};
							displayShowPicture(&picture[0]);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==3){
						if(displayLedShined[isNumber-1]){
							displaySetBottomLeft(false);
						}else{
							displaySetBottomLeft(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==4){
						if(displayLedShined[isNumber-1]){
							displaySetBottom(false);
						}else{
							displaySetBottom(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==5){
						if(displayLedShined[isNumber-1]){
							displaySetBottomRight(false);
						}else{
							displaySetBottomRight(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==6){
						if(displayLedShined[isNumber-1]){
							displaySetTopRight(false);
						}else{
							displaySetTopRight(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==7){
						if(displayLedShined[isNumber-1]){
							displaySetCenter(false);
						}else{
							displaySetCenter(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					}else if(isNumber==8){
						if(displayLedShined[isNumber-1]){
							displaySetPeriod(false);
						}else{
							displaySetPeriod(true);
						}
						displayLedShined[isNumber-1]=!displayLedShined[isNumber-1];
					} 
				break;
				case TEST_INDUCTION :
					inductionPwm=isNumber;
					//PWM pour le heater
				break;
				case TEST_FAN :
					fanPwm=isNumber;
					//PWM of Fan
				break;
				case TEST_MOTOR:
						motorRPMDesired=isNumber;
						motorPWMDesired=isNumber;
						motorSetSpeedRPM(motorRPMDesired);
				break;
				case TEST_VALVE :
					solenoidPwm=isNumber;
					writeI2CPin(i2c_config.i2c_servo, solenoidPwm);
					solenoidIsOpen=true;
					break;
				case TEST_SPEAKER :
					if(isNumber==1){
						speakerSpeakLanguage("slowly");
					}else if(isNumber==2){
						speakerSpeakLanguage("stop");
					}
					break;
				default :
				break;
			}
		}
	}
	printf("readInputFunction is finished");
	return 0;
}

void helpPrintf(){
	printf("\nMode enable :");
	printf("\ns : test speaker");
	printf("\nt : test temp/press");
	printf("\nb : test button");
	printf("\nd : test display");
	printf("\nf : test fan");
	printf("\ni : test induction");
	printf("\nm : test motor");
	printf("\nw : test weight");
	printf("\nv : test valve");
	printf("\nq : quit");
}
void quitPrintf(){
	printf("\nDo you want to quit? (y,n)");
}
void testTempPressPrintf(){
	int i;
	newTemppressData.pressInput=adc_values.Press.adc_value;
	newTemppressData.tempInput=adc_values.Temp.adc_value;
	newTemppressData.pressPasc=adc_values.Press.valueByOffset;
	newTemppressData.tempDeg=adc_values.Temp.valueByOffset;
	if(tabTemppressData.nbData<NBTEMPPRESSLINE){
		allTemppressData[tabTemppressData.nbData].pressInput=newTemppressData.pressInput;
		allTemppressData[tabTemppressData.nbData].tempInput=newTemppressData.tempInput;
		allTemppressData[tabTemppressData.nbData].pressPasc=newTemppressData.pressPasc;
		allTemppressData[tabTemppressData.nbData].tempDeg=newTemppressData.tempDeg;
	}else{
		for(i=1;i<NBTEMPPRESSLINE;i++){
			allTemppressData[i-1].pressInput=allTemppressData[i].pressInput;
			allTemppressData[i-1].tempInput=allTemppressData[i].tempInput;
			allTemppressData[i-1].pressPasc=allTemppressData[i].pressPasc;
			allTemppressData[i-1].tempDeg=allTemppressData[i].tempDeg;

		}
		allTemppressData[NBTEMPPRESSLINE-1].pressInput=newTemppressData.pressInput;
		allTemppressData[NBTEMPPRESSLINE-1].tempInput=newTemppressData.tempInput;
		allTemppressData[NBTEMPPRESSLINE-1].pressPasc=newTemppressData.pressPasc;
		allTemppressData[NBTEMPPRESSLINE-1].tempDeg=newTemppressData.tempDeg;
	}
	if(tabTemppressData.nbData==0){
		tabTemppressData.max=newTemppressData;
		tabTemppressData.min=newTemppressData;
		tabTemppressData.avg=newTemppressData;
	}else{
		if(tabTemppressData.min.tempInput>newTemppressData.tempInput){
			tabTemppressData.min.tempInput=newTemppressData.tempInput;
		}
		if(tabTemppressData.min.tempDeg>newTemppressData.tempDeg){
			tabTemppressData.min.tempDeg=newTemppressData.tempDeg;
		}
		if(tabTemppressData.min.pressInput>newTemppressData.pressInput){
			tabTemppressData.min.pressInput=newTemppressData.pressInput;
		}
		if(tabTemppressData.min.pressPasc>newTemppressData.pressPasc){
			tabTemppressData.min.pressPasc=newTemppressData.pressPasc;
		}
		if(tabTemppressData.max.tempInput<newTemppressData.tempInput){
			tabTemppressData.max.tempInput=newTemppressData.tempInput;
		}
		if(tabTemppressData.max.tempDeg<newTemppressData.tempDeg){
			tabTemppressData.max.tempDeg=newTemppressData.tempDeg;
		}
		if(tabTemppressData.max.pressInput<newTemppressData.pressInput){
			tabTemppressData.max.pressInput=newTemppressData.pressInput;
		}
		if(tabTemppressData.max.pressPasc<newTemppressData.pressPasc){
			tabTemppressData.max.pressPasc=newTemppressData.pressPasc;
		}

		tabTemppressData.delta.tempInput=tabTemppressData.max.tempInput-tabTemppressData.min.tempInput;
		tabTemppressData.delta.tempDeg=tabTemppressData.max.tempDeg-tabTemppressData.min.tempDeg;
		tabTemppressData.delta.pressInput=tabTemppressData.max.pressInput-tabTemppressData.min.pressInput;
		tabTemppressData.delta.pressPasc=tabTemppressData.max.pressPasc-tabTemppressData.min.pressPasc;

		tabTemppressData.avg.tempInput=newTemppressData.tempInput+tabTemppressData.avg.tempInput;
		tabTemppressData.avg.tempDeg=newTemppressData.tempDeg+tabTemppressData.avg.tempDeg;
		tabTemppressData.avg.pressInput=newTemppressData.pressInput+tabTemppressData.avg.pressInput;
		tabTemppressData.avg.pressPasc=newTemppressData.pressPasc+tabTemppressData.avg.pressPasc;
	}
	tabTemppressData.nbData++;
	if(inductionIsRunning){
		printf("\nThe heater is running, press 'n' to desactivate. (n)");
	}else{
		printf("\nThe heater is not running, press 'y' to activate. (y)");
	}
	printf("\nPress 't' to reset the valuz (t)");
	printf("\n_____________________________________________________");
	printf("\n  Value :PressInput| PressPasc| TempInput|  TempDeg |");
	
	printf("\n  Max   :");
	printColumnVAr(WIDHTCOLUMN,4,tabTemppressData.max.pressInput,tabTemppressData.max.pressPasc,tabTemppressData.max.tempInput,tabTemppressData.max.tempDeg);
	printf("\n  Min   :");
	printColumnVAr(WIDHTCOLUMN,4,tabTemppressData.min.pressInput,tabTemppressData.min.pressPasc,tabTemppressData.min.tempInput,tabTemppressData.min.tempDeg);
	printf("\n Delta  :");
	printColumnVAr(WIDHTCOLUMN,4,tabTemppressData.delta.pressInput,tabTemppressData.delta.pressPasc,tabTemppressData.delta.tempInput,tabTemppressData.delta.tempDeg);
	printf("\nAverage :");
	printColumnVAr(WIDHTCOLUMN,4,tabTemppressData.avg.pressInput/tabTemppressData.nbData,tabTemppressData.avg.pressPasc/tabTemppressData.nbData,tabTemppressData.avg.tempInput/tabTemppressData.nbData,tabTemppressData.avg.tempDeg/tabTemppressData.nbData);
	printf("\n___________________|__________|__________|__________|");
	if(tabTemppressData.nbData<NBWEIGHTLINE){
		for(i=0;i<tabTemppressData.nbData;i++){
			printf("\n         ");
			printColumnVAr(WIDHTCOLUMN,4,allTemppressData[i].pressInput,allTemppressData[i].pressPasc,allTemppressData[i].tempInput,allTemppressData[i].tempDeg);	
		}
	}else{
		for(i=0;i<NBWEIGHTLINE;i++){
			printf("\n         ");
			printColumnVAr(WIDHTCOLUMN,4,allTemppressData[i].pressInput,allTemppressData[i].pressPasc,allTemppressData[i].tempInput,allTemppressData[i].tempDeg);
		}
	}
	
}
void testValvePrintf(){
	printf("\nPress 'o' for open the valve or 'c' for close the valve.");
	if(solenoidIsOpen==true){
		printf("\nThe valve is opened");
	}else{
		printf("\nThe valve is closed");
	}
	if(daemonGetSettingsShieldVersion()<4){
		printf("\nEnter number betwenn 0 and 100 to activate the PWM : %d",solenoidPwm);
	}
}
void testBoutonPrintf(){
	boutonState[0]=readButton(0);
	boutonState[1]=readButton(1);
	boutonState[2]=readButton(2);
	boutonState[3]=state.lidClosed;
	boutonState[4]=state.lidLocked;
	boutonState[5]=state.pusherLocked;
	printf("\nBouton pressed 1 and release 0");
	if(boutonState[0]){
		printf("\nBouton 1 = 1");
	}else{
		printf("\nBouton 1 = 0");
	}
	if(boutonState[1]){
		printf("\nBouton 2 = 1");
	}else{
		printf("\nBouton 2 = 0");
	}
	if(boutonState[2]){
		printf("\nBouton 3 = 1");
	}else{
		printf("\nBouton 3 = 0");
	}
	if(boutonState[3]){
		printf("\nBouton lid closed = 1");
	}else{
		printf("\nBouton lid closed = 0");
	}
	if(boutonState[4]){
		printf("\nBouton lid locked = 1");
	}else{
		printf("\nBouton lid locked = 0");
	}
	if(boutonState[5]){
		printf("\nBouton pusher locked = 1");
	}else{
		printf("\nBouton pusher locked = 0");
	}

}
void testInductionPrintf(){
	if(daemonGetSettingsShieldVersion()<4){
		printf("\nwould you activate the heater? (y/n)");
	}else{
		printf("\nEnter number betwenn 0 and 100 to activate the PWM : %d",inductionPwm);
	}
	if(heaterGetStatusIsOn()){
		printf("\nThe heater is running and the PWM is %d",heaterGetPWMTrue());
	}else{
		printf("\nThe heater is not running");
	}
	if(heaterGetStatusErrorMsg() == NULL){
		printf("\nThere aren't error");
	}else{
		printf("\nThe error is %s",heaterGetStatusErrorMsg());
	}
	printf("\nThe temperatur of the transistor is %d",heaterGetTempTrans());
	printf("The temperatur of the transistor is %d",heaterGetTempTrans());
}
void testSDisplayPrintf(){
	printf("\nthe Shield version is %d",daemonGetSettingsShieldVersion());
	if(daemonGetSettingsShieldVersion()<4){
	printf("\nwitch led would you want to switch on or switch off? (1-8)");
	if(displayLedShined[0]){
		printf("\n                       ___1___        Led shined :   1      ");
	}else{
		printf("\n                       ___1___        Led shined :          ");
	}
	if(displayLedShined[1]){
		printf("\n                      |       |                      2      ");
	}else{
		printf("\n                      |       |                             ");
	}
	if(displayLedShined[2]){
		printf("\n                     2|       |6                     3      ");
	}else{
		printf("\n                     2|       |6                            ");
	}
	if(displayLedShined[3]){
		printf("\n                      |_______|                      4      ");
	}else{
		printf("\n                      |_______|                             ");
	}
	if(displayLedShined[4]){
		printf("\n                      |   7   |                      5      ");
	}else{
		printf("\n                      |   7   |                             ");
	}
	if(displayLedShined[5]){
		printf("\n                     3|       |5                     6      ");
	}else{
		printf("\n                     3|       |5                            ");
	}
	if(displayLedShined[6]){
		printf("\n                      |_______|                      7      ");
	}else{
		printf("\n                      |_______|                             ");
	}
	if(displayLedShined[7]){
		printf("\n                          4    *8                    8      ");	
	}else{
		printf("\n                          4    *8                           ");	
	}	
	}else{
	printf("\nWould you want to turn on or off all Leds? (y/n)");
	printf("\nWhat picture would you want to show? (Chess = 1/Smilly = 2)");
	printf("\nPress 'p' to show the following text 'I love everycook' parades before your eyes");
	
	switch (displayMode){
		case 0 :
			printf("\n\n'_'=led turn off    '0'=led turn on\n");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
			printf("\n     ______________");
		break;
		case 1 :
			printf("\n\n'_'=led turn off    '0'=led turn on\n");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
			printf("\n     00000000000000");
		break;
		case 2 :
			printf("\n\n'_'=led turn off    '0'=led turn on\n");
			printf("\n     0_0_0_0_0_0_0_");
			printf("\n     _0_0_0_0_0_0_0");
			printf("\n     0_0_0_0_0_0_0_");
			printf("\n     _0_0_0_0_0_0_0");
			printf("\n     0_0_0_0_0_0_0_");
			printf("\n     _0_0_0_0_0_0_0");
			printf("\n     0_0_0_0_0_0_0_");
			printf("\n     _0_0_0_0_0_0_0");
			printf("\n     0_0_0_0_0_0_0_");
		break;
		case 3 :
			printf("\n\n'_'=led turn off    '0'=led turn on\n");
			printf("\n     _____00000____");
			printf("\n     ____0_____0___");
			printf("\n     ___0_______0__");
			printf("\n     ___0_0___0_0__");
			printf("\n     ___0_______0__");
			printf("\n     ___0_0___0_0__");
			printf("\n     ___0__000__0__");
			printf("\n     ____0_____0___");
			printf("\n     _____00000____");
		break;
		case 4 :
			printf("\n\nYour are going to see 'I love everycook' on the display");
		break;
		}
		
	}
}
void testFanPrintf(){
	printf("\nThe temperature of the transistor is %d",heaterGetTempTrans());
	printf("\nThe temperatur of the transistor is %d",heaterGetTempTrans());
	printf("\nPWM of fan is %d",heaterGetFanPWM());
	//printf("\nEnter number betwenn 0 and 100 to activate the PWM : %d",fanPwm);
	//printf("\nThe temperature of the transistor is %d",fantemp);
}
void testMotorPrintf(){
	motorRPMTrued=motorGetRPMTrue();
	motorSensord=motorGetSensor();
	if(daemonGetSettingsShieldVersion()<4){
		printf("\nWhat PWM would you want?");
	}else{
		printf("\nHow many RPM would you want?");
	}
	if(daemonGetSettingsShieldVersion()<4){
		printf("\nYour command is %d",motorPWMDesired);
	}else{
		printf("\nThe motor turn at %d RPM per second and your command is %d",motorRPMTrued,motorRPMDesired);
	}
	if(daemonGetSettingsShieldVersion()==4){
		printf("\nState of position sensor %d",motorSensord);
	}
}
void testWeightPrintf(){
	int i;
	newWeightData.backL=adc_values.LoadCellBackLeft.adc_value;
	newWeightData.backR=adc_values.LoadCellBackRight.adc_value;
	newWeightData.frontL=adc_values.LoadCellFrontLeft.adc_value;
	newWeightData.frontR=adc_values.LoadCellFrontRight.adc_value;
	newWeightData.average=(newWeightData.backL+newWeightData.backR+newWeightData.frontL+newWeightData.frontR)/4;//change
	newWeightData.averageG=adc_values.Weight.valueByOffset;
	if(tabWeightData.nbData<NBWEIGHTLINE){
		allWeightData[tabWeightData.nbData].backL=newWeightData.backL;
		allWeightData[tabWeightData.nbData].backR=newWeightData.backR;
		allWeightData[tabWeightData.nbData].frontL=newWeightData.frontL;
		allWeightData[tabWeightData.nbData].frontR=newWeightData.frontR;
		allWeightData[tabWeightData.nbData].average=newWeightData.average;
		allWeightData[tabWeightData.nbData].averageG=newWeightData.averageG;
	}else{
		for(i=1;i<NBWEIGHTLINE;i++){
			allWeightData[i-1].backL=allWeightData[i].backL;
			allWeightData[i-1].backR=allWeightData[i].backR;
			allWeightData[i-1].frontL=allWeightData[i].frontL;
			allWeightData[i-1].frontR=allWeightData[i].frontR;
			allWeightData[i-1].average=allWeightData[i].average;
			allWeightData[i-1].averageG=allWeightData[i].averageG;
		}
		allWeightData[NBWEIGHTLINE-1].backL=newWeightData.backL;
		allWeightData[NBWEIGHTLINE-1].backR=newWeightData.backR;
		allWeightData[NBWEIGHTLINE-1].frontL=newWeightData.frontL;
		allWeightData[NBWEIGHTLINE-1].frontR=newWeightData.frontR;
		allWeightData[NBWEIGHTLINE-1].average=newWeightData.average;
		allWeightData[NBWEIGHTLINE-1].averageG=newWeightData.averageG;
	}
	if(tabWeightData.nbData==0){
		tabWeightData.max=newWeightData;
		tabWeightData.min=newWeightData;
		tabWeightData.avg=newWeightData;
	}else{
		if(tabWeightData.min.backL>newWeightData.backL){
			tabWeightData.min.backL=newWeightData.backL;
		}
		if(tabWeightData.min.backR>newWeightData.backR){
			tabWeightData.min.backR=newWeightData.backR;
		}
		if(tabWeightData.min.frontL>newWeightData.frontL){
			tabWeightData.min.frontL=newWeightData.frontL;
		}
		if(tabWeightData.min.frontR>newWeightData.frontR){
			tabWeightData.min.frontR=newWeightData.frontR;
		}
		if(tabWeightData.min.average>newWeightData.average){
			tabWeightData.min.average=newWeightData.average;
		}
		if(tabWeightData.min.averageG>newWeightData.averageG){
			tabWeightData.min.averageG=newWeightData.averageG;
		}
		if(tabWeightData.max.backL<newWeightData.backL){
			tabWeightData.max.backL=newWeightData.backL;
		}
		if(tabWeightData.max.backR<newWeightData.backR){
			tabWeightData.max.backR=newWeightData.backR;
		}
		if(tabWeightData.max.frontL<newWeightData.frontL){
			tabWeightData.max.frontL=newWeightData.frontL;
		}
		if(tabWeightData.max.frontR<newWeightData.frontR){
			tabWeightData.max.frontR=newWeightData.frontR;
		}
		if(tabWeightData.max.average<newWeightData.average){
			tabWeightData.max.average=newWeightData.average;
		}
		if(tabWeightData.max.averageG<newWeightData.averageG){
			tabWeightData.max.averageG=newWeightData.averageG;
		}
		tabWeightData.delta.backL=tabWeightData.max.backL-tabWeightData.min.backL;
		tabWeightData.delta.backR=tabWeightData.max.backR-tabWeightData.min.backR;
		tabWeightData.delta.frontL=tabWeightData.max.frontL-tabWeightData.min.frontL;
		tabWeightData.delta.frontR=tabWeightData.max.frontR-tabWeightData.min.frontR;
		tabWeightData.delta.average=tabWeightData.max.average-tabWeightData.min.average;
		tabWeightData.delta.averageG=tabWeightData.max.backL-tabWeightData.min.averageG;

		tabWeightData.avg.backL=newWeightData.backL+tabWeightData.avg.backL;
		tabWeightData.avg.backR=newWeightData.backR+tabWeightData.avg.backR;
		tabWeightData.avg.frontL=newWeightData.frontL+tabWeightData.avg.frontL;
		tabWeightData.avg.frontR=newWeightData.frontR+tabWeightData.avg.frontR;
		tabWeightData.avg.average=newWeightData.average+tabWeightData.avg.average;
		tabWeightData.avg.averageG=newWeightData.averageG+tabWeightData.avg.averageG;
	}
	tabWeightData.nbData++;
	printf("\nPress 'w' to reset the valuz (w)");
	printf("\n__________________________________________________________________________");
	printf("\n Sensor :  FrontL  |  FrontR  |   BackL  |   BackR  |  Average | Average/G|");
	
	printf("\n  Max   :");
	printColumnVAr(WIDHTCOLUMN,6,tabWeightData.max.frontL,tabWeightData.max.frontR,tabWeightData.max.backL,tabWeightData.max.backR,tabWeightData.max.average,tabWeightData.max.averageG);
	printf("\n  Min   :");
	printColumnVAr(WIDHTCOLUMN,6,tabWeightData.min.frontL,tabWeightData.min.frontR,tabWeightData.min.backL,tabWeightData.min.backR,tabWeightData.min.average,tabWeightData.min.averageG);
	printf("\n Delta  :");
	printColumnVAr(WIDHTCOLUMN,6,tabWeightData.delta.frontL,tabWeightData.delta.frontR,tabWeightData.delta.backL,tabWeightData.delta.backR,tabWeightData.delta.average,tabWeightData.delta.averageG);
	printf("\nAverage :");
	printColumnVAr(WIDHTCOLUMN,6,tabWeightData.avg.frontL/tabWeightData.nbData,tabWeightData.avg.frontR/tabWeightData.nbData,tabWeightData.avg.backL/tabWeightData.nbData,tabWeightData.avg.backR/tabWeightData.nbData,tabWeightData.avg.average/tabWeightData.nbData,tabWeightData.avg.averageG/tabWeightData.nbData);
	printf("\n___________________|__________|__________|__________|__________|________________");
	if(tabWeightData.nbData<NBWEIGHTLINE){
		for(i=0;i<tabWeightData.nbData;i++){
			printf("\n         ");
			printColumnVAr(WIDHTCOLUMN,6,allWeightData[i].frontL,allWeightData[i].frontR,allWeightData[i].backL,allWeightData[i].backR,allWeightData[i].average,allWeightData[i].averageG);	
		}
	}else{
		for(i=0;i<NBWEIGHTLINE;i++){
			printf("\n         ");
			printColumnVAr(WIDHTCOLUMN,6,allWeightData[i].frontL,allWeightData[i].frontR,allWeightData[i].backL,allWeightData[i].backR,allWeightData[i].average,allWeightData[i].averageG);
		}
	}
}
void testSpeakerPrintf(){
	printf("\nFrancais : f           Currente language : %s",speakerCurrentLabguage());
	printf("\nEnglish  : e");   
	printf("\nDeutsch  : d");
	printf("\nPress n to make error in %s",speakerCurrentLabguage()); 
	printf("\nPress 1 to say 'slowly' in %s",speakerCurrentLabguage()); 
	printf("\nPress 2 to say 'stop' in %s",speakerCurrentLabguage()); 
}
void  printColumnVAr( uint16_t widthColumn, uint16_t nbColumn,...){
	va_list ap;
	int i;
    va_start(ap, nbColumn);
    for( i = 0 ; i < nbColumn ; i++){
		printMiddle(widthColumn,va_arg(ap,uint32_t));
		printf("|");
    }
    va_end(ap);
}

void printMiddle(uint16_t space, uint32_t nb){
	int nbsize = 0;
	int nbSpace=0;
	uint32_t nbtemp=nb;
	int i=0;
	if(nbtemp == 0) {
    nbsize=1;
	}else if(nbtemp < 0)  { //si tu compte le moins dans la longueur
		++nbsize;
	}
	while(nbtemp != 0) {
		nbtemp /= 10;
		++nbsize;
	}
	nbSpace=space-nbsize;
	if(nbSpace>-1){
		if(nbSpace%2==0){
			for(i=0;i<(nbSpace/2);i++){
				printf(" ");
			}
			printf("%d",nb);
			for(i=0;i<(nbSpace/2);i++){
				printf(" ");
			}
		}else{
			for(i=0;i<((nbSpace/2)+1);i++){
				printf(" ");
			}
			printf("%d",nb);
			for(i=0;i<(nbSpace/2);i++){
				printf(" ");
			}
		}
	}else{
		printf("The number is bigger than numberSpace");
	}
}
//end function for the new mode

//function get
bool daemonGetSettingsDebug_enabled(){
	return settings.debug_enabled;
}
bool daemonGetSettingsDebug3_enabled(){
	return settings.debug3_enabled;
}
bool daemonGetSettingsDebug4_enabled(){
	return settings.debug4_enabled;
}
uint32_t daemonGetTimeValuesRunTime(){
	return timeValues.runTime;
}
uint32_t daemonGetSettingsTimeValuesStepEndTime(){
	return timeValues.stepEndTime;
}
uint32_t daemonGetCurrentCommandValuesMode(){
	return currentCommandValues.mode;
}
uint32_t daemonGetSettingsShieldVersion(){
	return settings.shieldVersion;
}
bool daemonGetRunningModeSimulationMode(){
	return runningMode.simulationMode;
}
uint32_t daemonGetTimeValuesRunTimeMillis(){
	return timeValues.runTimeMillis;
}
uint32_t daemonGetSettingsLongDelay(){
	return settings.LongDelay;
}
uint32_t daemonGetTimeValuesLastLogSaveTime(){
	return timeValues.lastLogSaveTime;
}
bool daemonGetAdcConfigRestartingAdc(){
	return adc_config.restarting_adc;
}
bool daemonGetStateRunning(){
	return state.running;
}
uint32_t daemonGetStateDelay(){
	return state.Delay;
}
uint8_t daemonGetVdebug(){
	return Vdebug;
}
//SET
void daemonSetTimeValuesStepEndTime(uint32_t step){
	timeValues.stepEndTime=step;
}
void daemonSetStateDelay(uint32_t delay){
		state.Delay=delay;
}
void daemonSetTimeValuesRunTime(uint32_t runtime){
	timeValues.runTime=runtime;
}
void daemonSetTimeValuesRunTimeMillis(uint32_t runtimemillis){
	timeValues.runTimeMillis=runtimemillis;
}
void daemonSetStateLidClosed(bool stateLidClosed){
	state.lidClosed = stateLidClosed;
}
void daemonSetStateLidLocked(bool stateLidLocked){
	state.lidLocked = stateLidLocked;
}
void daemonSetStatePusherLocked(bool statePusherLocked){
	state.pusherLocked = statePusherLocked;
}
void daemonSetVdebug(uint8_t bug){
	Vdebug=bug;
}
