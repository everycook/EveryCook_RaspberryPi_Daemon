#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>

#include <softPwm.h>
#include <wiringPi.h>


#include <string.h>

#include "basic_functions.h"
#include "firmware.h"



//Other functions
void NumberConvertToString(uint32_t num, char *str);
void StringClean(char *str, uint32_t len);
void StringUnion(char *fristString, char *secondString);
uint32_t StringConvertToNumber(char *str);
int POWNTimes(uint32_t num, uint8_t n);





float readTemp();
float readPress();
void HeatOn();
void HeatOff();
void setMotorPWM(uint32_t pwm);
void setServoOpen(uint8_t open);
float readWeight();




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
uint32_t ForceValue0=0;
uint32_t ForceValue100=1000;

float PressScaleFactor=0.1;
uint32_t PressOffset=1000;
uint32_t PressValue0=0;
uint32_t PressValue100=1000;

float TempScaleFactor=0.1;
uint32_t TempOffset=1000;
uint32_t TempValue0=0;
uint32_t TempValue100=1000;

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
uint32_t stepId = 0;

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

//hardware timer stuff...
//volatile bool LastEncoder=false;
//volatile int EncoderChanges=0;
//unsigned int LastEncoderChanges=0;
//uint32_t PrescaleFactor=1024; //15
//uint32_t Overflow=300;

//Servo stuff
const uint32_t ValveOpenAngle=4000;
const uint32_t ValveClosedAngle=0;
uint32_t ValveSetValue=0;

//Options
uint8_t doRememberBeep = 0;





uint8_t isBuzzing = 1; //to be sure first send a off to buzzer
const uint8_t ALLWAYS_STOP_BUZZING = 0;

char oldSegmentDisplay = ' ';
uint8_t blinkState = 0;

FILE *logFilePointer;



char curSegmentDisplay = ' ';




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




int main(void){
	if (DEBUG_ENABLED){printf("main\n");}
	initHardware();
	delay(30);
	StringClean(TotalUpdate, 512);
	ReadConfigurationFile();
	
	//if (DeleteLogOnStart){
		logFilePointer = fopen(logFile, "w");
	/*} else {
		logFilePointer = fopen(logFile, "w+");
	}*/
	Delay = LongDelay;
	
	//setFreqTimer();
	resetValues();
	
	if (DEBUG_ENABLED){printf("commandFile: %s\n", commandFile);}
	if (DEBUG_ENABLED){printf("statusFile: %s\n", statusFile);}
	
	runTime = millis()/1000;
	stepStartTime = runTime;
	
	while (1){
		//try {
			if (DEBUG_ENABLED){printf("loop...\n");}
			runTime = millis()/1000; //calculate runtime in seconds
			ReadFile();
			ProcessCommand();
			runTime = millis()/1000;
			time = runTime - stepStartTime;
			WriteFile();
			if (DEBUG_ENABLED){printf("loop end.\n");}
		/*} catch(exception e){
			if (DEBUG_ENABLED){printf("Exception: %s\n", e);}
		}*/
		delay(Delay);
	}
	
	//fputs(TotalUpdate, fp);
	fclose(logFilePointer);
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
		
		if (DEBUG_ENABLED){printf("stepId changed, new mode is: %d\n", mode);}
		
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
/*  //old:
	//Calibration: TempBot 0°C=24 100°C=25
	float TempValue=0;
	int AverageOf=1000;
	uint32_t TempValue = readTemp;
	for (int i =0; i< AverageOf; i++) {
		TempValue+=analogRead(TempPin); //read temperature
	}
	Temp=map(TempValue/AverageOf,635,3440,0,98);
*/
	
	uint32_t tempValueInt = readADC(ADC_TEMP);
	//TODO do calculate from value to °C
	float tempValue = ((float)tempValueInt-(float)TempOffset) * TempScaleFactor;
	if (DEBUG_ENABLED){printf("readTemp, new Value is %d / %f\n", tempValueInt, tempValue);}
	return tempValue;
}


