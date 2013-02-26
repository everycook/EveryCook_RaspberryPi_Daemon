//TODO_WIA we have full C support, not the minimal C we had in MC. So we have for example string.h (see man string) to handle strings and stuff
// MODE could be as text in the JSON. costs a few bytes but gives lots of readability
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>

#include <softPwm.h>
#include <wiringPi.h>


#include <string.h>

#include "basic_functions.h"
#include "firmware.h"



uint8_t ADC_LoadCellFrontLeft=0;
uint8_t ADC_LoadCellFrontRight=1;
uint8_t ADC_LoadCellBackLeft=2;
uint8_t ADC_LoadCellBackRight=3;
uint8_t ADC_Press=4;
uint8_t ADC_Temp=5;

//Other functions
void NumberConvertToString(uint32_t num, char *str);
void StringClean(char *str, uint32_t len);
void StringUnion(char *fristString, char *secondString);
uint32_t StringConvertToNumber(char *str);
float StringConvertToFloat(char *str);
int POWNTimes(uint32_t num, uint8_t n);




void initOutputFile(void);
float readTemp();
float readPress();
void HeatOn();
void HeatOff();
void setMotorPWM(uint32_t pwm);
void setServoOpen(uint8_t open);
float readWeight();
float readWeightSeparate(float* values);


void blink7Segment();



//	uint32_t adc0;
//uint8_t structregpointer = 0;
char TotalUpdate[512];

//HardwareTimer FreqTimer(3); //timer 2 conflicts with servo!!!


char *configFile = "config";

//Configurable Values
//char *commandFile = "/var/www/writefile.txt";
//char *statusFile = "/var/www/readfile.txt";
char *commandFile = "/dev/shm/command";
char *statusFile = "/dev/shm/status";
char *logFile = "/var/log/EveryCook_Deamon.log";


float ForceScaleFactor=0.1; //Conversion between digital Units and grams

float PressScaleFactor=0.1;
uint32_t PressOffset=1000;

float TempScaleFactor=0.1;
uint32_t TempOffset=1000;

//two calibrations points between ADC values and pressure in kPa
uint32_t PressADC1=100;
float PressValue1=0.1;
uint32_t PressADC2=1000;
float PressValue2=100.0;

//two calibrations points between ADC values and temperature in �C
uint32_t TempADC1=100;
float TempValue1=0.1;
uint32_t TempADC2=1000;
float TempValue2=100.0;

//two calibrations points between ADC values and temperature in �C
uint32_t ForceADC1=100;
float ForceValue1=0.1;
uint32_t ForceADC2=1000;
float ForceValue2=100.0;


uint8_t GainScale_1=8;
uint8_t GainScale_2=8;
uint8_t GainScale_3=8;
uint8_t GainScale_4=8;
uint8_t GainPress=8;
uint8_t GainTemp=4;

uint8_t BeepWeightReached=1;
uint8_t BeepStepEnd=1;

uint8_t DeleteLogOnStart=1;  //delete log at start saves disk space set 0 to keep log

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
uint32_t setStepId = 0;

float temp = 0;
float press = 0;
uint8_t motorRpm = 0; //0-255 
uint32_t motorOn = 0;
uint32_t motorOff = 0;
float weight = 0;
uint32_t time = 0;
uint32_t mode = 0;
uint32_t stepId = -1;

uint32_t oldMode = 0;
float oldTemp = 0;
float oldPress = 0;
float oldWeight = 0;




//Processing Values
uint32_t Delay = 1;
uint32_t runTime; //we need a runtime in seconds

//uint32_t stepRunTime = 0;
uint32_t stepEndTime = 0;
uint32_t stepStartTime = 0;


float referenceForce = 0; //the reference to get the zero of the scale
uint8_t scaleReady = 0;

uint8_t heatPowerStatus = 0; //Induction heater power 0=off >0=on
uint32_t nextTempCheckTime=0; //when did we do the last Temperature Check???


//uint8_t MotorDir =0; //0=cw 1=ccw
uint32_t motorStartTime =0; //When did we start the motor?

//float ForceScaleFactor = 4; //12.33 Conversion between digital Units and grams
//float ForceScaleFactor = 1;
uint32_t RemainTime=0;
uint32_t BeepEndTime=0; //when to stop the beeper

//Servo stuff
const uint32_t ValveOpenAngle=4000;
const uint32_t ValveClosedAngle=0;
uint32_t ValveSetValue=0;

//Options
bool doRememberBeep = 0;

uint8_t isBuzzing = 1; //to be sure first send a off to buzzer
const uint8_t ALLWAYS_STOP_BUZZING = 0;

uint32_t motorPwm = 0;
bool servoOpen = false;

char oldSegmentDisplay = ' ';
uint8_t blinkState = 0;

