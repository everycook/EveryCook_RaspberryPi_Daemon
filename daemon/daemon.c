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

#include "bool.h"
#include "modes.h"
#include "basic_functions.h"
#include "convertFunctions.h"
#include "middlewareSocket.h"
#include "daemon.h"

#include "daemon_structs.h"
#include "hardwareFunctions.h"

const float MOTOR_RPM_TO_PWM = 4095.0/200.0;

const uint8_t ALLWAYS_STOP_BUZZING = 0;

const uint8_t dataType = TYPE_TEXT;

struct ADC_Config adc_config = {0,1,2,3,4,5,0};
struct ADC_Noise_Values adc_noise = {0,1000000000,0, 0,1000000000,0, 0,1000000000,0, 0,1000000000,0, 0,1000000000,0, 0,1000000000,0};

struct ADC_Calibration forceCalibration = {};
struct ADC_Calibration pressCalibration = {};
struct ADC_Calibration tempCalibration = {};
struct I2C_Config i2c_config = {I2C_MOTOR, I2C_SERVO, I2C_7SEG_TOP, I2C_7SEG_TOP_LEFT, I2C_7SEG_TOP_RIGHT, I2C_7SEG_CENTER, I2C_7SEG_BOTTOM_LEFT, I2C_7SEG_BOTTOM_RIGHT, I2C_7SEG_BOTTOM, I2C_7SEG_PERIOD};

struct I2C_Servo_Values i2c_servo_values = {I2C_VALVE_OPEN_VALUE, I2C_VALVE_CLOSED_VALUE, 0, 0, 0, 0, 0, I2C_VALVE_CLOSED_VALUE, 0};

struct Command_Values newCommandValues = {0,0,0,0,0,0,0,0,-2};
struct Command_Values currentCommandValues = {0,0,0,0,0,0,0,0,-2};
struct Command_Values oldCommandValues = {0,0,0,0,0,0,0,0,-2};

struct Time_Values timeValues = {};

struct Running_Mode runningMode = {true, false, false, false, false,  false, false};

struct Settings settings = {5, 1,1,1, 40,10, 500,1, false, "127.0.0.1",8000,true,true, false,false, 100,800, "config","/dev/shm/command", "/dev/shm/status", "/var/log/EveryCook_Daemon.log"};

struct State state = {true,true,true, 1/*setting.ShortDelay*/, 0,false, false, true, 0, ' ',false, -1,""};

struct Daemon_Values daemon_values;

int parseParams(int argc, const char* argv[]);
void defineSignalHandler();
void handleSignal(int signum);
void initOutputFile(void);
bool checkForInput();
void doOutput();

double readTemp(struct Daemon_Values *dv);
double readPress(struct Daemon_Values *dv);
bool HeatOn(struct Daemon_Values *dv);
bool HeatOff(struct Daemon_Values *dv);
void setMotorPWM(uint16_t pwm, struct Daemon_Values *dv);
void setServoOpen(uint8_t openPercent, uint8_t steps, uint16_t stepWait, struct Daemon_Values *dv);
double readWeight(struct Daemon_Values *dv);
double readWeightSeparate(double* values, struct Daemon_Values *dv);

