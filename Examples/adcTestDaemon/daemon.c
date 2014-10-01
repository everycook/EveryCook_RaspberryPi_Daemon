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
#include "ad7796_interface.h"


#include "daemon_structs.h"
#include "hardwareFunctions.h"
//#include "atmel.h"
//#include <wiringSerial.h>
#include "virtualspiAtmel.h"


const uint32_t MIN_STATUS_INTERVAL = 20;

const uint8_t ALLWAYS_STOP_BUZZING = 0;

const uint8_t dataType = TYPE_TEXT;

const double SECONDS_PER_HOUR = 3600; //60*60

const uint8_t HOUR_COUNTER_VERSION = 0;

struct ADC_Config adc_config = {0,1,2,3,4,5, 0,8, {0x710, 0x711, 0x712, 0x713, 0x714, 0x115}, {0,0,0,0,0,0}, 2, 2, false};
struct ADC_Noise_Values adc_noise = {0,1000000000,0, 0,1000000000,0, 0,1000000000,0, 0,1000000000,0, 0,1000000000,0, 0,1000000000,0};
struct ADC_Values adc_values;
struct ADC_Average_Values adc_average;

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

/** @brief program different mode according to what is chosen during the launch
 *  @param argc : mode number selected
 *  @param argv : mode which has been selected
*/
int parseParams(int argc, const char* argv[]);
/** @brief initialization signals
 */
void defineSignalHandler();

void handleSignal(int signum);

/** @brief creation of file
 */
void initOutputFile(void);

/** @brief return the valu of temperaure, pression and loadcell
*/
void *readADCValues(void *ptr);

/** @brief creat a thread to speak
 *  @param * text : text that is spoken
*/
void speak(char* text);

