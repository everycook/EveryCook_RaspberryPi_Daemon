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
#include "basic_functions.h"
#include "convertFunctions.h"
#include "middlewareSocket.h"
#include "daemon.h"

uint8_t ADC_LoadCellFrontLeft=0;
uint8_t ADC_LoadCellFrontRight=1;
uint8_t ADC_LoadCellBackLeft=2;
uint8_t ADC_LoadCellBackRight=3;
uint8_t ADC_Press=4;
uint8_t ADC_Temp=5;

//values for noise measurement
uint32_t MaxWeight1=0;
uint32_t MinWeight1=1000000000;
uint32_t DeltaWeight1=0;
uint32_t MaxWeight2=0;
uint32_t MinWeight2=1000000000;
uint32_t DeltaWeight2=0;
uint32_t MaxWeight3=0;
uint32_t MinWeight3=1000000000;
uint32_t DeltaWeight3=0;
uint32_t MaxWeight4=0;
uint32_t MinWeight4=1000000000;
uint32_t DeltaWeight4=0;
uint32_t MaxTemp=0;
uint32_t MinTemp=1000000000;
uint32_t DeltaTemp=0;
uint32_t MaxPress=0;
uint32_t MinPress=1000000000;
uint32_t DeltaPress=0;


void handleSignal(int signum);
void initOutputFile(void);

double readTemp();
double readPress();
void HeatOn();
void HeatOff();
void setMotorPWM(uint32_t pwm);
void setServoOpen(uint8_t open);
double readWeight();
double readWeightSeparate(double* values);


void blink7Segment();


char *configFile = "config";

char *commandFile = "/dev/shm/command";
char *statusFile = "/dev/shm/status";
char *logFile = "/var/log/EveryCook_Daemon.log";


uint32_t lastLogSave=0;
uint8_t logSaveInterval=5; //setting for logging interval in seconds


double ForceScaleFactor=0.1; //Conversion between digital Units and grams
double ForceOffset=0; //for default reference force in calibration mode only

double PressScaleFactor=0.1;
double PressOffset=1000;

double TempScaleFactor=0.1;
double TempOffset=1000;

//all these values are read from the config file. changing them here is useless
uint32_t PressADC1=0;
double PressValue1=0;
uint32_t PressADC2=0;
double PressValue2=0;
uint32_t TempADC1=0;
double TempValue1=0;
uint32_t TempADC2=0;
double TempValue2=0;
uint32_t ForceADC1=0;
double ForceValue1=0;
uint32_t ForceADC2=0;
double ForceValue2=0;
uint8_t BeepWeightReached=1;
uint8_t BeepStepEnd=1;
uint8_t DeleteLogOnStart=1;  //delete log at start saves disk space set 0 to keep log

uint8_t i2c_7seg_top=I2C_7SEG_TOP;					//SegAPin
uint8_t i2c_7seg_top_left=I2C_7SEG_TOP_LEFT;		//SegBPin
uint8_t i2c_7seg_top_right=I2C_7SEG_TOP_RIGHT;		//SegFPin
uint8_t i2c_7seg_center=I2C_7SEG_CENTER;			//SegGPin
uint8_t i2c_7seg_bottom_left=I2C_7SEG_BOTTOM_LEFT;	//SegEPin
uint8_t i2c_7seg_bottom_right=I2C_7SEG_BOTTOM_RIGHT;		//SegCPin
uint8_t i2c_7seg_bottom=I2C_7SEG_BOTTOM;			//SegDPin
uint8_t i2c_7seg_period=I2C_7SEG_PERIOD;			//SegDPPin

//Values for change 7seg display
uint32_t LowTemp = 40;
uint32_t LowPress = 40;

//delay for normal operation and scale mode
uint32_t LongDelay = 500;
uint32_t ShortDelay = 1;

//Command values
uint32_t setTemp = 0;
uint32_t setPress = 0;
uint32_t setMotorRpm = 0; //0-255 
uint32_t setMotorOn = 0;
uint32_t setMotorOff = 0;
uint32_t setWeight = 0;
uint32_t setTime = 0;
uint32_t setMode = 0;
uint32_t setStepId = -2;

double temp = 0;
double press = 0;
uint8_t motorRpm = 0; //0-255 
uint32_t motorOn = 0;
uint32_t motorOff = 0;
double weight = 0;
uint32_t elapsedTime = 0;
uint32_t mode = 0;
uint32_t stepId = -2;

uint32_t oldMode = 0;
double oldTemp = 0;
double oldPress = 0;
double oldWeight = 0;

//Processing Values
uint32_t Delay = 1;
uint32_t runTime; //we need a runtime in seconds
time_t nowTime;
struct tm *localTime;


//uint32_t stepRunTime = 0;
uint32_t stepEndTime = 0;
uint32_t stepStartTime = 0;

uint32_t middlewareConnectTime = 0;

double referenceForce = 0; //the reference to get the zero of the scale
uint8_t scaleReady = 0;

uint8_t heatPowerStatus = 0; //Induction heater power 0=off >0=on
uint32_t nextTempCheckTime=0; //when did we do the last Temperature Check???
uint32_t motorStartTime =0; //When did we start the motor?
uint32_t RemainTime=0;
uint32_t BeepEndTime=0; //when to stop the beeper
uint32_t lastBlinkTime=0; //when to stop the beeper

//Servo stuff
const uint32_t ValveOpenAngle=4000;
const uint32_t ValveClosedAngle=0;
uint32_t ValveSetValue=0;

double motorRpmToPwm = 4095.0/200.0;

//Options
bool doRememberBeep = 0;

uint8_t isBuzzing = 1; //to be sure first send a off to buzzer
const uint8_t ALLWAYS_STOP_BUZZING = 0;

uint32_t motorPwm = 0;
bool servoOpen = false;

char oldSegmentDisplay = ' ';
uint8_t blinkState = 0;

FILE *logFilePointer;

char* middlewareHostname = "10.0.0.1";
int middlewarePortno = 8000;
char* oldSendedString = "";
int sockfd = -1;
bool useMiddleware = true;
bool useFile = false;
char middlewareBuffer[256];
time_t lastFileChangeTime = 0;



char TotalUpdate[512];
uint32_t value[15];
char names[15][10];

bool dataChanged = true;
bool timeChanged = true;

char curSegmentDisplay = ' ';

bool simulationMode = false;
bool simulationModeShow7Segment = false;
uint32_t simulationUpdateTime = 0;

bool debug_enabled = false;
bool debug3_enabled = false;

bool calibration = false;
bool measure_noise = false;
bool test_7seg = false;

bool running = true;

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
	printf("  -?,  --help                   show this help\r\n");
}