FILE *logFilePointer;

char curSegmentDisplay = ' ';

bool simulationMode = false;
bool simulationModeShow7Segment = false;
uint32_t simulationUpdateTime = 0;

bool debug_enabled = false;

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


void printUsage(){
	printf("Usage: ecfirmware [OPTIONS]\r\n");
	printf("programm options:\r\n");
	printf("  -s,  --sim        start in simulation mode, and will increase weight/temp/press automaticaly\r\n");
	printf("  -s7, --sim7seg    show 7Segment display in simulation Mode\r\n");
	printf("  -d,  --debug      activate Debug output\r\n");
	printf("  -?,  --help       show this help\r\n");
}


int main(int argc, const char* argv[]){
	printf("start...\n");
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
			debug_enabled = 1;
		} else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-?") == 0){
			printUsage();
			return 0;
		} else {
			printUsage();
			return 1;
		}
	}
	
	initHardware();
	delay(30);
	StringClean(TotalUpdate, 512);
	ReadConfigurationFile();
	
	initOutputFile();
	
	//if (DeleteLogOnStart){
		logFilePointer = fopen(logFile, "w");
	/*} else {
		logFilePointer = fopen(logFile, "w+");
	}*/
	Delay = LongDelay;
	
	//setFreqTimer();
	resetValues();
	
	if (debug_enabled){printf("commandFile: %s\n", commandFile);}
	if (debug_enabled){printf("statusFile: %s\n", statusFile);}
	
	runTime = millis()/1000;
	stepStartTime = runTime;
	
	while (1){
		//try {
			if (debug_enabled){printf("loop...\n");}
			runTime = millis()/1000; //calculate runtime in seconds
			//TODO_wia use system time???
			if (ReadFile()){
				ProcessCommand();
				runTime = millis()/1000;
				time = runTime - stepStartTime;
				WriteFile();
			} else {
				delay(10000);
			}
			if (debug_enabled){printf("loop end.\n");}
		/*} catch(exception e){
			if (debug_enabled){printf("Exception: %s\n", e);}
		}*/
		delay(Delay);
	}
	
	//fputs(TotalUpdate, fp);
	fclose(logFilePointer);
}


void initOutputFile(void){
	if (debug_enabled){printf("iniOutputFile\n");}
	//StringClean(TotalUpdate, 512);
	FILE *fp;
	//StringUnion(TotalUpdate, "{\"T0\":0,\"P0\":0,\"M0RPM\":0,\"M0ON\":0,\"M0OFF\":0,\"W0\":0,\"STIME\":0,\"SMODE\":0,\"SID\":0}");
	fp = fopen(statusFile, "w");
	//fputs(TotalUpdate, fp);
	fputs("{\"T0\":0,\"P0\":0,\"M0RPM\":0,\"M0ON\":0,\"M0OFF\":0,\"W0\":0,\"STIME\":0,\"SMODE\":0,\"SID\":0}", fp);
	fclose(fp);
	//StringClean(TotalUpdate, 512);
}

void resetValues(){
	isBuzzing = 1; //to be sure first send a off to buzzer
	oldSegmentDisplay = ' ';
}



void ProcessCommand(void){
	if (setStepId != stepId){
		if (setStepId < stepId){
			//TODO is reset/stop? or is someone try to begin new recipe before old is ended?
		}
		oldMode = mode;
		mode=setMode;
		stepId = setStepId;
		stepStartTime = runTime;
		
		if (debug_enabled){printf("stepId changed, new mode is: %d\n", mode);}
		if (debug_enabled || simulationMode){printf("ProcessCommand: T0: %d, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %d, STIME: %d, SMODE: %d, SID: %d\n", setTemp, setPress, setMotorRpm, setMotorOn, setMotorOff, setWeight, setTime, setMode, setStepId);}
		
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
			if (BeepStepEnd>0){
				BeepEndTime = runTime+BeepStepEnd;
			}
		} else if (mode != MODE_STANDBY){
			mode=MODE_STANDBY;
			if (BeepStepEnd>0){
				BeepEndTime = runTime+BeepStepEnd;
			}
		}
	}
	SegmentDisplay();
	Beep();
}


/******************* functions to evaluate **********************/


float readTemp(){
	if (simulationMode){
		int DeltaT=setTemp-oldTemp;
		float tempValue = oldTemp;	
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
		//if (debug_enabled){printf("readTemp, new Value is %f\n", tempValue);}
		printf("readTemp, new Value is %f\n", tempValue);
		return tempValue;
	} else {
		uint32_t tempValueInt = readADC(ADC_Temp);
		//TODO do calculate from value to °C
		float tempValue = ((float)tempValueInt-(float)TempOffset) * TempScaleFactor;
		tempValue = roundf(tempValue);
		if (debug_enabled){printf("readTemp, new Value is %d digits %f °C\n", tempValueInt, tempValue);}
		return tempValue;
	}
}