float readPress(){
/*
	float PressValue=0;
	int AverageOf=1000;
	for (int i =0; i< AverageOf; i++){
		PressValue+=analogRead(PressPin); //read pressure
	}
	//      press=map(PressValue/AverageOf,8,3320,0,100); //origin
	press=map(PressValue/AverageOf,8,1250,0,100);
	//      press=PressValue/AverageOf;
	*/
	
	uint32_t pressValueInt = readADC(ADC_PRESS);
	//TODO do calculate from value to kpa
	float pressValue = ((float)pressValueInt-(float)PressOffset) * PressScaleFactor;
	if (DEBUG_ENABLED){printf("readPress, new Value is %d / %f\n", pressValueInt, pressValue);}
	return pressValue;
}

//Power control functions
void HeatOn() {
	if (DEBUG_ENABLED){printf("HeatOn, was: %d\n", heatPowerStatus);}
	if (heatPowerStatus==0) { //if its off
		//  SerialUSB.println("heaton");
		/*
		digitalWrite(HeatStopPin,LOW);
		digitalWrite(HeatStartPin,HIGH);
		delay(50);
		digitalWrite(HeatStartPin,LOW);
		*/
		
		writeControllButtonPin(IND_KEY4, 1);
		
		heatPowerStatus=1; //save that we turned it on
	}
}

void HeatOff(){
	if (DEBUG_ENABLED){printf("HeatOff, was: %d\n", heatPowerStatus);}
	if (heatPowerStatus>0) { //if its on
		//digitalWrite(HeatStopPin,HIGH);
		writeControllButtonPin(IND_KEY4, 0);
		heatPowerStatus=0; //save that we turned it off
	}
}

void setMotorPWM(uint32_t pwm){
	if (DEBUG_ENABLED){printf("setMotorPWM, pwm: %d\n", pwm);}
	writeI2CPin(I2C_MOTOR, pwm);
}

void setServoOpen(uint8_t open){
	if (DEBUG_ENABLED){printf("setServoOpen, open: %d\n", open);}
	if (open){
		writeI2CPin(I2C_SERVO, ValveOpenAngle);
	} else {
		writeI2CPin(I2C_SERVO, ValveClosedAngle);
	}
}

float readWeight(){
/*  //old
	int AverageOf = 5000; // 5000 takes about 150ms. we average sensor values to get better results. set 1 to turn off averaging.
	float ForceValue1 = 0;
	float ForceValue2 = 0;
	float ForceValue3 = 0;
	float ForceValue4 = 0;
	float SumOfForces=0;
	//int lastmillis=millis();
	for (int i =0; i< AverageOf; i++) {
	ForceValue1 = analogRead(Scale1Pin) + ForceValue1;
	ForceValue2 = analogRead(Scale2Pin) + ForceValue2;
	ForceValue3 = analogRead(Scale3Pin) + ForceValue3;
	ForceValue4 = analogRead(Scale4Pin) + ForceValue4;
	}
	ForceValue1 = ForceValue1 / AverageOf;
	ForceValue2 = ForceValue2 / AverageOf;
	ForceValue3 = ForceValue3 / AverageOf;
	ForceValue4 = ForceValue4 / AverageOf;
	/ *  SerialUSB.print("F1 ");
	SerialUSB.print(ForceValue1);
	SerialUSB.print(" F2 ");
	SerialUSB.print(ForceValue2);
	SerialUSB.print(" F3 ");
	SerialUSB.print(ForceValue3);
	SerialUSB.print(" F4 ");
	SerialUSB.println(ForceValue4);* /
	SumOfForces = (ForceValue1+ForceValue2+ForceValue3+ForceValue4)*ForceScaleFactor/4; 
	/ *  SerialUSB.print("Reference ");
	SerialUSB.print(referenceForce);
	SerialUSB.print(" Sum ");
	SerialUSB.println(SumOfForces);
	* /
*/
	
	uint32_t weightValue1 = readADC(ADC_WEIGHT1);
	uint32_t weightValue2 = readADC(ADC_WEIGHT2);
	uint32_t weightValue3 = readADC(ADC_WEIGHT3);
	uint32_t weightValue4 = readADC(ADC_WEIGHT4);
	
	uint32_t weightValueSum = (weightValue1+weightValue2+weightValue3+weightValue4) / 4;
	
	float weightValue = (float)weightValueSum;
	//TODO do calculate from value to g
	weightValue *=ForceScaleFactor;
	if (DEBUG_ENABLED){printf("readWeight, new Value is %d / %f (%d, %d, %d, %d)\n", weightValueSum, weightValue, weightValue1, weightValue2, weightValue3, weightValue4);}
	return weightValue;
}