int main(int argc, const char* argv[]){
	printf("starting EveryCook daemon...\n");
	if (debug_enabled){printf("main\n");}
	int i;
	//char* param;
	for (i=1; i<argc; ++i){ //param 0 is programmname
		//param = argv[i];
		if(strcmp(argv[i], "--sim") == 0 || strcmp(argv[i], "-s") == 0){
			simulationMode = true;
		} else if(strcmp(argv[i], "--sim7seg") == 0 || strcmp(argv[i], "-s7") == 0){
			simulationModeShow7Segment = true;
		} else if(strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0){
			debug_enabled = true;
		} else if(strcmp(argv[i], "--debug2") == 0 || strcmp(argv[i], "-d2") == 0){
			setDebugEnabled(true);
		} else if(strcmp(argv[i], "--debug3") == 0 || strcmp(argv[i], "-d3") == 0){
			debug3_enabled = true;
		} else if(strcmp(argv[i], "--calibrate") == 0 || strcmp(argv[i], "-c") == 0){
			calibration = true;
			printf("we are in calibration mode, be careful!\n Heat will turn on automatically if switch is on!\n Use switch to turn heat off.\n");
		} else if(strcmp(argv[i], "--random-noise") == 0 || strcmp(argv[i], "-r") == 0){
			measure_noise = true;
			printf("we measure random noise\n");
		} else if(strcmp(argv[i], "--test-noise") == 0 || strcmp(argv[i], "-tn") == 0){
			measure_noise = true;
			printf("we measure random noise\n");
		} else if(strcmp(argv[i], "--test-7seg") == 0 || strcmp(argv[i], "-t7") == 0){
			test_7seg = true;
			printf("test 7seg config\n");
		} else if(strcmp(argv[i], "--no-middleware") == 0 || strcmp(argv[i], "-nm") == 0){
			useMiddleware = false;
			useFile = true;
		} else if(strcmp(argv[i], "--middleware-and-file") == 0 || strcmp(argv[i], "-mf") == 0){
			useMiddleware = true;
			useFile = true;
		} else if(strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-cg") == 0){
			if (argv[i+1][0] == '-'){
				printf("Error: after --config/-cg the next param must be a pathname, and must not start with -\n");
				printUsage();
				return 1;
			} else {
				++i;
				configFile = argv[i];
			}
		} else if(strcmp(argv[i], "--middleware-server") == 0 || strcmp(argv[i], "-ms") == 0){
			if (argv[i+1][0] == '-'){
				printf("Error: after --middleware-server/-ms the next param must be a ip, and must not start with -\n");
				printUsage();
				return 1;
			} else {
				++i;
				middlewareHostname = argv[i];
			}
		} else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-?") == 0){
			printUsage();
			return 0;
		} else {
			printUsage();
			return 1;
		}
	}
	
	//define signal handler
	signal (SIGTERM, handleSignal); //kill -term <pid>
	signal (SIGINT, handleSignal); //Ctr+c
	signal (SIGQUIT, handleSignal); //Ctrl+???
	signal (SIGKILL, handleSignal); //kill -kill <pid> //handling not possible...
	signal (SIGHUP, handleSignal); //hang-up signal is used to report that the user's terminal is disconnected
	signal (SIGTSTP, handleSignal); //is interactive Job Control stop signal
	
	
	
	initHardware();
	delay(30);
	StringClean(TotalUpdate, 512);
	ReadConfigurationFile();
	
	initOutputFile();
	Delay = LongDelay;
	
	resetValues();
	referenceForce = ForceOffset; //for default reference Force in calibration mode
	SegmentDisplaySimple(' ');
	
	if (debug_enabled){printf("commandFile is: %s\n", commandFile);}
	if (debug_enabled){printf("statusFile is: %s\n", statusFile);}
	
	//remove commandfile so no unexpectet action will be done
	if (remove(commandFile) != 0){
		printf("Error while remove old commandfile: %s\n", commandFile);
	}
		
	runTime = time(NULL); //TODO WIA CHANGE THIS
	printf("runtime is now: %d \n",runTime);
	stepStartTime = runTime; //TODO WIA CHANGE THIS
	
	uint32_t lastRunTime = 0;
	
	const uint8_t dataType = TYPE_TEXT;
	uint8_t recivedDataType = 0;
	while (running){
		//try {
			if (debug_enabled){printf("main loop...\n");}
			//runTime = millis()/1000; //calculate runtime in seconds
			if (calibration || measure_noise){
				//printf("we are in calibration mode, be careful!\n Heat will turn on automatically if switch is on!\n Use switch to turn heat off.\n");	
				if (calibration) HeatOn();
				readTemp();
				readPress();
				readWeight();
				SegmentDisplay();
			} else if (test_7seg){
				blink7Segment();
			} else {
				if (lastRunTime != runTime){
					timeChanged = true;
					lastRunTime = runTime;
				}
				bool valueChanged = false;
				if (useMiddleware){
					if (sockfd < 0){
						if (runTime - middlewareConnectTime > 5){
							//only try to connect to middleware every 5 sec.
							middlewareConnectTime = runTime;
							sockfd = connectToMiddleware(middlewareHostname, middlewarePortno);
						}
					}
					if (sockfd>=0 && isSocketReady(sockfd)){
						//if (reciveFromSock(sockfd, middlewareBuffer) ){
						if (recivePackageFromSock(sockfd, middlewareBuffer, &recivedDataType)){
							valueChanged = true;
							if (debug_enabled || debug3_enabled){printf("recived Value(with type %d): %s\n", recivedDataType, middlewareBuffer);}
							if (recivedDataType != dataType){
								error("ERROR recived dataType unknown");
							}
							parseSockInput(middlewareBuffer);
						} else {
							error("ERROR reading from socket");
							sockfd = -14;
						}
					}
					if (sockfd<0){
						if (useFile && ReadFile()){
							valueChanged = true;
						} else {
							delay(10000);
						}
					} else if (useFile){
						time_t changeTime = get_mtime(commandFile);
						//TODO: changeTime = localtime(changeTime);
						
						if (changeTime > lastFileChangeTime){
							if (ReadFile()){
								valueChanged = true;
							}
						} else {
							if (debug_enabled || debug3_enabled){printf("commandFile to old, is:%ld, last:%ld\n",changeTime,lastFileChangeTime);}
						}
					}
				} else {
					if (useFile && ReadFile()){
						valueChanged = true;
					} else {
						delay(10000);
					}
				}
				
				runTime = time(NULL); //TODO WIA CHANGE THIS
				
				if (valueChanged){
					if (debug_enabled){printf("valueChanged=true\n");}
					//TODO: lastFileChangeTime = runTime;
					evaluateInput();
				}
				ProcessCommand();
				elapsedTime = runTime - stepStartTime; //TODO WIA CHANGE THIS
				prepareState(TotalUpdate);
			
				if (dataChanged){// || timeChanged){
					if (debug_enabled){printf("dataChanged=true\n");}
					if (useMiddleware && sockfd>=0){
						if (!sendToSock(sockfd, TotalUpdate, dataType)){
							error("ERROR writing to socket");
							sockfd = -13;
						}
					}
					if (useFile) {
						writeStatus(TotalUpdate);
					}
					dataChanged = false;
					timeChanged = false;
				}
				writeLog();
			}
			if (debug_enabled){printf("main loop end.\n");}
		/*} catch(exception e){
			if (debug_enabled){printf("Exception: %s\n", e);}
		}*/
		delay(Delay);
	}
	
	SegmentDisplaySimple('S');
	fclose(logFilePointer);
	
	return 0;
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
		running = false;
	} else {
		printf("not handled signal %d recived\n", signum);
	}
}