/** @brief fonction witch speaks
 *  @param *ptr : text that is spoken
*/
void *speakThreadFunc(void *ptr);

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
	daemon_values.heaterStatus = &heaterStatus;
	daemon_values.hourCounter = &hourCounter;
	daemon_values.buttonConfig = &buttonConfig;
	daemon_values.buttonValues = &buttonValues;
	daemon_values.adc_values = &adc_values;
	daemon_values.adc_average = &adc_average;
	
	printf("starting EveryCook daemon...\n");
	if (settings.debug_enabled){printf("main\n");}
	
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
		VirtualSPIAtmelInit(settings.debug_enabled); // initilalization PIN (SPI)
	}
	
	initOutputFile();// création of the file wich is use for web
	state.Delay = settings.LongDelay;
	
	resetValues(); //reset all value
	state.referenceForce = forceCalibration.offset; //for default reference Force in calibration mode

	
	if (settings.debug_enabled){printf("commandFile is: %s\n", settings.commandFile);}
	if (settings.debug_enabled){printf("statusFile is: %s\n", settings.statusFile);}
	
	//remove commandfile so no unexpectet action will be done
	if (remove(settings.commandFile) != 0){
		printf("Error while remove old commandfile: %s\n", settings.commandFile);
	}
		
	timeValues.runTime = time(NULL); //TODO WIA CHANGE THIS
	printf("runtime is now: %d \n", timeValues.runTime);
	timeValues.stepStartTime = timeValues.runTime; //TODO WIA CHANGE THIS
	
	
	if (settings.debug_enabled){printf("runningModes: normalMode:%d, calibration:%d, measure_noise:%d, test_7seg:%d, test_servo:%d, test_heat_led:%d, test_motor:%d, test_buttons:%d, test_adc:%d, test_heating_power:%d, test_heating_press:%d, test_serial:%d\n", runningMode.normalMode, runningMode.calibration, runningMode.measure_noise, runningMode.test_7seg, runningMode.test_servo, runningMode.test_heat_led, runningMode.test_motor, runningMode.test_buttons, runningMode.test_adc, runningMode.test_heating_power, runningMode.test_heating_press, runningMode.test_serial);}
	
	
	if (runningMode.calibration || runningMode.measure_noise){
		state.Delay = 50;
		pthread_create(&threadReadADCValues, NULL, readADCValues, NULL);// return temp,press and loadcell
		while (state.running){
			timeValues.runTime = time(NULL);
			timeValues.runTimeMillis = millis();
			//printf("we are in calibration mode, be careful!\n Heat will turn on automatically if switch is on!\n Use switch to turn heat off.\n");	
			//if (runningMode.calibration) HeatOn(&daemon_values);// heat turn on
			if (settings.debug_enabled || runningMode.calibration || settings.debug3_enabled || runningMode.measure_noise) {	
				printf("time %8d | ", timeValues.runTimeMillis);
			}
			
			/*
			if (settings.debug_enabled || runningMode.calibration || settings.debug3_enabled){printf("Temp %d dig %.0f °C | ", adc_values.Temp.adc_value, adc_values.Temp.valueByOffset);}
			if (runningMode.measure_noise) {printf("NoiseTemp %d | ", adc_noise.DeltaTemp);}
			
			if (settings.debug_enabled || runningMode.calibration || settings.debug3_enabled){printf("Press %d digits %.0f kPa | ", adc_values.Press.adc_value, adc_values.Press.valueByOffset);}
			if (runningMode.measure_noise){printf("NoisePress %d |", adc_noise.DeltaPress);}
			*/
			
			//if (settings.debug_enabled || runningMode.calibration){printf("Weight %d dig %.1f g / %.1f g | FL %d FR %d | ", adc_values.Weight.adc_value, adc_values.Weight.value, adc_values.Weight.valueByOffset, adc_values.LoadCellFrontLeft.adc_value, adc_values.LoadCellFrontRight.adc_value, adc_values.LoadCellBackLeft.adc_value, adc_values.LoadCellBackRight.adc_value);}
			
			printf("FL %6.1f FR %6.1f | ", adc_values.Weight.valueByOffset, adc_values.Press.valueByOffset);
			if (settings.debug_enabled || runningMode.calibration){printf("FL %d FR %d | ", adc_values.LoadCellFrontLeft.adc_value, adc_values.LoadCellFrontRight.adc_value);}
			
			printf("%d:avgFL %d %d:avgFR %d | ", adc_average.index1, (adc_average.sum1/10),  adc_average.index1, (adc_average.sum2/10));
			
			if (runningMode.measure_noise){
				printf("NWeightFL %d | ", adc_noise.DeltaWeight1);
				printf("NWeightFR %d | ", adc_noise.DeltaWeight2);
				printf("NMinFL %d NMaxFL %d | ", adc_noise.MinWeight1, adc_noise.MaxWeight1);
				printf("NMinFR %d NMaxFR %d", adc_noise.MinWeight2, adc_noise.MaxWeight2);
			}
			
			printf("\n");
			delay(state.Delay);
		}
		pthread_join(threadReadADCValues, NULL);// wait the end of thread
		//HeatOff(&daemon_values);// heat turn off
	} else if (runningMode.test_adc){
		state.Delay = 5;
		printf("test_adc\n");

		bool adc24Bit = SPISelectChip(0);
		
		struct adc_private adc;
		uint8_t adcChannel = 0;
		if (settings.use_spi_dev){//SPI configuration
			ad7796_reset(&adc);
			delay(30);
		
			ad7796_write_data(&adc, AD7796_MODE, settings.test_ADC_update_rate & 0x000F);
			ad7796_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
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
		
		if (settings.use_spi_dev){
		} else {
			uint32_t clock = internalClockAvail;
			adc24Bit = SPISelectChip(1);
			adcChannel = 0;
			SPIWrite2Bytes(WRITE_MODE_REG, clock | (settings.test_ADC_update_rate & 0x000F));
			SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
			
			clock = externalClock;
			adc24Bit = SPISelectChip(0);
			adcChannel = 0;
			SPIWrite2Bytes(WRITE_MODE_REG, clock | (settings.test_ADC_update_rate & 0x000F));
			SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
		}
		
		
		adc24Bit = SPISelectChip(1);
		adcChannel = 1;
		while (state.running){
			uint8_t adcState = 0;
			if (settings.use_spi_dev){
			} else {
				adcState = SPIReadByte(READ_STATUS_REG);
			}
			if (settings.debug_enabled){ printf("%d: state %d / %8s\n", millis(), adcState, my_itoa(adcState, 2)); }
			if ((adcState & AD7796_STATUS_NOTREADY_MASK) == 0){
				uint32_t data;
				if (settings.use_spi_dev){
					data = ad7796_read_data(&adc);
				} else {
					data = (adc24Bit)?SPIRead3Bytes(READ_DATA_REG):SPIRead2Bytes(READ_DATA_REG);
				}
				//printf("readADC(%d): %d / %06X, loops: %d, time: %d\n", adcChannel, data, data, amount, millis() - lastTime);
				if (adc_config.inverse[adcChannel]){
					if (adc24Bit){
						data = 0x00FFFFFF - data;
					} else {
						data = 0x0000FFFF - data;
					}
				}
			
				adcValues[adcChannel] = data;
				adcTime[adcChannel] = millis() - lastTime;
				adcLoops[adcChannel] = amount;
//				if (adcChannel < 7){
//					++adcChannel;
				if (adcChannel < 1){
					adc24Bit = SPISelectChip(1);
					adcChannel = 1;
				} else {
					adc24Bit = SPISelectChip(0);
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
/*
				//Set ADC Channel & Gain to use
				if (settings.use_spi_dev){
					ad7796_select_channel2(&adc, adcChannel, adc_config.ADC_ConfigReg[adcChannel]);
				} else {
					SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
				}
*/
			} else {
				++amount;
			}
			delay(state.Delay);
		}
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
	printf("Usage: ecdaemon OPTIONS\r\n");
	printf("programm options:\r\n");
	printf("  -d,  --debug                  activate Debug output\r\n");
	printf("  -d2, --debug2                 activate Debug output for basic functions\r\n");
	printf("  -d3, --debug3                 activate Debug output for main logic functions\r\n");
	printf("  -c,  --calibrate              calibration mode, will output raw adc values\r\n");
	printf("  -cg, --config <path>          set configfile path to <file>\r\n");
	printf("  -tn, --test-noise             measure and output random noise\r\n");
	printf("  -ta, --test-adc [updateRate [offset [fullScale]]\r\n");
	printf("                                test adc, set update rate setting to submitted value, use %d if ommited.\r\n", settings.test_ADC_update_rate);
	printf("                                offset/fullScale calibration values i or s, if omittet no calibration.\r\n");
	printf("  -?,  --help                   show this help\r\n");
}

/** @brief program different mode according to what is chosen during the launch
 *  @param argc : mode number selected
 *  @param argv : mode which has been selected
*/
int parseParams(int argc, const char* argv[]){
	int i;
	for (i=1; i<argc; ++i){ //param 0 is programmname
		if(strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0){
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
							//settings.test_ADC_fullScaleCalibration = internalFullScaleCalibration;
							printf("internalFullScaleCalibration on AD7796 not available, do systemFullScaleCalibration instat\n");
							settings.test_ADC_fullScaleCalibration = systemFullScaleCalibration;
						} else if (argv[i][0] == 's'){
							settings.test_ADC_fullScaleCalibration = systemFullScaleCalibration;
						}
					}
				}
			}
			printf("test adc\n");
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
	hourCounter.heater = 0;
	hourCounter.motor = 0;
}