void SegmentDisplaySimple(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config);
void SegmentDisplayOptimized(char curSegmentDisplay, struct State *state, struct I2C_Config *i2c_config);
void blink7Segment(struct I2C_Config *i2c_config);


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
	daemon_values.newCommandValues = &newCommandValues;
	daemon_values.currentCommandValues = &currentCommandValues;
	daemon_values.oldCommandValues = &oldCommandValues;
	daemon_values.timeValues = &timeValues;
	daemon_values.runningMode = &runningMode;
	daemon_values.settings = &settings;
	daemon_values.state = &state;
	
	printf("starting EveryCook daemon...\n");
	if (settings.debug_enabled){printf("main\n");}
	
	int result=parseParams(argc, argv);
	if (result != -1){
		return result;
	}
	defineSignalHandler();
	
	initHardware();
	delay(30);
	StringClean(state.TotalUpdate, 512);
	ReadConfigurationFile();
	
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
	
	while (state.running){
		//try {
			if (settings.debug_enabled){printf("main loop...\n");}
			if (runningMode.normalMode){
				if (timeValues.lastRunTime != timeValues.runTime){
					state.timeChanged = true;
					timeValues.lastRunTime = timeValues.runTime;
				}
				bool valueChanged = checkForInput();
				
				timeValues.runTime = time(NULL); //TODO WIA CHANGE THIS
				
				if (valueChanged){
					if (settings.debug_enabled){printf("valueChanged=true\n");}
					//TODO: timeValues.lastFileChangeTime = timeValues.runTime;
					evaluateInput();
				}
				ProcessCommand();
				currentCommandValues.time = timeValues.runTime - timeValues.stepStartTime; //TODO WIA CHANGE THIS
				
				doOutput();
			} else {
				if (runningMode.calibration || runningMode.measure_noise){
					//printf("we are in calibration mode, be careful!\n Heat will turn on automatically if switch is on!\n Use switch to turn heat off.\n");	
					if (runningMode.calibration) HeatOn(&daemon_values);
					readTemp(&daemon_values);
					readPress(&daemon_values);
					readWeight(&daemon_values);
					SegmentDisplay();
				} else if (runningMode.test_7seg){
					blink7Segment(&i2c_config);
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
					return 0;
				}
			}
			if (settings.debug_enabled){printf("main loop end.\n");}
		/*} catch(exception e){
			if (settings.debug_enabled){printf("Exception: %s\n", e);}
		}*/
		delay(state.Delay);
	}
	
	SegmentDisplaySimple('S', &state, &i2c_config);
	fclose(state.logFilePointer);
	
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
	printf("  -ms, --middleware-server <ip> set middleware Server to <ip>\r\n");
	printf("  -tn, --test-noise 			measure and output random noise\r\n");
	printf("  -t7, --test-7seg 				blink 7segments to evaluate correct config\r\n");
	printf("  -ts, --test-servo [from [to]]	test servo open value, if from/to is omitted, values are from: %d, to: %d\r\n", settings.test_servo_min, settings.test_servo_max);
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
				settings.configFile = argv[i];
			}
		} else if(strcmp(argv[i], "--middleware-server") == 0 || strcmp(argv[i], "-ms") == 0){
			if (argv[i+1][0] == '-'){
				printf("Error: after --middleware-server/-ms the next param must be a ip, and must not start with -\n");
				printUsage();
				return 1;
			} else {
				++i;
				settings.middlewareHostname = argv[i];
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

void initOutputFile(void){
	if (settings.debug_enabled){printf("iniOutputFile\n");}
	//StringClean(state.TotalUpdate, 512);
	FILE *fp;
	fp = fopen(settings.statusFile, "w");
	fputs("{\"T0\":0,\"P0\":0,\"M0RPM\":0,\"M0ON\":0,\"M0OFF\":0,\"W0\":0,\"STIME\":0,\"SMODE\":0,\"SID\":-2}", fp);
	fclose(fp);
	if (settings.DeleteLogOnStart){
		state.logFilePointer = fopen(settings.logFile, "w");
		fprintf(state.logFilePointer,"Time, Temp, Press, MotorRpm, Weight, setTemp, setPress, setMotorRpm, setWeight, setMode, Mode\n");
	} else {
		state.logFilePointer = fopen(settings.logFile, "a");
	}
	
	//fclose(state.logFilePointer);
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
					error("ERROR recived dataType unknown");
				}
				parseSockInput(state.middlewareBuffer);
			} else {
				error("ERROR reading from socket");
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
	if (state.dataChanged){// || state.timeChanged){
		if (settings.debug_enabled){printf("dataChanged=true\n");}
		if (settings.useMiddleware && state.sockfd>=0){
			if (!sendToSock(state.sockfd, state.TotalUpdate, dataType)){
				error("ERROR writing to socket");
				state.sockfd = -13;
			}
		}
		if (settings.useFile) {
			writeStatus(state.TotalUpdate);
		}
		state.dataChanged = false;
		state.timeChanged = false;
	}
	writeLog();
}

void ProcessCommand(void){
	if (newCommandValues.stepId != currentCommandValues.stepId){
		if (newCommandValues.stepId < currentCommandValues.stepId){
			//-1 stop/not aus
			if(newCommandValues.stepId == -1){
				setMotorPWM(0, &daemon_values);
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
		timeValues.nextTempCheckTime = 0;
		
		if (settings.debug_enabled){printf("stepId changed, new mode is: %d\n", currentCommandValues.mode);}
		if (settings.debug_enabled || runningMode.simulationMode || settings.debug3_enabled){printf("ProcessCommand: T0: %.0f, P0: %.0f, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %.0f, STIME: %d, SMODE: %d, SID: %d\n", newCommandValues.temp, newCommandValues.press, newCommandValues.motorRpm, newCommandValues.motorOn, newCommandValues.motorOff, newCommandValues.weight, newCommandValues.time, newCommandValues.mode, newCommandValues.stepId);}
		
		OptionControl();
	}
	TempControl();
	PressControl();
	MotorControl();
	if (settings.debug3_enabled){
		printf("\n");
	}
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
		if (timeValues.runTime>=timeValues.nextTempCheckTime || !state.heatPowerStatus){
			if (HeatOff(&daemon_values)){
				delay(500);
			}
			
			oldCommandValues.temp = currentCommandValues.temp;
			currentCommandValues.temp=readTemp(&daemon_values);
			
			if (oldCommandValues.temp != currentCommandValues.temp){
				state.dataChanged = true;
			}
			
			int deltaT=newCommandValues.temp-currentCommandValues.temp;
			if (currentCommandValues.mode==MODE_HEATUP || currentCommandValues.mode==MODE_COOK) {
				if (deltaT<=0) {
					timeValues.nextTempCheckTime=timeValues.runTime+1;
					if (currentCommandValues.mode==MODE_HEATUP) {//heatup function
						timeValues.stepEndTime=timeValues.runTime-2;
						currentCommandValues.mode=MODE_HOT; //we are hot
						state.dataChanged=true;
					}
				}
				else if (deltaT <= 10) { timeValues.nextTempCheckTime=timeValues.runTime+15; HeatOn(&daemon_values);}
				else if (deltaT <= 50) { timeValues.nextTempCheckTime=timeValues.runTime+25; HeatOn(&daemon_values);}
				else {timeValues.nextTempCheckTime=timeValues.runTime+35; HeatOn(&daemon_values);} 
			}
			if (currentCommandValues.mode==MODE_HEATUP && state.heatPowerStatus) { //heatup
				timeValues.stepEndTime=timeValues.nextTempCheckTime+1;
			} else if (currentCommandValues.mode==MODE_COOLDOWN && deltaT>=0) { //cooldown function
				timeValues.stepEndTime=timeValues.runTime-2;
				currentCommandValues.mode=MODE_COLD;
				state.dataChanged=true;
			} else if (currentCommandValues.mode==MODE_COOLDOWN) {
				timeValues.nextTempCheckTime=timeValues.runTime+1;
				timeValues.stepEndTime=timeValues.nextTempCheckTime+1;
			}
		}
	} else {
		oldCommandValues.temp = currentCommandValues.temp;
		currentCommandValues.temp=readTemp(&daemon_values);
		
		if (oldCommandValues.temp != currentCommandValues.temp){
			state.dataChanged = true;
		}
	}
}

void PressControl(){
	if (currentCommandValues.mode<MIN_COOK_MODE || currentCommandValues.mode>MAX_COOK_MODE) HeatOff(&daemon_values);
	if (currentCommandValues.mode>=MIN_PRESS_MODE && currentCommandValues.mode<=MAX_PRESS_MODE) {
		if (timeValues.runTime>=timeValues.nextTempCheckTime || !state.heatPowerStatus){
			if (HeatOff(&daemon_values)){
				delay(500);
			}
			oldCommandValues.press = currentCommandValues.press;
			currentCommandValues.press = readPress(&daemon_values);
			
			if (oldCommandValues.press != currentCommandValues.press){
				state.dataChanged = true;
			}
			
			int deltaP=newCommandValues.press-currentCommandValues.press;
			if (currentCommandValues.mode==MODE_PRESSUP || currentCommandValues.mode==MODE_PRESSHOLD) {
				if (deltaP<=0) {
					timeValues.nextTempCheckTime=timeValues.runTime+1;
					if (currentCommandValues.mode==MODE_PRESSUP) {//pressup function
						timeValues.stepEndTime=timeValues.runTime-2;
						currentCommandValues.mode=MODE_PRESSURIZED; //we are pressurized
						state.dataChanged=true;
					}
				}
				else if (deltaP <= 10) { timeValues.nextTempCheckTime=timeValues.runTime+15; HeatOn(&daemon_values);}
				else if (deltaP <= 50) { timeValues.nextTempCheckTime=timeValues.runTime+25; HeatOn(&daemon_values);}
				else {timeValues.nextTempCheckTime=timeValues.runTime+35; HeatOn(&daemon_values);}
			}
			if (currentCommandValues.mode==MODE_PRESSUP && state.heatPowerStatus) { //pressure up
				timeValues.stepEndTime=timeValues.nextTempCheckTime+1;
			} else if (currentCommandValues.mode==MODE_PRESSDOWN && deltaP>=0) { //pressure down function
				timeValues.stepEndTime=timeValues.runTime-2;
				currentCommandValues.mode=MODE_PRESSURELESS;
				state.dataChanged=true;
			} else if (currentCommandValues.mode==MODE_PRESSDOWN) {
				timeValues.nextTempCheckTime=timeValues.runTime+1;
				timeValues.stepEndTime=timeValues.nextTempCheckTime+1;
			}
		}
	} else if (currentCommandValues.mode>=MIN_TEMP_MODE && currentCommandValues.mode<=MAX_TEMP_MODE) {
		if (timeValues.runTime>=timeValues.nextTempCheckTime || !state.heatPowerStatus){
			oldCommandValues.press = currentCommandValues.press;
			currentCommandValues.press = readPress(&daemon_values);
			
			if (oldCommandValues.press != currentCommandValues.press){
				state.dataChanged = true;
			}
		}
	} else {
		oldCommandValues.press = currentCommandValues.press;
		currentCommandValues.press = readPress(&daemon_values);
		
		if (oldCommandValues.press != currentCommandValues.press){
			state.dataChanged = true;
		}
	}
}

//void MotorControl(struct State state, struct Command_Values currentCommandValues, struct Command_Values newCommandValues, struct Time_Values timeValues){
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
				}
			} else {
				currentCommandValues.motorRpm = newCommandValues.motorRpm;
				timeValues.motorStartTime=timeValues.runTime;
			}
		} else {
			currentCommandValues.motorRpm = 0;
		}
	} else {
		currentCommandValues.motorRpm=0;
	}
	state.motorPwm = currentCommandValues.motorRpm * MOTOR_RPM_TO_PWM;
	setMotorPWM(state.motorPwm, &daemon_values);
}