void initOutputFile(void){
	if (debug_enabled){printf("iniOutputFile\n");}
	//StringClean(TotalUpdate, 512);
	FILE *fp;
	fp = fopen(statusFile, "w");
	fputs("{\"T0\":0,\"P0\":0,\"M0RPM\":0,\"M0ON\":0,\"M0OFF\":0,\"W0\":0,\"STIME\":0,\"SMODE\":0,\"SID\":-2}", fp);
	fclose(fp);
	if (DeleteLogOnStart){
		logFilePointer = fopen(logFile, "w");
		fprintf(logFilePointer,"Time, Temp, Press, MotorRpm, Weight, setTemp, setPress, setMotorRpm, setWeight, setMode, Mode\n");
	} else {
		logFilePointer = fopen(logFile, "a");
	}
	
	//fclose(logFilePointer);
}

void resetValues(){
	isBuzzing = 1; //to be sure first send a off to buzzer
	oldSegmentDisplay = ' ';
	setStepId=-2;
	stepId=-2;
	dataChanged = true;
	timeChanged = true;
	lastFileChangeTime = 0;
	
	temp = 0;
	press = 0;
	weight = 0;
	elapsedTime = 0;
	mode = 0;
	
	oldMode = 0;
	oldTemp = 0;
	oldPress = 0;
	oldWeight = 0;
	
	Delay = LongDelay;
}



void ProcessCommand(void){
	if (setStepId != stepId){
		if (setStepId < stepId){
			//-1 stop/not aus
			if(setStepId == -1){
				setMotorPWM(0);
				HeatOff();
			}
			//TODO is reset/stop? or is someone try to begin new recipe before old is ended?
			if(setStepId==0){
				resetValues();
				setStepId=0;
			}
		}
		oldMode = mode;
		mode=setMode;
		stepId = setStepId;
		stepStartTime = runTime;
		dataChanged = true;
		stepEndTime = stepStartTime + setTime;
		nextTempCheckTime = 0;
		
		if (debug_enabled){printf("stepId changed, new mode is: %d\n", mode);}
		if (debug_enabled || simulationMode || debug3_enabled){printf("ProcessCommand: T0: %d, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %d, STIME: %d, SMODE: %d, SID: %d\n", setTemp, setPress, setMotorRpm, setMotorOn, setMotorOff, setWeight, setTime, setMode, setStepId);}
		
		OptionControl();
	}
	TempControl();
	PressControl();
	MotorControl();
	ValveControl();
	ScaleFunction();
	
	if (stepEndTime > runTime) {
		RemainTime=stepEndTime-runTime;
	} else if (mode<MIN_STATUS_MODE) {
		RemainTime=0;
		if (mode==MODE_COOK || mode == MODE_PRESSHOLD){
			mode=MODE_COOK_TIMEEND;
			dataChanged=true;
			if (BeepStepEnd>0){
				BeepEndTime = runTime+BeepStepEnd;
			}
		} else if (mode != MODE_STANDBY){
			mode=MODE_STANDBY;
			dataChanged=true;
			if (BeepStepEnd>0){
				BeepEndTime = runTime+BeepStepEnd;
			}
		}
	}
	SegmentDisplay();
	Beep();
}


/******************* functions to evaluate **********************/


double readTemp(){
	if (simulationMode){
		double tempValue = oldTemp;
		if (nextTempCheckTime != 0){
			int DeltaT=setTemp-oldTemp;
			if (mode==MODE_HEATUP || mode==MODE_COOK) {
				if (DeltaT<0) {
					tempValue = oldTemp-1;
				} else if (DeltaT==0) {
					tempValue = oldTemp;
				} else if (DeltaT <= 10) {
					tempValue = oldTemp+1;
				} else if (DeltaT <= 20) {
					tempValue = oldTemp+5;
				} else if (DeltaT <= 50) {
					tempValue = oldTemp+25;
				} else {
					tempValue = oldTemp+40;
				}
			} else if (mode==MODE_COOLDOWN){
				tempValue = oldTemp-1;
			}
		//} else {
			//is first call, no change yet
		}
		//if (debug_enabled){printf("readTemp, new Value is %f\n", tempValue);}
		printf("readTemp, new Value is %f\n", tempValue);
		return tempValue;
	} else {
		uint32_t tempValueInt = readADC(ADC_Temp);
		double tempValue = (double)tempValueInt * TempScaleFactor+(double)TempOffset;
		tempValue = round(tempValue);
		if (debug_enabled || calibration){printf("Temp %d dig %.0f �C | ", tempValueInt, tempValue);}
		if (measure_noise) {
			if (tempValueInt>MaxTemp) MaxTemp=tempValueInt;
			if (tempValueInt<MinTemp) MinTemp=tempValueInt;
			DeltaTemp=MaxTemp-MinTemp;
			printf("NoiseTemp %d | ", DeltaTemp);
		}
		return tempValue;
	}
}


double readPress(){
	if (simulationMode){
		double pressValue = oldPress;
		if (nextTempCheckTime != 0){
			int DeltaP=setPress-oldPress;
			if (mode==MODE_PRESSUP || mode==MODE_PRESSHOLD) {
				if (DeltaP<0) {
					--pressValue;
				} else if (DeltaP==0) {
					//Nothing //pressValue = oldPress;
				} else if (DeltaP <= 10) {
					pressValue = oldPress+2;
				} else if (DeltaP <= 50) {
					pressValue = oldPress+5;
				} else {
					pressValue = oldPress+10;
				}
			} else if (mode==MODE_PRESSDOWN){
				pressValue = oldPress-1;
			}
		//} else {
			//is first call, no change yet
		}
		//if (debug_enabled){printf("readPress, new Value is %f\n", pressValue);}
		printf("readPress, new Value is %f\n", pressValue);
		return pressValue;
	} else {
		uint32_t pressValueInt = readADC(ADC_Press);
		double pressValue = pressValueInt* PressScaleFactor+PressOffset;
		if (debug_enabled || calibration){printf("Press %d digits %.1f kPa | ", pressValueInt, pressValue);}
		if (measure_noise){
			if (pressValueInt>MaxPress) MaxPress=pressValueInt;
			if (pressValueInt<MinPress) MinPress=pressValueInt;
			DeltaPress=MaxPress-MinPress;
			printf("NoisePress %d |", DeltaPress);
		}
		return pressValue;
	}
}

//Power control functions
void HeatOn() {
	//if (debug_enabled || simulationMode){printf("HeatOn, was: %d\n", heatPowerStatus);}
	if (heatPowerStatus==0) { //if its off
		if (debug_enabled || simulationMode || calibration){printf("HeatOn status was: %d\n", heatPowerStatus);}
		if (!simulationMode){
			writeControllButtonPin(IND_KEY4, 0); //"press" the power button
			delay(500);
			writeControllButtonPin(IND_KEY4, 1);
		}
		heatPowerStatus=1; //save that we turned it on
	}
}

void HeatOff(){
	//if (debug_enabled || simulationMode){printf("HeatOff, was: %d\n", heatPowerStatus);}
	if (heatPowerStatus>0) { //if its on
		if (debug_enabled || simulationMode){printf("HeatOff\n");}
		if (!simulationMode){
			writeControllButtonPin(IND_KEY4, 0); //"press" the power button
			delay(500);
			writeControllButtonPin(IND_KEY4, 1);
		}
		heatPowerStatus=0; //save that we turned it off
	}
}