/** @brief creation of file
 */
void initOutputFile(void){
	if (settings.debug_enabled){printf("iniOutputFile\n");}
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
	if (settings.shieldVersion < 4){
		writeI2CPin(i2c_config.i2c_servo, i2c_servo_values.i2c_servo_closed); // send the value by the I2C
	}
	
	heaterStatus.ledValues[0] = 0;
	heaterStatus.ledValues[1] = 0;
	heaterStatus.ledValues[2] = 0;
	heaterStatus.ledValues[3] = 0;
	heaterStatus.ledValues[4] = 0;
	heaterStatus.ledValues[5] = 0;
}

//void ScaleFunction(struct State state, struct Settings settings, struct Command_Values oldCommandValues, struct Command_Values currentCommandValues, struct Command_Values newCommandValues, struct Time_Values timeValues){
/** @brief control weight
*/
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
			state.weightPercent = 0;
			if (state.Delay == settings.ShortDelay){
				state.Delay=settings.LongDelay;
			}
			if (settings.debug_enabled){printf("\tscaleReady\n");}
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

/** @brief beep if doRememberBeep==1 (after 1 sec, 5 sec 30 sec...)
*/
void Beep(){
	if(settings.doRememberBeep==1){
		if (timeValues.runTime>timeValues.stepEndTime){//if step is ended
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
	sprintf(TotalUpdate, "{\"T0\":%.2f,\"P0\":%d,\"M0RPM\":%d,\"M0ON\":%d,\"M0OFF\":%d,\"W0\":%.0f,\"STIME\":%d,\"SMODE\":%d,\"SID\":%d,\"heaterHasPower\":%d,\"isOn\":%d,\"noPan\":%d,\"lidClosed\":%d,\"lidLocked\":%d,\"pusherLocked\":%d}",	currentCommandValues.temp, 						press, currentCommandValues.motorRpm, currentCommandValues.motorOn, currentCommandValues.motorOff, currentCommandValues.weight, currentCommandValues.time, currentCommandValues.mode, currentCommandValues.stepId, heaterStatus.hasPower, heaterStatus.isOn, heaterStatus.noPanError, state.lidClosed, state.lidLocked, state.pusherLocked);
	if (settings.debug_enabled){printf("prepareState: T0: %f, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %f, STIME: %d, SMODE: %d, SID: %d, heaterHasPower: %d, isOn: %d, noPan: %d, lidClosed:%d, lidLocked:%d, pusherLocked:%d\n", 		currentCommandValues.temp, currentCommandValues.press, currentCommandValues.motorRpm, currentCommandValues.motorOn, currentCommandValues.motorOff, currentCommandValues.weight, currentCommandValues.time, currentCommandValues.mode, currentCommandValues.stepId, heaterStatus.hasPower, heaterStatus.isOn, heaterStatus.noPanError, state.lidClosed, state.lidLocked, state.pusherLocked);}
}

/** @brief write data in settings.statusFile
 *  @param * data
*/
void writeStatus(char* data){
	if (settings.debug_enabled){printf("WriteStatus\n");}
	
	FILE *fp;
	fp = fopen(settings.statusFile, "w");
	fputs(data, fp);
	fclose(fp);
	if (settings.debug3_enabled){printf("%s\n", data);}
	
	if (settings.debug_enabled){printf("WriteStatus: after write\n");}
}


/** @ brief write on file settings.logFile and settings.hourCounterFile
*/
void writeLog(){
	if (settings.debug_enabled){printf("writeLog\n");}
	timeValues.nowTime = time(NULL);
	timeValues.localTime=localtime(&timeValues.nowTime);
	
	if (timeValues.runTime>=timeValues.lastLogSaveTime+settings.logSaveInterval){
		char tempString[20];
		StringClean(tempString, 20);
		strftime(tempString, 20,"%F %T",timeValues.localTime);
		char logline[200];
		sprintf(logline, "%s, %.1f, %i, %i, %.1f, %.1f, %i, %i, %.1f, %i, %i, %i, %i, %i, %i, %i, %i\n",tempString, currentCommandValues.temp, currentCommandValues.press, i2c_motor_values.motorRpm, currentCommandValues.weight, newCommandValues.temp, newCommandValues.press, newCommandValues.motorRpm, newCommandValues.weight, newCommandValues.mode, currentCommandValues.mode, heaterStatus.hasPower, heaterStatus.isOn, heaterStatus.noPanError, state.lidClosed, state.lidLocked, state.pusherLocked);
		
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
			if (settings.debug_enabled){printf("\tline with # found\n");}
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

/** @brief Read the configuration
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
	if (fp != NULL){// if file exist
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
				if (adc_config.ADC_ref != 0) {
					printf("\tError: for AD7796 only adc_ref 0(refin1) is allowed, config is: %d\n", adc_config.ADC_ref);
					adc_config.ADC_ref = 0;
				}
				
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
	} else { // if file doesn't exist
		printf("config file '%s' not found!", settings.configFile);
	}
	
	fclose(fp);
	
	if (settings.shieldVersion < 1 && settings.shieldVersion > 4){
		printf("Shield version %d unknown stop daemon.\n", settings.shieldVersion);
		exit(1);
	}
	
	setADCModeReg(adc_config.ADC_update_rate & 0x000F);
	
	if (settings.debug_enabled){printf("done.\n");}
}

/** @brief Read the calibration
 */
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
				if (adc_config.ADC_ConfigReg[ptr] != 128) {
					printf("\tError: for AD7796 only gain 128 is allowed, Gain_LoadCellFrontLeft config is: %d\n", adc_config.ADC_ConfigReg[ptr]);
					adc_config.ADC_ConfigReg[ptr] = 128;
				}
				adc_config.ADC_ConfigReg[ptr] = POWNTimes(adc_config.ADC_ConfigReg[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
				if (showReadedConfigs){printf("\tGain_LoadCellFrontLeft: %04X\n", adc_config.ADC_ConfigReg[ptr]);} // (old: %d)
			} else if(strcmp(keyString, "Gain_LoadCellFrontRight") == 0){
				ptr = adc_config.ADC_LoadCellFrontRight;
				adc_config.ADC_ConfigReg[ptr] = StringConvertToNumber(valueString);
				if (adc_config.ADC_ConfigReg[ptr] != 128) {
					printf("\tError: for AD7796 only gain 128 is allowed, Gain_LoadCellFrontRight config is: %d\n", adc_config.ADC_ConfigReg[ptr]);
					adc_config.ADC_ConfigReg[ptr] = 128;
				}
				adc_config.ADC_ConfigReg[ptr] = POWNTimes(adc_config.ADC_ConfigReg[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
				if (showReadedConfigs){printf("\tGain_LoadCellFrontRight: %04X\n", adc_config.ADC_ConfigReg[ptr]);} // (old: %d)
			} else if(strcmp(keyString, "Gain_LoadCellBackLeft") == 0){
				ptr = adc_config.ADC_LoadCellBackLeft;
				adc_config.ADC_ConfigReg[ptr] = StringConvertToNumber(valueString);
				if (adc_config.ADC_ConfigReg[ptr] != 128) {
					printf("\tError: for AD7796 only gain 128 is allowed, Gain_LoadCellBackLeft config is: %d\n", adc_config.ADC_ConfigReg[ptr]);
					adc_config.ADC_ConfigReg[ptr] = 128;
				}
				adc_config.ADC_ConfigReg[ptr] = POWNTimes(adc_config.ADC_ConfigReg[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
				if (showReadedConfigs){printf("\tGain_LoadCellBackLeft: %04X\n", adc_config.ADC_ConfigReg[ptr]);} // (old: %d)
			} else if(strcmp(keyString, "Gain_LoadCellBackRight") == 0){
				ptr = adc_config.ADC_LoadCellBackRight;
				adc_config.ADC_ConfigReg[ptr] = StringConvertToNumber(valueString);
				if (adc_config.ADC_ConfigReg[ptr] != 128) {
					printf("\tError: for AD7796 only gain 128 is allowed, Gain_LoadCellBackRight config is: %d\n", adc_config.ADC_ConfigReg[ptr]);
					adc_config.ADC_ConfigReg[ptr] = 128;
				}
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

uint32_t readADCByChannel(uint8_t adcChannel){
	bool adc24Bit = SPISelectChip(adcChannel);
	bool readyToRead = false;
	while (!readyToRead){//wait until the transmition is ready
		uint8_t adcState = SPIReadByte(READ_STATUS_REG);
		if ((adcState & 0x80) == 0){
			readyToRead = true;
		} else {
			delay(5);
		}
	}
	uint32_t data = (adc24Bit)?SPIRead3Bytes(READ_DATA_REG):SPIRead2Bytes(READ_DATA_REG);
	
	if (adc_config.inverse[adcChannel]){
		if (adc24Bit){
			data = 0x00FFFFFF - data;
		} else {
			data = 0x0000FFFF - data;
		}
	}
	return data;
}

/** @brief return the valu of temperaure, pression and loadcell
*/
void *readADCValues(void *ptr){
	uint32_t delayWeight;
	uint32_t delayTempPess;
	
	if (settings.use_spi_dev){
	} else {
		uint8_t adcChannel = 0;
		uint32_t clock = internalClockAvail;
		SPISelectChip(1);
		SPIWrite2Bytes(WRITE_MODE_REG, clock | (settings.test_ADC_update_rate & 0x000F));
		SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
		
		clock = externalClock;
		SPISelectChip(0);
		SPIWrite2Bytes(WRITE_MODE_REG, clock | (settings.test_ADC_update_rate & 0x000F));
		SPIWrite2Bytes(WRITE_CONFIG_REG, adc_config.ADC_ConfigReg[adcChannel]);
	}
	delay(200);
	
	delayWeight = 10;
	delayTempPess = 100;
	while (state.running){
		if (currentCommandValues.mode == MODE_SCALE || millis() - timeValues.lastWeightUpdateTime > 2000 || runningMode.calibration || runningMode.measure_noise){
			adc_values.LoadCellFrontLeft.adc_value = readADCByChannel(0);
			
			adc_values.LoadCellFrontLeft.value = (double)adc_values.LoadCellFrontLeft.adc_value * forceCalibration.scaleFactor;
			adc_values.LoadCellFrontLeft.valueByOffset=adc_values.LoadCellFrontLeft.value + state.referenceForce;
			adc_values.LoadCellFrontLeft.valueByOffset = round(adc_values.LoadCellFrontLeft.valueByOffset);
			
			
			
			adc_average.index1++;
			if (adc_average.index1>9){
				adc_average.index1 = 0;
			}
			adc_average.sum1 -= adc_average.values1[adc_average.index1];
			adc_average.values1[adc_average.index1] = adc_values.LoadCellFrontLeft.adc_value;
			adc_average.sum1 += adc_average.values1[adc_average.index1];
			
			if (adc_average.index1 == 0){
				adc_values.Weight.adc_value = adc_average.sum1 / 10;
				adc_values.Weight.value = (double)adc_values.Weight.adc_value * forceCalibration.scaleFactor;
				adc_values.Weight.valueByOffset=adc_values.Weight.value + forceCalibration.offset;
				adc_values.Weight.valueByOffset = round(adc_values.Weight.valueByOffset);
			}
			
			
			
			adc_values.LoadCellFrontRight.adc_value = readADCByChannel(1);
			
			adc_values.LoadCellFrontRight.value = (double)adc_values.LoadCellFrontRight.adc_value * forceCalibration.scaleFactor;
			adc_values.LoadCellFrontRight.valueByOffset=adc_values.LoadCellFrontRight.value + state.referenceForce;
			adc_values.LoadCellFrontRight.valueByOffset = round(adc_values.LoadCellFrontRight.valueByOffset);
			
			adc_average.index2++;
			if (adc_average.index2>9){
				adc_average.index2 = 0;
			}
			adc_average.sum2 -= adc_average.values2[adc_average.index2];
			adc_average.values2[adc_average.index2] = adc_values.LoadCellFrontRight.adc_value;
			adc_average.sum2 += adc_average.values2[adc_average.index2];
			
			
			if (adc_average.index2 == 0){	
				adc_values.Press.adc_value = adc_average.sum2 / 10;
				adc_values.Press.value = (double)adc_values.Press.adc_value * pressCalibration.scaleFactor;
				adc_values.Press.valueByOffset=adc_values.Press.value + pressCalibration.offset;
				adc_values.Press.valueByOffset = round(adc_values.Press.valueByOffset);
			}
			
			
			timeValues.lastWeightUpdateTime = millis();
			
			if (runningMode.measure_noise){
				if (adc_values.LoadCellFrontLeft.adc_value > adc_noise.MaxWeight1) adc_noise.MaxWeight1 = adc_values.LoadCellFrontLeft.adc_value;
				if (adc_values.LoadCellFrontLeft.adc_value < adc_noise.MinWeight1) adc_noise.MinWeight1 = adc_values.LoadCellFrontLeft.adc_value;
				adc_noise.DeltaWeight1 = adc_noise.MaxWeight1 - adc_noise.MinWeight1;
				
				if (adc_values.LoadCellFrontRight.adc_value > adc_noise.MaxWeight2) adc_noise.MaxWeight2 = adc_values.LoadCellFrontRight.adc_value;
				if (adc_values.LoadCellFrontRight.adc_value < adc_noise.MinWeight2) adc_noise.MinWeight2 = adc_values.LoadCellFrontRight.adc_value;
				adc_noise.DeltaWeight2 = adc_noise.MaxWeight2 - adc_noise.MinWeight2;
			}
			
			
			delay(delayWeight);
		}
	}
	return 0;
}

/** @brief creat a thread to speak
 *  @param * text : text that is spoken
*/
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

/** @brief fonction wich speaks
 *  @param *ptr : text that is spoken
*/
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