//void ValveControl(struct State state, struct Settings settings, struct Command_Values currentCommandValues, struct Time_Values timeValues){
void ValveControl(){
	if (currentCommandValues.mode==MODE_PRESSVENT) {
		HeatOff(&daemon_values);
		setServoOpen(100, 10, 1000, &daemon_values);
		timeValues.stepEndTime=timeValues.runTime+1;
		if (currentCommandValues.press < settings.LowPress) {
			delay(2000);
			currentCommandValues.mode=MODE_PRESSURELESS;
			state.dataChanged=true;
		}
	} else {
		setServoOpen(0, 1, 0, &daemon_values);
	}
}

//void ScaleFunction(struct State state, struct Settings settings, struct Command_Values oldCommandValues, struct Command_Values currentCommandValues, struct Command_Values newCommandValues, struct Time_Values timeValues){
void ScaleFunction(){
	if (currentCommandValues.mode==MODE_SCALE || currentCommandValues.mode==MODE_WEIGHT_REACHED){
		timeValues.stepEndTime=timeValues.runTime+2;
		oldCommandValues.weight = currentCommandValues.weight;
		double sumOfForces = readWeight(&daemon_values);
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
			if (currentCommandValues.weight>=newCommandValues.weight) {//If we have reached the required mass
				if (currentCommandValues.mode != MODE_WEIGHT_REACHED){
					if(settings.BeepWeightReached > 0){
						timeValues.beepEndTime=timeValues.runTime+settings.BeepWeightReached;
					}
					if (settings.debug_enabled){printf("\tweight reached!\n");}
				} else {
					if (settings.debug_enabled){printf("\tweight reached...\n");}
				}
				if (currentCommandValues.mode != MODE_WEIGHT_REACHED){
					currentCommandValues.mode=MODE_WEIGHT_REACHED;
					state.dataChanged=true;
				}
				timeValues.remainTime=0;
			}
			if (settings.debug_enabled){printf("\t\tweight: %f / old: %f\n", currentCommandValues.weight, oldCommandValues.weight);}
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
			buzzer(1, BUZZER_PWM);
		}
	} else {
		if (state.isBuzzing || ALLWAYS_STOP_BUZZING){
			buzzer(0, BUZZER_PWM);
			state.isBuzzing = false;
		}
	}
}