void setMotorPWM(uint32_t pwm){
	if (debug_enabled){printf("setMotorPWM, pwm: %d\n", pwm);}
	if (motorPwm != pwm){
		if (!simulationMode){
			writeI2CPin(I2C_MOTOR, pwm);
		} else {
			printf("setMotorPWM, pwm: %d\n", pwm);
		}
		motorPwm = pwm;
	}
}

void setServoOpen(bool open){
	if (debug_enabled){printf("setServoOpen, open: %d\n", open);}
	if (servoOpen != open){
		if (!simulationMode){
			if (open){
				writeI2CPin(I2C_SERVO, ValveOpenAngle);
			} else {
				writeI2CPin(I2C_SERVO, ValveClosedAngle);
			}
		} else {
			printf("setServoOpen, open: %d\n", open);
		}
		servoOpen = open;
	}
}

double readWeight(){
	if (simulationMode){
		if (!scaleReady) {
			return 0.0;
		} else {
			if (runTime <= simulationUpdateTime){
				return oldWeight;
			} else {
				int deltaW = setWeight-oldWeight;
				double weightValue = oldWeight;
				if (deltaW<0) {
					--weightValue;
				} else if (deltaW==0) {
					//Nothing
				} else if (deltaW<=10) {
					weightValue = oldWeight + 2;
				} else if (deltaW<=50) {
					weightValue = oldWeight + 5;
				} else {
					weightValue = oldWeight + 10;
				}
				if (weightValue != oldWeight){
					weightValue += referenceForce;
					printf("readWeight, new Value is %f (old:%f)\n", weightValue, oldWeight);
					//if (debug_enabled){printf("readWeight, new Value is %f\n", weightValue);}
				} else {
					weightValue += referenceForce;
				}
				simulationUpdateTime = runTime;
				return weightValue;
			}
		}
	} else {
		uint32_t weightValue1 = readADC(ADC_LoadCellFrontLeft);
		uint32_t weightValue2 = readADC(ADC_LoadCellFrontRight);
		uint32_t weightValue3 = readADC(ADC_LoadCellBackLeft);
		uint32_t weightValue4 = readADC(ADC_LoadCellBackRight);
		
		uint32_t weightValueSum = (weightValue1+weightValue2+weightValue3+weightValue4) / 4;
		
		double weightValue = (double)weightValueSum;
		weightValue *=ForceScaleFactor;
		weightValue = roundf(weightValue);
		//if (debug_enabled || calibration){printf("readWeight %d digits, %.1f grams\n", weightValueSum, weightValue);}
		if (debug_enabled || calibration){printf("Weight %d dig %.1f g / %.1f g | FL %d FR %d BL %d BR %d\n", weightValueSum, weightValue, (weightValue+referenceForce), weightValue1, weightValue2, weightValue3, weightValue4);}
		if (measure_noise){
			if (weightValue1>MaxWeight1) MaxWeight1=weightValue1;
			if (weightValue1<MinWeight1) MinWeight1=weightValue1;
			DeltaWeight1=MaxWeight1-MinWeight1;
			printf("NoiseWeightFL %d | ", DeltaWeight1);
			
			if (weightValue2>MaxWeight2) MaxWeight2=weightValue2;
			if (weightValue2<MinWeight2) MinWeight2=weightValue2;
			DeltaWeight2=MaxWeight2-MinWeight2;
			printf("NoiseWeightFR %d | ", DeltaWeight2);
			
			if (weightValue3>MaxWeight3) MaxWeight3=weightValue3;
			if (weightValue3<MinWeight3) MinWeight3=weightValue3;
			DeltaWeight3=MaxWeight3-MinWeight3;
			printf("NoiseWeightBL %d | ", DeltaWeight3);
			
			if (weightValue4>MaxWeight4) MaxWeight4=weightValue4;
			if (weightValue4<MinWeight4) MinWeight4=weightValue4;
			DeltaWeight4=MaxWeight4-MinWeight4;
			printf("NoiseWeightBR %d\n", DeltaWeight4);
		}
		return weightValue;
	}
}

double readWeightSeparate(double* values){
	if (simulationMode){
		if (!scaleReady) {
			return 0.0;
		} else {
			int deltaW = setWeight-oldWeight;
			double weightValue = oldWeight;
			if (deltaW<0) {
				--weightValue;
			} else if (deltaW==0) {
				//Nothing
			} else if (deltaW<=10) {
				weightValue = oldWeight + 2;
			} else if (deltaW<=50) {
				weightValue = oldWeight + 5;
			} else {
				weightValue = oldWeight + 10;
			}
			weightValue += referenceForce;
			if (debug_enabled){printf("readWeight, new Value is %f\n", weightValue);}
			
			if (values != NULL){
				values[0] = weightValue;
				values[1] = weightValue;
				values[2] = weightValue;
				values[3] = weightValue;
			}
			return weightValue;
		}
	} else {
		uint32_t weightValue1 = readADC(ADC_LoadCellFrontLeft);
		uint32_t weightValue2 = readADC(ADC_LoadCellFrontRight);
		uint32_t weightValue3 = readADC(ADC_LoadCellBackLeft);
		uint32_t weightValue4 = readADC(ADC_LoadCellBackRight);
		
		if (values != NULL){
			values[0] = (double)weightValue1 * ForceScaleFactor;
			values[1] = (double)weightValue2 * ForceScaleFactor;
			values[2] = (double)weightValue3 * ForceScaleFactor;
			values[3] = (double)weightValue4 * ForceScaleFactor;
		}
		
		uint32_t weightValueSum = (weightValue1+weightValue2+weightValue3+weightValue4) / 4;
		
		double weightValue = (double)weightValueSum;
		weightValue *=ForceScaleFactor;
		weightValue = roundf(weightValue);
		if (debug_enabled){
			if (values != NULL){
				printf("readWeight, new Value is %d / %f (%d, %d, %d, %d / %f, %f, %f, %f)\n", weightValueSum, weightValue, weightValue1, weightValue2, weightValue3, weightValue4, values[0], values[1], values[2], values[3]);
			} else {
				printf("readWeight, new Value is %d / %f (%d, %d, %d, %d)\n", weightValueSum, weightValue, weightValue1, weightValue2, weightValue3, weightValue4);
			}
		}
		return weightValue;
	}
}

/******************* processing functions **********************/
void OptionControl(){
  if (mode>=MODE_OPTIONS_BEGIN){
    if (mode==MODE_OPTION_REMEMBER_BEEP_ON){
      doRememberBeep=true;
    } else if (mode==MODE_OPTION_REMEMBER_BEEP_OFF){
      doRememberBeep=false;
    } else if (mode==MODE_OPTION_7SEGMENT_BLINK){
		SegmentDisplaySimple(' ');
		blink7Segment();
		blink7Segment();
		blink7Segment();
		blink7Segment();
	}
    mode=MODE_STANDBY;
	dataChanged=true;
  }
}