/******************* processing functions **********************/
void OptionControl(){
  if (mode>=MODE_OPTIONS_BEGIN){
    if (mode==MODE_OPTION_REMEMBER_BEEP_ON){
      doRememberBeep=1;
    } else if (mode==MODE_OPTION_REMEMBER_BEEP_OFF){
      doRememberBeep=0;
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
			uint32_t TempValue = readTemp();
			oldTemp = temp;
			temp=TempValue;
			
			//SerialUSB.println("Temperature control: ");
			//SerialUSB.print(temp);  SerialUSB.print(" Degree C - Bottom Temperature");
			int DeltaT=setTemp-temp;
			if (mode==MODE_HEADUP || mode==MODE_COOK) {
				if (DeltaT<=0) {
					nextTempCheckTime=runTime+1;
					if (mode==MODE_HEADUP) {//heatup function
						stepEndTime=runTime-2;
						mode=MODE_HOT; //we are hot
					}
				}
				else if (DeltaT <= 10) { nextTempCheckTime=runTime+5; HeatOn(); }
				else if (DeltaT <= 50) { nextTempCheckTime=runTime+10; HeatOn();}
				else {nextTempCheckTime=runTime+20; HeatOn();}
				//  SerialUSB.print("runTime is now: ");SerialUSB.print(runTime);SerialUSB.print(" s Next Temperature Check at: ");SerialUSB.print(nextTempCheckTime);SerialUSB.println(" s"); 
			}
			if (mode==MODE_HEADUP && heatPowerStatus==1) { //heatup
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
			uint32_t pressValue = readPress();
			oldPress = press;
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
		float SumOfForces = readWeight();
		
		if (DEBUG_ENABLED){printf("ScaleFunction\n");}
		
		/*
		SerialUSB.print("Time ");
		SerialUSB.println(millis());
		SerialUSB.println(millis()-lastmillis);
		*/
		if (scaleReady==0) { //we are not ready for weighting
			//SerialUSB.println("referencing");
			referenceForce=SumOfForces;
			scaleReady=1;
			if (Delay == ShortDelay){
				Delay=LongDelay;
			}
			if (DEBUG_ENABLED){printf("\tscaleReady\n");}
		} else { //we have a reference and are ready
			Delay=ShortDelay;
			
			oldWeight = weight;
			weight=(SumOfForces-referenceForce);
			/*SerialUSB.print("Actual weight: ");
			SerialUSB.println(weight);
			SerialUSB.print("set weight: ");
			SerialUSB.print(setWeight);
			SerialUSB.println(" g");*/
			if (weight>=setWeight) {//If we have reached the required mass
				if (mode != MODE_WEIGHT_REACHED){
					if(BeepWeightReached > 0){
						BeepEndTime=runTime+BeepWeightReached;
					}
					if (DEBUG_ENABLED){printf("\tweight reached!\n");}
				} else {
					if (DEBUG_ENABLED){printf("\tweight reached...\n");}
				}
				mode=MODE_WEIGHT_REACHED;
				RemainTime=0;
				//SerialUSB.println("Mass reached");
			}
			if (DEBUG_ENABLED){printf("\t\tweight: %f / old: %f\n", weight, oldWeight);}
		}
	} else if (Delay == ShortDelay){
		scaleReady=0;
		referenceForce=0;
		Delay=LongDelay;
	}
}

/*
void TestWatchDog(){
	FreqTimer.pause();
	//SerialUSB.println("Stopped watchdog timer! Reset imminent");
}

void setFreqTimer() {
	FreqTimer.pause();
	FreqTimer.setPrescaleFactor(PrescaleFactor);
	FreqTimer.setOverflow(Overflow);
	FreqTimer.setMode(1,TIMER_OUTPUT_COMPARE);
	FreqTimer.setCompare(TIMER_CH1,1);
	FreqTimer.attachInterrupt(1,FreqHandler);
	FreqTimer.refresh();
	FreqTimer.resume();
}
void FreqHandler() { 
//  togglePin(FreqPin);
//  togglePin(BOARD_LED_PIN);
	iwdg_feed();
	if (digitalRead(EncoderPin)!=LastEncoder){
		EncoderChanges++;
		LastEncoder=!LastEncoder;
	}
}
*/
void SegmentDisplay(){
	/*if (mode==53) {
		digitalWrite(Seg1Pin,HIGH);
		digitalWrite(Seg2Pin,LOW);
		return;
	}*/ 
	
	char curSegmentDisplay = ' ';
	if (press>LowPress){
		curSegmentDisplay = 'P';
	} else if (temp>LowTemp){
		curSegmentDisplay = 'H';
	} else {
		curSegmentDisplay = '0';
	}
	SegmentDisplaySimple(curSegmentDisplay);
	//SegmentDisplayOptimized(curSegmentDisplay);
	
	//togglePin(I2C_7SEG_PERIOD);
	if (blinkState){
		writeI2CPin(I2C_7SEG_PERIOD,I2C_7SEG_OFF);
		blinkState = 0;
		if (DEBUG_ENABLED){printf("SegmentDisplay: blink off\n");}
	} else {
		blinkState = 1;
		writeI2CPin(I2C_7SEG_PERIOD,I2C_7SEG_ON);
		if (DEBUG_ENABLED){printf("SegmentDisplay: blink on\n");}
	}
	
	if (DEBUG_ENABLED){printf("SegmentDisplay: char: %c\n", oldSegmentDisplay);}
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
	if (DEBUG_ENABLED){printf("WriteFile\n");}
	char tempString[10];
	StringClean(tempString, 10);
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
	
	if (DEBUG_ENABLED){printf("WriteFile: T0: %f, P0: %f, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %f, STIME: %d, SMODE: %d, SID: %d\n", temp, press, motorRpm, motorOn, motorOff, weight, time, mode, stepId);}
	
	fp = fopen(statusFile, "w");
	fputs(TotalUpdate, fp);
	fclose(fp);
	
	StringUnion(TotalUpdate, "\n");
	if (DEBUG_ENABLED){printf("WriteFile: before write\n");}
	fputs(TotalUpdate, logFilePointer);
	if (DEBUG_ENABLED){printf("WriteFile: after write\n");}
	StringClean(TotalUpdate, 512);
}

//format: {"T0":000,"P0":000,"M0RPM":0000,"M0ON":000,"M0OFF":000,"W0":0000,"STIME":000000,"SMODE":00,"SID":000}

void ReadFile(void){
	if (DEBUG_ENABLED){printf("ReadFile\n");}
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
	if (fp != NULL){
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
	
	if (DEBUG_ENABLED){printf("ReadFile: T0: %d, P0: %d, M0RPM: %d, M0ON: %d, M0OFF: %d, W0: %d, STIME: %d, SMODE: %d, SID: %d\n", setTemp, setPress, setMotorRpm, setMotorOn, setMotorOff, setWeight, setTime, setMode, setStepId);}
	
	if(setTemp>200) {setTemp=200;}
	if(setPress>200) {setPress=200;}
	if (setMotorOn>0 && setMotorOn<2) setMotorOn=2;
	if (setMotorOff>0 && setMotorOff<2) setMotorOff=2;
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






/* Read the configuration
 *
 */
void ReadConfigurationFile(void){
	if (DEBUG_ENABLED){printf("ReadConfigurationFile...\n");}
	
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
				if (DEBUG_ENABLED){printf("\tline with # found\n");}
				while (c != '\n'){
					c = fgetc(fp);
				}
			} else if (c == '\n'){
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
				
				if (DEBUG_ENABLED){printf("\tkey: %s, value: %s\n", keyString, valueString);}
				
				//ParseConfigValue
				if(strcmp(keyString, "ForceValue0") == 0){
					ForceValue0 = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tForceValue0: %d\n", ForceValue0);} // (old: %d)
				} else if(strcmp(keyString, "ForceValue100") == 0){
					ForceValue100 = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tForceValue100: %d\n", ForceValue100);} // (old: %d)
				} else if(strcmp(keyString, "PressValue0") == 0){
					PressValue0 = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tPressValue0: %d\n", PressValue0);} // (old: %d)
				} else if(strcmp(keyString, "PressValue100") == 0){
					PressValue100 = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tPressValue100: %d\n", PressValue100);} // (old: %d)
				} else if(strcmp(keyString, "TempValue0") == 0){
					TempValue0 = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tTempValue0: %d\n", TempValue0);} // (old: %d)
				} else if(strcmp(keyString, "TempValue100") == 0){
					TempValue100 = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tTempValue100: %d\n", TempValue100);} // (old: %d)
				} else if(strcmp(keyString, "GainScale_1") == 0){
					ptr = 0;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (DEBUG_ENABLED){printf("\tGainScale_1: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "GainScale_2") == 0){
					ptr = 1;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (DEBUG_ENABLED){printf("\tGainScale_2: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "GainScale_3") == 0){
					ptr = 2;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (DEBUG_ENABLED){printf("\tGainScale_3: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "GainScale_4") == 0){
					ptr = 3;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (DEBUG_ENABLED){printf("\tGainScale_4: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "GainPress") == 0){
					ptr = 4;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (DEBUG_ENABLED){printf("\tGainPress: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "GainTemp") == 0){
					ptr = 5;
					newADCConfig[ptr] = StringConvertToNumber(valueString);
					newADCConfig[ptr] = POWNTimes(newADCConfig[ptr], 2)<<9 | 1<<8 | 1<<4 | ptr;
					if (DEBUG_ENABLED){printf("\tGainTemp: %04X\n", newADCConfig[ptr]);} // (old: %d)
				} else if(strcmp(keyString, "BeepWeightReached") == 0){
					BeepWeightReached = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tBeepWeightReached: %d\n", BeepWeightReached);} // (old: %d)
				} else if(strcmp(keyString, "BeepStepEnd") == 0){
					BeepStepEnd = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tBeepStepEnd: %d\n", BeepStepEnd);} // (old: %d)
				} else if(strcmp(keyString, "doRememberBeep") == 0){
					doRememberBeep = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tdoRememberBeep: %d\n", doRememberBeep);} // (old: %d)
				} else if(strcmp(keyString, "DeleteLogOnStart") == 0){
					DeleteLogOnStart = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tDeleteLogOnStart: %d\n", DeleteLogOnStart);} // (old: %d)
				} else if(strcmp(keyString, "LogFile") == 0){
					//TODO: alloc new mem
					//logFile = valueString;
					if (DEBUG_ENABLED){printf("\tLogFile: %s\n", logFile);} // (old: %s)
				} else if(strcmp(keyString, "CommandFile") == 0){
					//TODO: alloc new mem
					//commandFile = valueString;
					if (DEBUG_ENABLED){printf("\tCommandFile: %s\n", commandFile);} // (old: %s)
				} else if(strcmp(keyString, "StatusFile") == 0){
					//TODO: alloc new mem
					//statusFile = valueString;
					if (DEBUG_ENABLED){printf("\tStatusFile: %s\n", statusFile);} // (old: %s)
				} else if(strcmp(keyString, "LowTemp") == 0){
					LowTemp = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tLowTemp: %d\n", LowTemp);} // (old: %d)
				} else if(strcmp(keyString, "LowPress") == 0){
					LowPress = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tLowPress: %d\n", LowPress);} // (old: %d)
				} else if(strcmp(keyString, "LongDelay") == 0){
					LongDelay = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tLongDelay: %d\n", LongDelay);} // (old: %d)
				} else if(strcmp(keyString, "ShortDelay") == 0){
					ShortDelay = StringConvertToNumber(valueString);
					if (DEBUG_ENABLED){printf("\tShortDelay: %d\n", ShortDelay);} // (old: %d)
				} else {
					if (DEBUG_ENABLED){printf("\tkey not Found\n");}
				}
				StringClean(keyString, 30);
				StringClean(valueString, 100);
			}
		}
	}
	ForceScaleFactor=100.0/((float)ForceValue100-(float)ForceValue0);
	PressScaleFactor=100.0/((float)PressValue100-(float)PressValue0);
	PressOffset=PressValue0;
	TempScaleFactor=100.0/((float)TempValue100-(float)TempValue0);
	TempOffset=TempValue0;
	
	if (DEBUG_ENABLED){printf("ForceScaleFactor: %f, PressScaleFactor: %f, PressOffset: %d, TempScaleFactor: %f, TempOffset: %d\n", ForceScaleFactor, PressScaleFactor, PressOffset, TempScaleFactor, TempOffset);}
	
	fclose(fp);
	if (DEBUG_ENABLED){printf("done.\n");}
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




































		
	
	
	
	
	
	