/*******************PI File read/write Code**********************/
//format: {"T0":000,"P0":000,"M0RPM":0000,"M0ON":000,"M0OFF":000,"W0":0000,"STIME":000000,"SMODE":00,"SID":000}


void prepareState(char* TotalUpdate){
	StringClean(TotalUpdate, 512);
	
	sprintf(TotalUpdate, "{\"T0\":%.2f,\"P0\":%.2f,\"M0RPM\":%d,\"M0ON\":%d,\"M0OFF\":%d,\"W0\":%.0f,\"STIME\":%d,\"SMODE\":%d,\"SID\":%d}", currentCommandValues.temp, currentCommandValues.press, currentCommandValues.motorRpm, currentCommandValues.motorOn, currentCommandValues.motorOff, currentCommandValues.weight, currentCommandValues.time, currentCommandValues.mode, currentCommandValues.stepId);
	if (settings.debug_enabled){printf("prepareState: T0: %f, P0: %f, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %f, STIME: %d, SMODE: %d, SID: %d\n", currentCommandValues.temp, currentCommandValues.press, currentCommandValues.motorRpm, currentCommandValues.motorOn, currentCommandValues.motorOff, currentCommandValues.weight, currentCommandValues.time, currentCommandValues.mode, currentCommandValues.stepId);}
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


void writeLog(){
	if (settings.debug_enabled){printf("writeLog\n");}
	timeValues.nowTime = time(NULL);
	timeValues.localTime=localtime(&timeValues.nowTime);
	
	if (timeValues.runTime>=timeValues.lastLogSaveTime+settings.logSaveInterval){
		char tempString[20];
		StringClean(tempString, 20);
		strftime(tempString, 20,"%F %T",timeValues.localTime);
		//state.logFilePointer = fopen(settings.logFile, "a");
		fprintf(state.logFilePointer,"%s, %.1f, %.1f, %i, %.1f, %.1f, %.1f, %i, %.1f, %i, %i\n",tempString, currentCommandValues.temp, currentCommandValues.press, currentCommandValues.motorRpm, currentCommandValues.weight, newCommandValues.temp, newCommandValues.press, newCommandValues.motorRpm, newCommandValues.weight, newCommandValues.mode, currentCommandValues.mode);
		//open once and flush, instat open and close it always
		fflush(state.logFilePointer);
		//fclose(state.logFilePointer);
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
			while (c >= 48 && c <= 58){
				tempValue[i] = c;
				c = input[inputPos];
				++inputPos;
				i++;
			}
			i = 0;
			state.value[ptr] = StringConvertToNumber(tempValue);
			
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
			while (c >= 48 && c <= 58){
				tempValue[i] = c;
				c = fgetc(fp);
				i++;
			}
			i = 0;
			state.value[ptr] = StringConvertToNumber(tempValue);
			
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
	if (settings.debug_enabled){printf("evaluateInput: T0: %.0f, P0: %.0f, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %.0f, STIME: %d, SMODE: %d, SID: %d\n", newCommandValues.temp, newCommandValues.press, newCommandValues.motorRpm, newCommandValues.motorOn, newCommandValues.motorOff, newCommandValues.weight, newCommandValues.time, newCommandValues.mode, newCommandValues.stepId);}
	
	if(newCommandValues.temp>200) {newCommandValues.temp=200;}
	if(newCommandValues.press>200) {newCommandValues.press=200;}
	if (newCommandValues.motorOn>0 && newCommandValues.motorOn<2) newCommandValues.motorOn=2;
	if (newCommandValues.motorOff>0 && newCommandValues.motorOff<2) newCommandValues.motorOff=2;
}



/* Read the configuration
 *
 */
void ReadConfigurationFile(void){
	if (settings.debug_enabled){printf("ReadConfigurationFile...\n");}
	
	bool showReadedConfigs = settings.debug_enabled || runningMode.calibration;
	uint32_t newADCConfig[] = {0x710, 0x711, 0x712, 0x713, 0x714, 0x115};
	
	FILE *fp;
	char keyString[30];
	char valueString[100];
	uint8_t i = 0;
	uint8_t ptr = 0;
	char c;
	char c2;
	StringClean(keyString, 30);
	StringClean(valueString, 100);
	fp = fopen(settings.configFile, "r");
	if (fp != NULL){
		c = fgetc(fp);
		while (c != 255){
			if (c == '#'){
				if (showReadedConfigs){printf("\tline with # found\n");}
				while (c != '\r' && c != '\n'){
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
				while (c != '\r' && c != '\n'){
					if (c == '#'){
						//allow coment behind value
						break;
					}
					valueString[i] = c;
					c = fgetc(fp);
					++i;
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
				
				if (showReadedConfigs){printf("\tkey: '%s', value: '%s'\n", keyString, valueString);}
				
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
					
				} else if(strcmp(keyString, "Gain_LoadCellFrontLeft") == 0){
					ptr = adc_config.ADC_LoadCellFrontLeft;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_LoadCellFrontLeft: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_LoadCellFrontRight") == 0){
					ptr = adc_config.ADC_LoadCellFrontRight;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_LoadCellFrontRight: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_LoadCellBackLeft") == 0){
					ptr = adc_config.ADC_LoadCellBackLeft;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_LoadCellBackLeft: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_LoadCellBackRight") == 0){
					ptr = adc_config.ADC_LoadCellBackRight;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_LoadCellBackRight: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_Press") == 0){
					ptr = adc_config.ADC_Press;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_Press: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_Temp") == 0){
					ptr = adc_config.ADC_Temp;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | adc_config.ADC_ref<<6 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_Temp: %04X\n", newADCConfig[ptr]);} // (old: %d)
				
				} else if(strcmp(keyString, "BeepWeightReached") == 0){
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
				} else if(strcmp(keyString, "LogFile") == 0){
					//TODO: alloc new mem
					//settings.logFile = valueString;
					if (showReadedConfigs){printf("\tLogFile: %s\n", settings.logFile);} // (old: %s)
				} else if(strcmp(keyString, "CommandFile") == 0){
					//TODO: alloc new mem
					//settings.commandFile = valueString;
					if (showReadedConfigs){printf("\tCommandFile: %s\n", settings.commandFile);} // (old: %s)
				} else if(strcmp(keyString, "StatusFile") == 0){
					//TODO: alloc new mem
					//settings.statusFile = valueString;
					if (settings.debug_enabled){printf("\tStatusFile: %s\n", settings.statusFile);} // (old: %s)
				} else if(strcmp(keyString, "LowTemp") == 0){
					settings.LowTemp = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\tLowTemp: %d\n", settings.LowTemp);} // (old: %d)
				} else if(strcmp(keyString, "LowPress") == 0){
					settings.LowPress = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\tLowPress: %d\n", settings.LowPress);} // (old: %d)
				} else if(strcmp(keyString, "LongDelay") == 0){
					settings.LongDelay = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\tLongDelay: %d\n", settings.LongDelay);} // (old: %d)
				} else if(strcmp(keyString, "ShortDelay") == 0){
					settings.ShortDelay = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\tShortDelay: %d\n", settings.ShortDelay);} // (old: %d)
					
				} else if(strcmp(keyString, "logSaveInterval") == 0){
					settings.logSaveInterval = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\tlogSaveInterval: %d\n", settings.logSaveInterval);} // (old: %d)
					
				} else if(strcmp(keyString, "middlewareHostname") == 0){
					//TODO: alloc new mem
					//settings.middlewareHostname = valueString;
					if (settings.debug_enabled){printf("\tmiddlewareHostname: %s\n", settings.middlewareHostname);}
				} else if(strcmp(keyString, "middlewarePortno") == 0){
					settings.middlewarePortno = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\tmiddlewarePortno: %d\n", settings.middlewarePortno);}
					
					
				} else if(strcmp(keyString, "i2c_7seg_top") == 0){
					i2c_config.i2c_7seg_top = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_7seg_top: %d\n", i2c_config.i2c_7seg_top);}
				} else if(strcmp(keyString, "i2c_7seg_top_left") == 0){
					i2c_config.i2c_7seg_top_left = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_7seg_top_left: %d\n", i2c_config.i2c_7seg_top_left);}
				} else if(strcmp(keyString, "i2c_7seg_top_right") == 0){
					i2c_config.i2c_7seg_top_right = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_7seg_top_right: %d\n", i2c_config.i2c_7seg_top_right);}
				} else if(strcmp(keyString, "i2c_7seg_center") == 0){
					i2c_config.i2c_7seg_center = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_7seg_center: %d\n", i2c_config.i2c_7seg_center);}
				} else if(strcmp(keyString, "i2c_7seg_bottom_left") == 0){
					i2c_config.i2c_7seg_bottom_left = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_7seg_bottom_left: %d\n", i2c_config.i2c_7seg_bottom_left);}
				} else if(strcmp(keyString, "i2c_7seg_bottom_right") == 0){
					i2c_config.i2c_7seg_bottom_right = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_7seg_bottom_right: %d\n", i2c_config.i2c_7seg_bottom_right);}
				} else if(strcmp(keyString, "i2c_7seg_bottom") == 0){
					i2c_config.i2c_7seg_bottom = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_7seg_bottom: %d\n", i2c_config.i2c_7seg_bottom);}
				} else if(strcmp(keyString, "i2c_7seg_period") == 0){
					i2c_config.i2c_7seg_period = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_7seg_period: %d\n", i2c_config.i2c_7seg_period);}
					
				} else if(strcmp(keyString, "i2c_motor") == 0){
					i2c_config.i2c_motor = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_motor: %d\n", i2c_config.i2c_motor);}
				} else if(strcmp(keyString, "i2c_servo") == 0){
					i2c_config.i2c_servo = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_servo %d\n", i2c_config.i2c_servo);}
					
				} else if(strcmp(keyString, "i2c_servo_open") == 0){
					i2c_servo_values.i2c_servo_open = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_servo_open: %d\n", i2c_servo_values.i2c_servo_open);}
				} else if(strcmp(keyString, "i2c_servo_closed") == 0){
					i2c_servo_values.i2c_servo_closed = StringConvertToNumber(valueString);
					if (settings.debug_enabled){printf("\ti2c_servo_closed %d\n", i2c_servo_values.i2c_servo_closed);}
				} else {
					if (settings.debug_enabled){printf("\tkey not Found\n");}
				}
				StringClean(keyString, 30);
				StringClean(valueString, 100);
			}
			if (c != '#'){
				c = fgetc(fp);
			}
		}
	}
	
	forceCalibration.scaleFactor=(forceCalibration.Value2-forceCalibration.Value1)/((double)forceCalibration.ADC2-(double)forceCalibration.ADC1);
	forceCalibration.offset=forceCalibration.Value1 - forceCalibration.ADC1*forceCalibration.scaleFactor ;  //offset in g //for default reference force in calibration mode only
	
	pressCalibration.scaleFactor=(pressCalibration.Value2-pressCalibration.Value1)/((double)pressCalibration.ADC2-(double)pressCalibration.ADC1);
	pressCalibration.offset=pressCalibration.Value1 - pressCalibration.ADC1*pressCalibration.scaleFactor ;  //offset in pa
	
	tempCalibration.scaleFactor=(tempCalibration.Value2-tempCalibration.Value1)/((double)tempCalibration.ADC2-(double)tempCalibration.ADC1);
	tempCalibration.offset=tempCalibration.Value1 - tempCalibration.ADC1*tempCalibration.scaleFactor ;  //offset in °C
	
	setADCConfigReg(newADCConfig);
	
	if (showReadedConfigs){printf("ForceScaleFactor: %.2e, PressScaleFactor: %.2e, PressOffset: %f, TempScaleFactor: %.2e, TempOffset: %f\n", forceCalibration.scaleFactor, pressCalibration.scaleFactor, pressCalibration.offset, tempCalibration.scaleFactor, tempCalibration.offset);}
	
	fclose(fp);
	if (settings.debug_enabled){printf("done.\n");}
}