void TempControl(){
	if (mode<MIN_COOK_MODE || mode>MAX_COOK_MODE) HeatOff();
	if (mode>=MIN_TEMP_MODE && mode<=MAX_TEMP_MODE) {
		if (runTime>=nextTempCheckTime || heatPowerStatus==0){
			HeatOff();
			delay(500);
			oldTemp = temp;
			uint32_t TempValue = readTemp();
			temp=TempValue;
			
			if (oldTemp != temp){
				dataChanged = true;
			}
			
			int DeltaT=setTemp-temp;
			if (mode==MODE_HEATUP || mode==MODE_COOK) {
				if (DeltaT<=0) {
					nextTempCheckTime=runTime+1;
					if (mode==MODE_HEATUP) {//heatup function
						stepEndTime=runTime-2;
						mode=MODE_HOT; //we are hot
						dataChanged=true;
					}
				}
				else if (DeltaT <= 10) { nextTempCheckTime=runTime+15; HeatOn(); }
				else if (DeltaT <= 50) { nextTempCheckTime=runTime+25; HeatOn();}
				else {nextTempCheckTime=runTime+35; HeatOn();} 
			}
			if (mode==MODE_HEATUP && heatPowerStatus==1) { //heatup
				stepEndTime=nextTempCheckTime+1;
			} else if (mode==MODE_COOLDOWN && DeltaT>=0) { //cooldown function
				stepEndTime=runTime-2;
				mode=MODE_COLD;
				dataChanged=true;
			} else if (mode==MODE_COOLDOWN) {
				nextTempCheckTime=runTime+1;
				stepEndTime=nextTempCheckTime+1;
			}
		}
	} else {
		oldTemp = temp;
		uint32_t TempValue = readTemp();
		temp=TempValue;
		
		if (oldTemp != temp){
			dataChanged = true;
		}
	}
}

void PressControl(){
	if (mode<MIN_COOK_MODE || mode>MAX_COOK_MODE) HeatOff();
	if (mode>=MIN_PRESS_MODE && mode<=MAX_PRESS_MODE) {
		if (runTime>=nextTempCheckTime || heatPowerStatus==0){
			HeatOff();
			delay(100);
			oldPress = press;
			uint32_t pressValue = readPress();
			press=pressValue;
			
			if (oldPress != press){
				dataChanged = true;
			}
			
			int DeltaP=setPress-press;
			if (mode==MODE_PRESSUP || mode==MODE_PRESSHOLD) {
				if (DeltaP<=0) {
					nextTempCheckTime=runTime+1;
					if (mode==MODE_PRESSUP) {//pressup function
						stepEndTime=runTime-2;
						mode=MODE_PRESSURIZED; //we are pressurized
						dataChanged=true;
					}
				}
				else if (DeltaP <= 10) { nextTempCheckTime=runTime+5; HeatOn(); }
				else if (DeltaP <= 50) { nextTempCheckTime=runTime+10; HeatOn();}
				else {nextTempCheckTime=runTime+20; HeatOn();}
			}
			if (mode==MODE_PRESSUP && heatPowerStatus==1) { //pressure up
				stepEndTime=nextTempCheckTime+1;
			} else if (mode==MODE_PRESSDOWN && DeltaP>=0) { //pressure down function
				stepEndTime=runTime-2;
				mode=MODE_PRESSURELESS;
				dataChanged=true;
			} else if (mode==MODE_PRESSDOWN) {
				nextTempCheckTime=runTime+1;
				stepEndTime=nextTempCheckTime+1;
			}
		}
	} else if (mode>=MIN_TEMP_MODE && mode<=MAX_TEMP_MODE) {
		if (runTime>=nextTempCheckTime || heatPowerStatus==0){
			oldPress = press;
			uint32_t pressValue = readPress();
			press=pressValue;
			if (oldPress != press){
				dataChanged = true;
			}
		}
	} else {
		oldPress = press;
		uint32_t pressValue = readPress();
		press=pressValue;
		
		if (oldPress != press){
			dataChanged = true;
		}
	}
}

void MotorControl(){
	if (mode==MODE_CUT || mode>=MIN_TEMP_MODE){
		if (mode==MODE_CUT) stepEndTime=runTime+1;
		if (setMotorRpm > 0 && mode<=MAX_STATUS_MODE) {
			if  (setMotorOn > 0 && setMotorOff > 0) {
				if (runTime >= motorStartTime+setMotorOn && runTime < motorStartTime+setMotorOn+setMotorOff) { 
					motorRpm = 0; 
				} else if (motorRpm==0){
					motorRpm = setMotorRpm;
					motorStartTime=runTime;
				}
			} else {
				motorRpm = setMotorRpm;
				motorStartTime=runTime;
			}
		} else {
			motorRpm = 0;
		}
	} else {
		motorRpm=0;
	}
	uint16_t motorPwm = motorRpm * motorRpmToPwm;
	setMotorPWM(motorPwm);
}

void ValveControl(){
	if (mode==MODE_PRESSVENT) {
		HeatOff();
		setServoOpen(1);
		stepEndTime=runTime+1;
		if (press < LowPress) {
			delay(2000);
			mode=MODE_PRESSURELESS;
			dataChanged=true;
		}
	} else {
		setServoOpen(0);
	}
}


void ScaleFunction() {
	if (mode==MODE_SCALE || mode==MODE_WEIGHT_REACHED){
		stepEndTime=runTime+2;
		oldWeight = weight;
		double SumOfForces = readWeight();
		if (debug_enabled){printf("ScaleFunction\n");}
		
		if (!scaleReady) { //we are not ready for weighting
			referenceForce=SumOfForces;
			oldWeight=SumOfForces;
			scaleReady=true;
			if (Delay == ShortDelay){
				Delay=LongDelay;
			}
			if (debug_enabled){printf("\tscaleReady\n");}
		} else { //we have a reference and are ready
			Delay=ShortDelay;
			weight=(SumOfForces-referenceForce);
			
			if (oldWeight != weight){
				dataChanged = true;
			}
			if (weight>=setWeight) {//If we have reached the required mass
				if (mode != MODE_WEIGHT_REACHED){
					if(BeepWeightReached > 0){
						BeepEndTime=runTime+BeepWeightReached;
					}
					if (debug_enabled){printf("\tweight reached!\n");}
				} else {
					if (debug_enabled){printf("\tweight reached...\n");}
				}
				if (mode != MODE_WEIGHT_REACHED){
					mode=MODE_WEIGHT_REACHED;
					dataChanged=true;
				}
				RemainTime=0;
			}
			if (debug_enabled){printf("\t\tweight: %f / old: %f\n", weight, oldWeight);}
		}
	} else if (Delay == ShortDelay || scaleReady){
		scaleReady=false;
		referenceForce=0;
		Delay=LongDelay;
		weight = 0.0;
	}
}