float readPress(){
	if (simulationMode){
		int DeltaP=setPress-oldPress;
		float pressValue = oldPress;
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
		
		//if (debug_enabled){printf("readPress, new Value is %f\n", pressValue);}
		printf("readPress, new Value is %f\n", pressValue);
		return pressValue;
	} else {
		uint32_t pressValueInt = readADC(ADC_Press);
		//TODO do calculate from value to kpa
		float pressValue = ((float)pressValueInt-(float)PressOffset) * PressScaleFactor;
		if (debug_enabled){printf("readPress, new Value is %d / %f\n", pressValueInt, pressValue);}
		return pressValue;
	}
}

//Power control functions
void HeatOn() {
	//if (debug_enabled || simulationMode){printf("HeatOn, was: %d\n", heatPowerStatus);}
	if (heatPowerStatus==0) { //if its off
		if (debug_enabled || simulationMode){printf("HeatOn\n");}
		if (!simulationMode){
			writeControllButtonPin(IND_KEY4, 1); //"press" the power button
			delay(50);
			writeControllButtonPin(IND_KEY4, 0);
		}
		heatPowerStatus=1; //save that we turned it on
	}
}

void HeatOff(){
	//if (debug_enabled || simulationMode){printf("HeatOff, was: %d\n", heatPowerStatus);}
	if (heatPowerStatus>0) { //if its on
		if (debug_enabled || simulationMode){printf("HeatOff\n");}
		if (!simulationMode){
			writeControllButtonPin(IND_KEY4, 1); //"press" the power button
			delay(50);
			writeControllButtonPin(IND_KEY4, 0);
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

float readWeight(){
	if (simulationMode){
		if (!scaleReady) {
			return 0.0;
		} else {
			if (runTime <= simulationUpdateTime){
				return oldWeight;
			} else {
				int deltaW = setWeight-oldWeight;
				float weightValue = oldWeight;
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
		
		float weightValue = (float)weightValueSum;
		weightValue *=ForceScaleFactor;
		weightValue = roundf(weightValue);
		if (debug_enabled){printf("readWeight, new Value is %d digits, %f grams (%d, %d, %d, %d)\n", weightValueSum, weightValue, weightValue1, weightValue2, weightValue3, weightValue4);}
		return weightValue;
	}
}

float readWeightSeparate(float* values){
	if (simulationMode){
		if (!scaleReady) {
			return 0.0;
		} else {
			int deltaW = setWeight-oldWeight;
			float weightValue = oldWeight;
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
			values[0] = (float)weightValue1 * ForceScaleFactor;
			values[1] = (float)weightValue2 * ForceScaleFactor;
			values[2] = (float)weightValue3 * ForceScaleFactor;
			values[3] = (float)weightValue4 * ForceScaleFactor;
		}
		
		uint32_t weightValueSum = (weightValue1+weightValue2+weightValue3+weightValue4) / 4;
		
		float weightValue = (float)weightValueSum;
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
		blink7Segment();
	}
    mode=MODE_STANDBY;
  }
}

void TempControl(){
	if (mode<MIN_COOK_MODE || mode>MAX_COOK_MODE) HeatOff();
	if (mode>=MIN_TEMP_MODE && mode<=MAX_TEMP_MODE) {
		if (runTime>=nextTempCheckTime || heatPowerStatus==0){
			HeatOff();
			delay(100);
			oldTemp = temp;
			uint32_t TempValue = readTemp();
			temp=TempValue;
			
			//SerialUSB.println("Temperature control: ");
			//SerialUSB.print(temp);  SerialUSB.print(" Degree C - Bottom Temperature");
			int DeltaT=setTemp-temp;
			if (mode==MODE_HEATUP || mode==MODE_COOK) {
				if (DeltaT<=0) {
					nextTempCheckTime=runTime+1;
					if (mode==MODE_HEATUP) {//heatup function
						stepEndTime=runTime-2;
						mode=MODE_HOT; //we are hot
					}
				}
				else if (DeltaT <= 10) { nextTempCheckTime=runTime+5; HeatOn(); }
				else if (DeltaT <= 50) { nextTempCheckTime=runTime+10; HeatOn();}
				else {nextTempCheckTime=runTime+20; HeatOn();}
				//  SerialUSB.print("runTime is now: ");SerialUSB.print(runTime);SerialUSB.print(" s Next Temperature Check at: ");SerialUSB.print(nextTempCheckTime);SerialUSB.println(" s"); 
			}
			if (mode==MODE_HEATUP && heatPowerStatus==1) { //heatup
				stepEndTime=nextTempCheckTime+1;
			} else if (mode==MODE_COOLDOWN && DeltaT>=0) { //cooldown function
				stepEndTime=runTime-2;
				mode=MODE_COLD;
			} else if (mode==MODE_COOLDOWN) {
				nextTempCheckTime=runTime+1;
				stepEndTime=nextTempCheckTime+1;
			}
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
			//SerialUSB.println("Pressure control: ");
			//SerialUSB.print(press);  SerialUSB.print(" kPa Pressure");
			int DeltaP=setPress-press;
			if (mode==MODE_PRESSUP || mode==MODE_PRESSHOLD) {
				if (DeltaP<=0) {
					nextTempCheckTime=runTime+1;
					if (mode==MODE_PRESSUP) {//pressup function
						stepEndTime=runTime-2;
						mode=MODE_PRESSURIZED; //we are pressurized
					}
				}
				else if (DeltaP <= 10) { nextTempCheckTime=runTime+5; HeatOn(); }
				else if (DeltaP <= 50) { nextTempCheckTime=runTime+10; HeatOn();}
				else {nextTempCheckTime=runTime+20; HeatOn();}
				//SerialUSB.print("runTime is now: ");SerialUSB.print(runTime);SerialUSB.print(" s Next Pressure Check at: ");SerialUSB.print(nextTempCheckTime);SerialUSB.println(" s"); 
			}
			if (mode==MODE_PRESSUP && heatPowerStatus==1) { //pressure up
				stepEndTime=nextTempCheckTime+1;
			} else if (mode==MODE_PRESSDOWN && DeltaP>=0) { //pressure down function
				stepEndTime=runTime-2;
				mode=MODE_PRESSURELESS;
			} else if (mode==MODE_PRESSDOWN) {
				nextTempCheckTime=runTime+1;
				stepEndTime=nextTempCheckTime+1;
			}
		}
	} else if (mode>=MIN_TEMP_MODE && mode<=MAX_TEMP_MODE) {
		if (runTime>=nextTempCheckTime || heatPowerStatus==0){
			//press=map(analogRead(PressPin),0,2000,0,100); //read pressure
			uint32_t pressValue = readPress();
			oldPress = press;
			press=pressValue;
		}
	}
}

void MotorControl (){
	if (mode==MODE_CUT || mode>=MIN_TEMP_MODE){
		if (mode==MODE_CUT) stepEndTime=runTime+1;
		if (setMotorRpm > 0 && mode<=MAX_STATUS_MODE) {
			//  SerialUSB.println("Motor Control");
			// SerialUSB.print("Encoder Changes"); SerialUSB.println(EncoderChanges);
			// SerialUSB.print("Last Encoder Changes"); SerialUSB.println(LastEncoderChanges);

			if  (setMotorOn > 0 && setMotorOff > 0) {
				if (runTime >= motorStartTime+setMotorOn && runTime < motorStartTime+setMotorOn+setMotorOff) { 
					motorRpm = 0; 
				} else if (motorRpm==0){
					motorRpm = setMotorRpm;
					motorStartTime=runTime;
				}
				//  SerialUSB.println("Interval mode");
			} else {
				motorRpm = setMotorRpm;
				motorStartTime=runTime;
			}
			/*  SerialUSB.print("runTime: ");  SerialUSB.print(runTime);  SerialUSB.print(" motorStartTime: ");  SerialUSB.print(motorStartTime);
			SerialUSB.print(" MotorrunTime: ");  SerialUSB.print(setMotorOn);  SerialUSB.print(" MotorPauseTime: ");  SerialUSB.print(setMotorOff);
			SerialUSB.print(" Motor Rpm: ");  SerialUSB.println(motorRpm);
			*/
		}
	} else {
		//SerialUSB.println(" Motor off: ");
		motorRpm=0;
	}
	/*
	if (motorRpm>0 && LastEncoderChanges==EncoderChanges){
		mode=42;
		motorRpm=0;
	}
	*/
	//pwmWrite(RPMPin,motorRpm);
	setMotorPWM(motorRpm);
	//LastEncoderChanges=EncoderChanges;
}

void ValveControl(){
	if (mode==MODE_PRESSVENT) {
		HeatOff();
		//servo1.write(ValveOpenAngle);
		setServoOpen(1);
		stepEndTime=runTime+1;
		if (press < LowPress) {
			delay(2000);
			mode=MODE_PRESSURELESS;
		}
	} else {
		//servo1.write(ValveClosedAngle);
		setServoOpen(0);
	}
}


void ScaleFunction () {
	if (mode==MODE_SCALE || mode==MODE_WEIGHT_REACHED){
		stepEndTime=runTime+2;
		oldWeight = weight;
		float SumOfForces = readWeight();
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
			
			if (weight>=setWeight) {//If we have reached the required mass
				if (mode != MODE_WEIGHT_REACHED){
					if(BeepWeightReached > 0){
						BeepEndTime=runTime+BeepWeightReached;
					}
					if (debug_enabled){printf("\tweight reached!\n");}
				} else {
					if (debug_enabled){printf("\tweight reached...\n");}
				}
				mode=MODE_WEIGHT_REACHED;
				RemainTime=0;
				//SerialUSB.println("Mass reached");
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
	/*if (mode==53) {
		digitalWrite(Seg1Pin,HIGH);
		digitalWrite(Seg2Pin,LOW);
		return;
	}*/ 
	
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
			
			//togglePin(I2C_7SEG_PERIOD);
			if (blinkState){
				writeI2CPin(I2C_7SEG_PERIOD,I2C_7SEG_OFF);
				blinkState = 0;
				if (debug_enabled){printf("SegmentDisplay: blink off\n");}
			} else {
				blinkState = 1;
				writeI2CPin(I2C_7SEG_PERIOD,I2C_7SEG_ON);
				if (debug_enabled){printf("SegmentDisplay: blink on\n");}
			}
		} else {
			oldSegmentDisplay = curSegmentDisplay;
		}
	}
}

void SegmentDisplaySimple(char curSegmentDisplay){
	if (curSegmentDisplay == 'P'){
		oldSegmentDisplay = 'P';
		writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
	} else if (curSegmentDisplay == 'H'){
		oldSegmentDisplay = 'H';
		writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
	} else if (curSegmentDisplay == '0'){
		oldSegmentDisplay = '0';
		writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_ON);
		writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_ON);
	} else {
		oldSegmentDisplay = ' ';
		writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
	}
}

void SegmentDisplayOptimized(char curSegmentDisplay){
	if (curSegmentDisplay == 'P'){
		if (curSegmentDisplay != oldSegmentDisplay){
			if (oldSegmentDisplay == 'H'){
				writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_OFF);
				//writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
			} else if (oldSegmentDisplay == '0'){
				//writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_OFF);
				writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
			} else if (oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_OFF);
				writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
			}
			oldSegmentDisplay = curSegmentDisplay;
		}
	} else if (curSegmentDisplay == 'H'){
		if (curSegmentDisplay != oldSegmentDisplay){
			if (oldSegmentDisplay == 'P'){
				writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_OFF);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
			} else if (oldSegmentDisplay == '0'){
				writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_OFF);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
			} else if (oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_OFF);
				writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
			}
			oldSegmentDisplay = curSegmentDisplay;
		}
	} else if (curSegmentDisplay == '0'){
		if (curSegmentDisplay != oldSegmentDisplay){
			if (oldSegmentDisplay == 'H'){
				writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_OFF);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_ON);
			} else if (oldSegmentDisplay == 'P'){
				//writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_OFF);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_ON);
			} else if (oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_OFF);
				writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_ON);
				writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_ON);
			}
			oldSegmentDisplay = curSegmentDisplay;
		}
	} else {
		//Empty all
		writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_OFF);
		writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
		oldSegmentDisplay = ' ';
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
	
	//if (runTime<BeepEndTime) digitalWrite(BeepPin,HIGH);
	//else digitalWrite(BeepPin,LOW);
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