void SegmentDisplay(){
	char curSegmentDisplay = ' ';
	if (press>=LowPress){
		curSegmentDisplay = 'P';
	} else if (temp>=LowTemp){
		curSegmentDisplay = 'H';
	} else {
		curSegmentDisplay = '0';
	}
	if (curSegmentDisplay != oldSegmentDisplay){
		if (debug_enabled || simulationMode){printf("SegmentDisplay: '%c'\n", curSegmentDisplay);}
		if (!simulationMode || simulationModeShow7Segment){
			SegmentDisplaySimple(curSegmentDisplay);
			//SegmentDisplayOptimized(curSegmentDisplay);
		} else {
			oldSegmentDisplay = curSegmentDisplay;
		}
	}
	if (!simulationMode || simulationModeShow7Segment){
		if (lastBlinkTime != runTime){
			lastBlinkTime = runTime;
			//togglePin(i2c_7seg_period);
			if (blinkState){
				writeI2CPin(i2c_7seg_period,I2C_7SEG_OFF);
				blinkState = false;
				if (debug_enabled || (simulationMode && !simulationModeShow7Segment)){printf("SegmentDisplay: blink off\n");}
			} else {
				blinkState = true;
				writeI2CPin(i2c_7seg_period,I2C_7SEG_ON);
				if (debug_enabled || (simulationMode && !simulationModeShow7Segment)){printf("SegmentDisplay: blink on\n");}
			}
		}
	}
}

void SegmentDisplaySimple(char curSegmentDisplay){
	if (curSegmentDisplay == 'P'){
		oldSegmentDisplay = 'P';
		writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
	} else if (curSegmentDisplay == 'H'){
		oldSegmentDisplay = 'H';
		writeI2CPin(i2c_7seg_top,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
	} else if (curSegmentDisplay == '0'){
		oldSegmentDisplay = '0';
		writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_center,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_bottom,I2C_7SEG_ON);
	} else if (curSegmentDisplay == 'S'){
		oldSegmentDisplay = 'S';
		writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_bottom,I2C_7SEG_ON);
	} else if (curSegmentDisplay == 'E'){
		oldSegmentDisplay = 'E';
		writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
		writeI2CPin(i2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom,I2C_7SEG_ON);
	} else {
		oldSegmentDisplay = ' ';
		writeI2CPin(i2c_7seg_top,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_top_left,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_center,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
	}
}

void SegmentDisplayOptimized(char curSegmentDisplay){
	if (curSegmentDisplay == 'P'){
		if (curSegmentDisplay != oldSegmentDisplay){
			if (oldSegmentDisplay == 'H'){
				writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_OFF);
				//writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (oldSegmentDisplay == '0'){
				//writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_OFF);
				writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_OFF);
				writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
			} else {
				SegmentDisplaySimple(curSegmentDisplay);
			}
			oldSegmentDisplay = curSegmentDisplay;
		}
	} else if (curSegmentDisplay == 'H'){
		if (curSegmentDisplay != oldSegmentDisplay){
			if (oldSegmentDisplay == 'P'){
				writeI2CPin(i2c_7seg_top,I2C_7SEG_OFF);
				//writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (oldSegmentDisplay == '0'){
				writeI2CPin(i2c_7seg_top,I2C_7SEG_OFF);
				//writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
			} else if (oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(i2c_7seg_top,I2C_7SEG_OFF);
				writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
			} else {
				SegmentDisplaySimple(curSegmentDisplay);
			}
			oldSegmentDisplay = curSegmentDisplay;
		}
	} else if (curSegmentDisplay == '0'){
		if (curSegmentDisplay != oldSegmentDisplay){
			if (oldSegmentDisplay == 'H'){
				writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_center,I2C_7SEG_OFF);
				//writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom,I2C_7SEG_ON);
			} else if (oldSegmentDisplay == 'P'){
				//writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_center,I2C_7SEG_OFF);
				//writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
				//writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom,I2C_7SEG_ON);
			} else if (oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_center,I2C_7SEG_OFF);
				writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_ON);
				writeI2CPin(i2c_7seg_bottom,I2C_7SEG_ON);
			} else {
				SegmentDisplaySimple(curSegmentDisplay);
			}
			oldSegmentDisplay = curSegmentDisplay;
		}
	} else {
		SegmentDisplaySimple(curSegmentDisplay);
		/*
		//Empty all
		writeI2CPin(i2c_7seg_top,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_top_left,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_center,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_top_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_OFF);
		writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
		oldSegmentDisplay = ' ';
		*/
	}
}

void Beep(){
	if(doRememberBeep==1){
		if (runTime>stepEndTime){
			if (mode>=MIN_STATUS_MODE && mode<=MAX_STATUS_MODE){
				int toLateTime = runTime-stepEndTime;
				if (toLateTime==1){
					BeepEndTime=runTime+1;
				} else if (toLateTime==5){
					BeepEndTime=runTime+1;
				} else if (toLateTime==30){
					BeepEndTime=runTime+1;
				} else if (toLateTime==60){
					BeepEndTime=runTime+1;
				} else if (toLateTime>0 && toLateTime%300==0){ //each 5 min
					BeepEndTime=runTime+5;
				}
			}
		}
	}
	if (runTime<BeepEndTime){
		if (!isBuzzing){
			isBuzzing = 1;
			buzzer(1, BUZZER_PWM);
		}
	} else {
		if (isBuzzing || ALLWAYS_STOP_BUZZING){
			buzzer(0, BUZZER_PWM);
			isBuzzing = 0;
		}
	}
}


/*******************PI File read/write Code**********************/
//format: {"T0":000,"P0":000,"M0RPM":0000,"M0ON":000,"M0OFF":000,"W0":0000,"STIME":000000,"SMODE":00,"SID":000}


void prepareState(char* TotalUpdate){
	StringClean(TotalUpdate, 512);
	
	sprintf(TotalUpdate, "{\"T0\":%.2f,\"P0\":%.2f,\"M0RPM\":%d,\"M0ON\":%d,\"M0OFF\":%d,\"W0\":%.0f,\"STIME\":%d,\"SMODE\":%d,\"SID\":%d}", temp, press, motorRpm, motorOn, motorOff, weight, elapsedTime, mode, stepId);
	if (debug_enabled){printf("prepareState: T0: %f, P0: %f, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %f, STIME: %d, SMODE: %d, SID: %d\n", temp, press, motorRpm, motorOn, motorOff, weight, elapsedTime, mode, stepId);}
}

void writeStatus(char* data){
	if (debug_enabled){printf("WriteStatus\n");}
	
	FILE *fp;
	fp = fopen(statusFile, "w");
	fputs(data, fp);
	fclose(fp);
	
	if (debug_enabled){printf("WriteStatus: after write\n");}
}


void writeLog(){
	if (debug_enabled){printf("writeLog\n");}
	nowTime = time(NULL);
	localTime=localtime(&nowTime);
	
	if (runTime>=lastLogSave+logSaveInterval){
		char tempString[20];
		StringClean(tempString, 20);
		strftime(tempString, 20,"%F %T",localTime);
		//logFilePointer = fopen(logFile, "a");
		fprintf(logFilePointer,"%s, %.1f, %.1f, %i, %.1f,%i, %i, %i, %i, %i, %i\n",tempString, temp, press, motorRpm, weight, setTemp, setPress, setMotorRpm, setWeight, setMode, mode );
		//open once and flush, instat open and close it always
		fflush(logFilePointer);
		//fclose(logFilePointer);
		lastLogSave=runTime;
	}
	if (debug_enabled){printf("writeLog: after write\n");}
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
			value[ptr] = StringConvertToNumber(tempValue);
			
			//names[ptr] = tempName;
			int32_t j = 0;
			for (; j < 10; ++j){
				names[ptr][j] = tempName[j];
			}
			
			if (debug_enabled){printf("found %s with value %d\r\n", names[ptr], value[ptr]);}
			StringClean(tempName, 10);
			StringClean(tempValue, 10);
			++ptr;
		}
		c = input[inputPos];
		++inputPos;
	}
}

bool ReadFile(){
	if (debug_enabled){printf("ReadFile\n");}
	FILE *fp;
	char tempName[10];
	char tempValue[10];
	uint8_t i = 0;
	uint8_t ptr = 0;
	char c;
	
	StringClean(tempName, 10);
	StringClean(tempValue, 10);
	fp = fopen(commandFile, "r");
	if (fp == NULL){
		printf("could not open file %s\n", commandFile);
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
			value[ptr] = StringConvertToNumber(tempValue);
			
			//names[ptr] = tempName;
			int32_t j = 0;
			for (; j < 10; ++j){
				names[ptr][j] = tempName[j];
			}
			
			if (debug_enabled){printf("found %s with value %d\r\n", names[ptr], value[ptr]);}
			
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
		//if (debug_enabled){printf("check %s with value %d\r\n", names[i], value[i]);}
		if(strcmp(names[i], "T0") == 0){
			setTemp = value[i];
		} else if(strcmp(names[i], "P0") == 0){
			setPress = value[i];
		} else if(strcmp(names[i], "M0RPM") == 0){
			setMotorRpm = value[i];
		} else if(strcmp(names[i], "M0ON") == 0){
			setMotorOn = value[i];
		} else if(strcmp(names[i], "M0OFF") == 0){
			setMotorOff = value[i];
		} else if(strcmp(names[i], "W0") == 0){
			setWeight = value[i];
		} else if(strcmp(names[i], "STIME") == 0){
			setTime = value[i];
		} else if(strcmp(names[i], "SMODE") == 0){
			setMode = value[i];
		} else if(strcmp(names[i], "SID") == 0){
			setStepId = value[i];
		}
	}
	if (debug_enabled){printf("evaluateInput: T0: %d, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %d, STIME: %d, SMODE: %d, SID: %d\n", setTemp, setPress, setMotorRpm, setMotorOn, setMotorOff, setWeight, setTime, setMode, setStepId);}
	
	if(setTemp>200) {setTemp=200;}
	if(setPress>200) {setPress=200;}
	if (setMotorOn>0 && setMotorOn<2) setMotorOn=2;
	if (setMotorOff>0 && setMotorOff<2) setMotorOff=2;
}



/* Read the configuration
 *
 */