/* Get the adc datas and signals and write to the file "/var/www/readfile.txt"
 */
void WriteFile(void){
	if (debug_enabled){printf("WriteFile\n");}
	char tempString[10];
	StringClean(tempString, 10);
	StringClean(TotalUpdate, 512);
	FILE *fp;
	
	StringUnion(TotalUpdate, "{");
	StringUnion(TotalUpdate, "\"T0\":");
	NumberConvertToString((uint32_t)temp, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	StringClean(tempString, 10);
	
	StringUnion(TotalUpdate, "\"P0\":");
	NumberConvertToString((uint32_t)press, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	StringClean(tempString, 10);
	
	StringUnion(TotalUpdate, "\"M0RPM\":");
	NumberConvertToString(motorRpm, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	StringClean(tempString, 10);
	
	StringUnion(TotalUpdate, "\"M0ON\":");
	NumberConvertToString(motorOn, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	StringClean(tempString, 10);
	
	StringUnion(TotalUpdate, "\"M0OFF\":");
	NumberConvertToString(motorOff, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	StringClean(tempString, 10);
	
	StringUnion(TotalUpdate, "\"W0\":");
	NumberConvertToString((uint32_t)weight, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	StringClean(tempString, 10);
	
	StringUnion(TotalUpdate, "\"STIME\":");
	NumberConvertToString(time, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	StringClean(tempString, 10);
	
	StringUnion(TotalUpdate, "\"SMODE\":");
	NumberConvertToString(mode, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	StringClean(tempString, 10);
	
	StringUnion(TotalUpdate, "\"SID\":");
	NumberConvertToString(stepId, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, "}");
	
	if (debug_enabled){printf("WriteFile: T0: %f, P0: %f, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %f, STIME: %d, SMODE: %d, SID: %d\n", temp, press, motorRpm, motorOn, motorOff, weight, time, mode, stepId);}
	
	fp = fopen(statusFile, "w");
	fputs(TotalUpdate, fp);
	fclose(fp);
	
	StringUnion(TotalUpdate, "\n");
	if (debug_enabled){printf("WriteFile: before write\n");}
	fputs(TotalUpdate, logFilePointer);
	if (debug_enabled){printf("WriteFile: after write\n");}
	StringClean(TotalUpdate, 512);
}

//format: {"T0":000,"P0":000,"M0RPM":0000,"M0ON":000,"M0OFF":000,"W0":0000,"STIME":000000,"SMODE":00,"SID":000}

bool ReadFile(void){
	if (debug_enabled){printf("ReadFile\n");}
	FILE *fp;
	char tempName[10];
	char tempValue[10];
	uint32_t value[15];
	char names[15][10];
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
			
			StringClean(tempName, 10);
			StringClean(tempValue, 10);
			ptr++;
		}
	}
	fclose(fp);
	
	//TODO only set "setXXX" values if stepId changed, other wise ignore "new/changed values".
	for(i = 0; i < 15; ++i){
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
	if (debug_enabled){printf("ReadFile: T0: %d, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %d, STIME: %d, SMODE: %d, SID: %d\n", setTemp, setPress, setMotorRpm, setMotorOn, setMotorOff, setWeight, setTime, setMode, setStepId);}
	
	if(setTemp>200) {setTemp=200;}
	if(setPress>200) {setPress=200;}
	if (setMotorOn>0 && setMotorOn<2) setMotorOn=2;
	if (setMotorOff>0 && setMotorOff<2) setMotorOff=2;
	return true;
}

/* Convert a number to a string
 *
 */
void NumberConvertToString(uint32_t num, char *str){
	uint8_t i = 0;
	uint32_t value, mutiplecand = 1;

	value = num;
	do {
		mutiplecand = mutiplecand*10;
		i++;
	//} while (value >= mutiplecand || i > 9); //TODO: this would get en endloos loop if i>9...
	} while (value >= mutiplecand);
	
	while (mutiplecand != 1){
		mutiplecand = mutiplecand/10;
		*str++ = num/mutiplecand+48;
		num = num%mutiplecand;
	}
}
/* Clean the string
 *
 */
void StringClean(char *str, uint32_t len){
	uint32_t i = 0;

	for (; i< len; i++){
		str[i] = 0x00;
	}
}
/* Combine two strings to one string
 *
 */
void StringUnion(char *fristString, char *secondString){
	uint8_t i = 0, fristEndPtr = 0;

	while (fristString[fristEndPtr]){
		fristEndPtr++;
	}
	while (secondString[i]){
		fristString[fristEndPtr+i] = secondString[i];
		i++;
 	}
}
/* convert a string to a number
 *
 */
uint32_t StringConvertToNumber(char *str){
	uint32_t value = 0 ,len = 0, mutiple = 1;

	while (str[len]){
		len++;
		mutiple *= 10;
	}
	len = 0;	
	while (str[len]){
		mutiple = mutiple/10;
		value = value + (str[len]-48)*mutiple;
		len++;
	}
	return value;
}

float StringConvertToFloat(char *str){
	float value = 0.0, mutiple = 1.0;
	uint32_t len = 0;

	while (str[len]){
		if (str[len] == '.'){
			break;
		}
		len++;
		mutiple *= 10.0;
	}
	len = 0;	
	while (str[len]){
		if (str[len] != '.'){
			mutiple = mutiple/10.0;
			value = value + (str[len]-48)*mutiple;
		}
		len++;
	}
	return value;
}





/* Read the configuration
 *
 */
void ReadConfigurationFile(void){
	if (debug_enabled){printf("ReadConfigurationFile...\n");}
	
	uint32_t newADCConfig[] = {0x710, 0x711, 0x712, 0x713, 0x714, 0x115};
	
	FILE *fp;
	char keyString[30];
	char valueString[100];
	uint8_t i = 0;
	uint8_t ptr = 0;
	char c;
	StringClean(keyString, 30);
	StringClean(valueString, 100);
	fp = fopen(configFile, "r");
	if (fp != NULL){
		while ((c = fgetc(fp)) != 255){
			//c = fgetc(fp);
			if (c == '#'){
				if (debug_enabled){printf("\tline with # found\n");}
				while (c != '\n'){
					c = fgetc(fp);
				}
			} else if (c == '\n'){
				//Empty line
			} else if (c == '\r'){
				//Empty line
			} else {
				i = 0;
				while (c != '='){
					keyString[i] = c;
					c = fgetc(fp);
					i++;
				}
				i = 0;
				c = fgetc(fp); //currently its '=' so read next
				while (c != '\n'){
					if (c != '\r'){
						valueString[i] = c;
					}
					c = fgetc(fp);
					i++;
				}
				
				if (debug_enabled){printf("\tkey: %s, value: %s\n", keyString, valueString);}
				
				//ParseConfigValue
				if(strcmp(keyString, "ForceADC1") == 0){
					ForceADC1 = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tForceADC1: %d\n", ForceADC1);} // (old: %d)
				} else if(strcmp(keyString, "ForceValue1") == 0){
					ForceValue1 = StringConvertToFloat(valueString);
					if (debug_enabled){printf("\tForceValue1: %f\n", ForceValue1);} // (old: %d)
				} else if(strcmp(keyString, "ForceADC2") == 0){
					ForceADC2 = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tForceADC2: %d\n", ForceADC2);} // (old: %d)
				} else if(strcmp(keyString, "ForceValue2") == 0){
					ForceValue2 = StringConvertToFloat(valueString);
					if (debug_enabled){printf("\tForceValue2: %f\n", ForceValue2);} // (old: %d)
				
				} else if(strcmp(keyString, "PressADC1") == 0){
					PressADC1 = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tPressADC1: %d\n", PressADC1);} // (old: %d)
				} else if(strcmp(keyString, "PressValue1") == 0){
					PressValue1 = StringConvertToFloat(valueString);
					if (debug_enabled){printf("\tPressValue1: %f\n", PressValue1);} // (old: %d)
				} else if(strcmp(keyString, "PressADC2") == 0){
					PressADC2 = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tPressADC2: %d\n", PressADC2);} // (old: %d)
				} else if(strcmp(keyString, "PressValue2") == 0){
					PressValue2 = StringConvertToFloat(valueString);
					if (debug_enabled){printf("\tPressValue2: %f\n", PressValue2);} // (old: %d)
				
				} else if(strcmp(keyString, "TempADC1") == 0){
					TempADC1 = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tTempADC1: %d\n", TempADC1);} // (old: %d)
				} else if(strcmp(keyString, "TempValue1") == 0){
					TempValue1 = StringConvertToFloat(valueString);
					if (debug_enabled){printf("\tTempValue1: %f\n", TempValue1);} // (old: %d)
				} else if(strcmp(keyString, "TempADC2") == 0){
					TempADC2 = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tTempADC2: %d\n", TempADC2);} // (old: %d)
				} else if(strcmp(keyString, "TempValue2") == 0){
					TempValue2 = StringConvertToFloat(valueString);
					if (debug_enabled){printf("\tTempValue2: %f\n", TempValue2);} // (old: %d)
				
				
				} else if(strcmp(keyString, "ADC_LoadCellFrontLeft") == 0){
					ADC_LoadCellFrontLeft = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tADC_LoadCellFrontLeft: %d\n", ADC_LoadCellFrontLeft);}

				} else if(strcmp(keyString, "ADC_LoadCellFrontRight") == 0){
					ADC_LoadCellFrontRight = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tLADC_oadCellFrontRight: %d\n", ADC_LoadCellFrontRight);}

				} else if(strcmp(keyString, "ADC_LoadCellBackLeft") == 0){
					ADC_LoadCellBackLeft = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tADC_LoadCellBackLeft: %d\n", ADC_LoadCellBackLeft);}

				} else if(strcmp(keyString, "ADC_LoadCellBackRight") == 0){
					ADC_LoadCellBackRight = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tADC_LoadCellBackRight: %d\n", ADC_LoadCellBackRight);}

				} else if(strcmp(keyString, "ADC_Press") == 0){
					ADC_Press = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tADC_Press: %d\n", ADC_Press);}

				} else if(strcmp(keyString, "ADC_Temp") == 0){
					ADC_Temp = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tADC_Temp: %d\n", ADC_Temp);}
					
				} else if(strcmp(keyString, "Gain_LoadCellFrontLeft") == 0){
					ptr = ADC_LoadCellFrontRight;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (debug_enabled){printf("\tGain_LoadCellFrontLeft: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_LoadCellFrontRight") == 0){
					ptr = ADC_LoadCellBackLeft;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (debug_enabled){printf("\tGain_LoadCellFrontRight: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_LoadCellBackLeft") == 0){
					ptr = ADC_LoadCellBackRight;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (debug_enabled){printf("\tGain_LoadCellBackLeft: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_LoadCellBackRight") == 0){
					ptr = ADC_LoadCellBackRight;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (debug_enabled){printf("\tGain_LoadCellBackRight: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_Press") == 0){
					ptr = ADC_Press;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (debug_enabled){printf("\tGain_Press: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "Gain_Temp") == 0){
					ptr = ADC_Temp;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (debug_enabled){printf("\tGain_Temp: %04X\n", newADCConfig[ptr]);} // (old: %d)
				
				} else if(strcmp(keyString, "BeepWeightReached") == 0){
					BeepWeightReached = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tBeepWeightReached: %d\n", BeepWeightReached);} // (old: %d)
				} else if(strcmp(keyString, "BeepStepEnd") == 0){
					BeepStepEnd = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tBeepStepEnd: %d\n", BeepStepEnd);} // (old: %d)
				} else if(strcmp(keyString, "doRememberBeep") == 0){
					doRememberBeep = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tdoRememberBeep: %d\n", doRememberBeep);} // (old: %d)
				} else if(strcmp(keyString, "DeleteLogOnStart") == 0){
					DeleteLogOnStart = StringConvertToNumber(valueString);
					if (debug_enabled){printf("\tDeleteLogOnStart: %d\n", DeleteLogOnStart);} // (old: %d)
				} else if(strcmp(keyString, "LogFile") == 0){
					//TODO: alloc new mem
					//logFile = valueString;
					if (debug_enabled){printf("\tLogFile: %s\n", logFile);} // (old: %s)
				} else if(strcmp(keyString, "CommandFile") == 0){
					//TODO: alloc new mem
					//commandFile = valueString;
					if (debug_enabled){printf("\tCommandFile: %s\n", commandFile);} // (old: %s)
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
				} else {
					if (debug_enabled){printf("\tkey not Found\n");}
				}
				StringClean(keyString, 30);
				StringClean(valueString, 100);
			}
		}
	}
	/*
	ForceScaleFactor=100.0/((float)ForceValue100-(float)ForceValue0);
	PressScaleFactor=100.0/((float)PressValue100-(float)PressValue0);
	PressOffset=PressValue0;
	TempScaleFactor=100.0/((float)TempValue100-(float)TempValue0);
	TempOffset=TempValue0;
	*/
	
	ForceScaleFactor=(ForceValue2-ForceValue1)/((float)ForceADC2-(float)ForceADC1);
	
	PressScaleFactor=(PressValue2-PressValue1)/((float)PressADC2-(float)PressADC1);
	PressOffset=PressScaleFactor/PressValue1;
	
	PressScaleFactor=(TempValue2-TempValue1)/((float)TempADC2-(float)TempADC1);
	PressOffset=PressScaleFactor/TempValue1;
	
	if (debug_enabled){printf("ForceScaleFactor: %f, PressScaleFactor: %f, PressOffset: %d, TempScaleFactor: %f, TempOffset: %d\n", ForceScaleFactor, PressScaleFactor, PressOffset, TempScaleFactor, TempOffset);}
	
	fclose(fp);
	if (debug_enabled){printf("done.\n");}
}
/*
*/
int POWNTimes(uint32_t num, uint8_t n){
	int i = 0;

	while (num > 1){
		num = num / n;
		i++;
	}
	return i;
}


void blink7Segment(){
	//mitte
	writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_ON);
	delay(500);
	writeI2CPin(I2C_7SEG_TOP,I2C_7SEG_OFF);
	
	//links
	writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_ON);
	delay(500);
	writeI2CPin(I2C_7SEG_TOP_LEFT,I2C_7SEG_OFF);
	
	//oben
	writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_ON);
	delay(500);
	writeI2CPin(I2C_7SEG_TOP_RIGHT,I2C_7SEG_OFF);
	
	
	//rechts
	writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_ON);
	delay(500);
	writeI2CPin(I2C_7SEG_CENTER,I2C_7SEG_OFF);
	
	//ulinks
	writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_ON);
	delay(500);
	writeI2CPin(I2C_7SEG_BOTTOM_LEFT,I2C_7SEG_OFF);
	
	//unten
	writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_ON);
	delay(500);
	writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,I2C_7SEG_OFF);
	
	//urechts
	writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_ON);
	delay(500);
	writeI2CPin(I2C_7SEG_BOTTOM,I2C_7SEG_OFF);
	
	//punkt
	writeI2CPin(I2C_7SEG_PERIOD,I2C_7SEG_ON);
	delay(500);
	writeI2CPin(I2C_7SEG_PERIOD,I2C_7SEG_OFF);
	delay(500);
}




































		
	
	
	
	
	
	