void ReadConfigurationFile(void){
	if (debug_enabled){printf("ReadConfigurationFile...\n");}
	
	bool showReadedConfigs = debug_enabled || calibration;
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
	fp = fopen(configFile, "r");
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
					ForceADC1 = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tForceADC1: %d\n", ForceADC1);} // (old: %d)
				} else if(strcmp(keyString, "ForceValue1") == 0){
					ForceValue1 = StringConvertToDouble(valueString);
					if (showReadedConfigs){printf("\tForceValue1: %f\n", ForceValue1);} // (old: %d)
				} else if(strcmp(keyString, "ForceADC2") == 0){
					ForceADC2 = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tForceADC2: %d\n", ForceADC2);} // (old: %d)
				} else if(strcmp(keyString, "ForceValue2") == 0){
					ForceValue2 = StringConvertToDouble(valueString);
					if (showReadedConfigs){printf("\tForceValue2: %f\n", ForceValue2);} // (old: %d)
				//pressure
				} else if(strcmp(keyString, "PressADC1") == 0){
					PressADC1 = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tPressADC1: %d\n", PressADC1);} // (old: %d)
				} else if(strcmp(keyString, "PressValue1") == 0){
					PressValue1 = StringConvertToDouble(valueString);
					if (showReadedConfigs){printf("\tPressValue1: %f\n", PressValue1);} // (old: %d)
				} else if(strcmp(keyString, "PressADC2") == 0){
					PressADC2 = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tPressADC2: %d\n", PressADC2);} // (old: %d)
				} else if(strcmp(keyString, "PressValue2") == 0){
					PressValue2 = StringConvertToDouble(valueString);
					if (showReadedConfigs){printf("\tPressValue2: %f\n", PressValue2);} // (old: %d)
				//temperature
				} else if(strcmp(keyString, "TempADC1") == 0){
					TempADC1 = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tTempADC1: %d\n", TempADC1);} // (old: %d)
				} else if(strcmp(keyString, "TempValue1") == 0){
					TempValue1 = StringConvertToDouble(valueString);
					if (showReadedConfigs){printf("\tTempValue1: %f\n", TempValue1);} // (old: %d)
				} else if(strcmp(keyString, "TempADC2") == 0){
					TempADC2 = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tTempADC2: %d\n", TempADC2);} // (old: %d)
				} else if(strcmp(keyString, "TempValue2") == 0){
					TempValue2 = StringConvertToDouble(valueString);
					if (showReadedConfigs){printf("\tTempValue2: %f\n", TempValue2);} // (old: %d)
				
				//adc channels
				} else if(strcmp(keyString, "ADC_LoadCellFrontLeft") == 0){
					ADC_LoadCellFrontLeft = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tADC_LoadCellFrontLeft: %d\n", ADC_LoadCellFrontLeft);}

				} else if(strcmp(keyString, "ADC_LoadCellFrontRight") == 0){
					ADC_LoadCellFrontRight = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tLADC_oadCellFrontRight: %d\n", ADC_LoadCellFrontRight);}

				} else if(strcmp(keyString, "ADC_LoadCellBackLeft") == 0){
					ADC_LoadCellBackLeft = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tADC_LoadCellBackLeft: %d\n", ADC_LoadCellBackLeft);}

				} else if(strcmp(keyString, "ADC_LoadCellBackRight") == 0){
					ADC_LoadCellBackRight = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tADC_LoadCellBackRight: %d\n", ADC_LoadCellBackRight);}

				} else if(strcmp(keyString, "ADC_Press") == 0){
					ADC_Press = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tADC_Press: %d\n", ADC_Press);}

				} else if(strcmp(keyString, "ADC_Temp") == 0){
					ADC_Temp = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tADC_Temp: %d\n", ADC_Temp);}
					
				} else if(strcmp(keyString, "Gain_LoadCellFrontLeft") == 0){
					ptr = ADC_LoadCellFrontLeft;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | 1<<7 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_LoadCellFrontLeft: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_LoadCellFrontRight") == 0){
					ptr = ADC_LoadCellFrontRight;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | 1<<7 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_LoadCellFrontRight: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_LoadCellBackLeft") == 0){
					ptr = ADC_LoadCellBackLeft;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | 1<<7 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_LoadCellBackLeft: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_LoadCellBackRight") == 0){
					ptr = ADC_LoadCellBackRight;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | 1<<7 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_LoadCellBackRight: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_Press") == 0){
					ptr = ADC_Press;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | 1<<7 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_Press: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_Temp") == 0){
					ptr = ADC_Temp;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<8 | 1<<7 | 1<<4 | ptr;
					if (showReadedConfigs){printf("\tGain_Temp: %04X\n", newADCConfig[ptr]);} // (old: %d)
				
				} else if(strcmp(keyString, "BeepWeightReached") == 0){
					BeepWeightReached = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tBeepWeightReached: %d\n", BeepWeightReached);} // (old: %d)
				} else if(strcmp(keyString, "BeepStepEnd") == 0){
					BeepStepEnd = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tBeepStepEnd: %d\n", BeepStepEnd);} // (old: %d)
				} else if(strcmp(keyString, "doRememberBeep") == 0){
					doRememberBeep = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tdoRememberBeep: %d\n", doRememberBeep);} // (old: %d)
				} else if(strcmp(keyString, "DeleteLogOnStart") == 0){
					DeleteLogOnStart = StringConvertToNumber(valueString);
					if (showReadedConfigs){printf("\tDeleteLogOnStart: %d\n", DeleteLogOnStart);} // (old: %d)
				} else if(strcmp(keyString, "LogFile") == 0){
					//TODO: alloc new mem
					//logFile = valueString;
					if (showReadedConfigs){printf("\tLogFile: %s\n", logFile);} // (old: %s)
				} else if(strcmp(keyString, "CommandFile") == 0){
					//TODO: alloc new mem
					//commandFile = valueString;
					if (showReadedConfigs){printf("\tCommandFile: %s\n", commandFile);} // (old: %s)
				} else if(strcmp(keyString, "StatusFile") == 0){
					//TODO: alloc new mem
					//statusFile = valueString;
					if (debug_enabled){printf("\tStatusFile: %s\n", statusFile);} // (old: %s)
				} else if(strcmp(keyString, "LowTemp") == 0){
					LowTemp = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tLowTemp: %d\n", LowTemp);} // (old: %d)
				} else if(strcmp(keyString, "LowPress") == 0){
					LowPress = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tLowPress: %d\n", LowPress);} // (old: %d)
				} else if(strcmp(keyString, "LongDelay") == 0){
					LongDelay = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tLongDelay: %d\n", LongDelay);} // (old: %d)
				} else if(strcmp(keyString, "ShortDelay") == 0){
					ShortDelay = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tShortDelay: %d\n", ShortDelay);} // (old: %d)
					
				} else if(strcmp(keyString, "logSaveInterval") == 0){
					logSaveInterval = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tlogSaveInterval: %d\n", logSaveInterval);} // (old: %d)
					
				} else if(strcmp(keyString, "middlewareHostname") == 0){
					//TODO: alloc new mem
					//middlewareHostname = valueString;
					if (debug_enabled){printf("\tmiddlewareHostname: %s\n", middlewareHostname);}
				} else if(strcmp(keyString, "middlewarePortno") == 0){
					middlewarePortno = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tmiddlewarePortno: %d\n", middlewarePortno);}
					
					
				} else if(strcmp(keyString, "i2c_7seg_top") == 0){
					i2c_7seg_top = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\ti2c_7seg_top: %d\n", i2c_7seg_top);}
				} else if(strcmp(keyString, "i2c_7seg_top_left") == 0){
					i2c_7seg_top_left = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\ti2c_7seg_top_left: %d\n", i2c_7seg_top_left);}
				} else if(strcmp(keyString, "i2c_7seg_top_right") == 0){
					i2c_7seg_top_right = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\ti2c_7seg_top_right: %d\n", i2c_7seg_top_right);}
				} else if(strcmp(keyString, "i2c_7seg_center") == 0){
					i2c_7seg_center = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\ti2c_7seg_center: %d\n", i2c_7seg_center);}
				} else if(strcmp(keyString, "i2c_7seg_bottom_left") == 0){
					i2c_7seg_bottom_left = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\ti2c_7seg_bottom_left: %d\n", i2c_7seg_bottom_left);}
				} else if(strcmp(keyString, "i2c_7seg_bottom_right") == 0){
					i2c_7seg_bottom_right = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\ti2c_7seg_bottom_right: %d\n", i2c_7seg_bottom_right);}
				} else if(strcmp(keyString, "i2c_7seg_bottom") == 0){
					i2c_7seg_bottom = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\ti2c_7seg_bottom: %d\n", i2c_7seg_bottom);}
				} else if(strcmp(keyString, "i2c_7seg_period") == 0){
					i2c_7seg_period = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\ti2c_7seg_period: %d\n", i2c_7seg_period);}
					
				} else {
					if (debug_enabled){printf("\tkey not Found\n");}
				}
				StringClean(keyString, 30);
				StringClean(valueString, 100);
			}
			if (c != '#'){
				c = fgetc(fp);
			}
		}
	}
	ForceScaleFactor=(ForceValue2-ForceValue1)/((double)ForceADC2-(double)ForceADC1);
	ForceOffset=ForceValue1 - ForceADC1*ForceScaleFactor ;  //offset in g //for default reference force in calibration mode only
	
	PressScaleFactor=(PressValue2-PressValue1)/((double)PressADC2-(double)PressADC1);
	PressOffset=PressValue1 - PressADC1*PressScaleFactor ;  //offset in pa
	
	TempScaleFactor=(TempValue2-TempValue1)/((double)TempADC2-(double)TempADC1);
	TempOffset=TempValue1 - TempADC1*TempScaleFactor ;  //offset in �C
	
	setADCConfigReg(newADCConfig);
	
	if (showReadedConfigs){printf("ForceScaleFactor: %.2e, PressScaleFactor: %.2e, PressOffset: %f, TempScaleFactor: %.2e, TempOffset: %f\n", ForceScaleFactor, PressScaleFactor, PressOffset, TempScaleFactor, TempOffset);}
	
	fclose(fp);
	if (debug_enabled){printf("done.\n");}
}


void blink7Segment(){
	//mitte
	printf("center: %d\n", i2c_7seg_center);
	writeI2CPin(i2c_7seg_center,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_7seg_center,I2C_7SEG_OFF);
	
	//links
	printf("top left: %d\n", i2c_7seg_top_left);
	writeI2CPin(i2c_7seg_top_left,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_7seg_top_left,I2C_7SEG_OFF);
	
	//oben
	printf("top: %d\n", i2c_7seg_top);
	writeI2CPin(i2c_7seg_top,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_7seg_top,I2C_7SEG_OFF);
	
	
	//rechts
	printf("top right: %d\n", i2c_7seg_top_right);
	writeI2CPin(i2c_7seg_top_right,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_7seg_top_right,I2C_7SEG_OFF);
	
	//ulinks
	printf("bottom left: %d\n", i2c_7seg_bottom_left);
	writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_7seg_bottom_left,I2C_7SEG_OFF);
	
	//unten
	printf("bottom: %d\n", i2c_7seg_bottom);
	writeI2CPin(i2c_7seg_bottom,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_7seg_bottom,I2C_7SEG_OFF);
	
	//urechts
	printf("bottom right: %d\n", i2c_7seg_bottom_right);
	writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_7seg_bottom_right,I2C_7SEG_OFF);
	
	//punkt
	printf("period: %d\n", i2c_7seg_period);
	writeI2CPin(i2c_7seg_period,I2C_7SEG_ON);
	delay(5000);
	writeI2CPin(i2c_7seg_period,I2C_7SEG_OFF);
	delay(5000);
}
