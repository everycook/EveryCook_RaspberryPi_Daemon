#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <wiringPi.h>
#include <softPwm.h>
#include <string.h>

#include "basic_functions.h"
#include "firmware.h"



//Other functions
void NumberConvertToString(uint32_t num, char *str);
void StringClean(char *str, uint32_t len);
void StringUnion(char *fristString, char *secondString);
uint32_t StringConvertToNumber(char *str);
int POWNTimes(uint32_t num, uint8_t n);

//	uint32_t adc0;
//uint8_t structregpointer = 0;
char TotalUpdate[512];

//HardwareTimer FreqTimer(3); //timer 2 conflicts with servo!!!
















//pins for the final board
const int Scale1Pin = 8; //one of the scale sensors
const int Scale2Pin = 9; //one of the scale sensors 
const int Scale3Pin = 6; //one of the scale sensors
const int Scale4Pin = 7; //one of the scale sensors
const int TempPin = 11; //Temperature at the Bottom
const int PressPin = 10; //pressure sensor pin
const int ServoPin = 26; //Servo for valve
const int RPMPin = 25;   //Motor RPM 
//const int RotDirPin = 31;   //Motor Rotational Direction 1=cw 0=ccw (for later)
const int EncoderPin = 30; //Encoder on Motor
const int HeatStartPin = 28;   //Heating start
const int HeatStopPin = 29;   //Heating stop
const int BeepPin = 27;   //Beeper
const int SegAPin=  21; //7 segment display
const int SegBPin=  22; //7 segment display
const int SegCPin=  16; //7 segment display
const int SegDPin=  17; //7 segment display
const int SegEPin=  18; //7 segment display
const int SegFPin=  19; //7 segment display
const int SegGPin=  20; //7 segment display
const int SegDPPin= 15; //7 segment display
/*
//pins for the simulation board
const int Scale1Pin = 3; //one of the scale sensors
const int Scale2Pin = 4; //one of the scale sensors
const int Scale3Pin = 5; //one of the scale sensors
const int Scale4Pin = 6; //one of the scale sensors
const int TempPin = 7; //Temperature at the Bottom
const int PressPin = 10; //pressure sensor pin
const int ServoPin = 11; //Servo for valve
const int RPMPin = 15;   //Motor RPM
const int EncoderPin = 26; //Encoder on Motor 
const int HeatStartPin = 14;   //Heating start
const int HeatStopPin = 12;   //Heating stop
const int BeepPin = 13;   //Beeper 27
const int SegAPin=  30; // 7 segment display
const int SegBPin=  22; //7 segment display
const int SegCPin=  19; // 7 segment display
const int SegDPin=  20; //7 segment display
const int SegEPin=  21; //7 segment display
const int SegFPin=  31; //7 segment display
const int SegGPin=  25; //7 segment display
const int SegDPPin=  18; //7 segment display
*/
int DeltaT=0;
unsigned int StepID =0;
float ReferenceForce=0; //the reference to get the zero of the scale
unsigned int SetWeight = 0;
float Weight = 0;
int ScaleReady = 0;
const unsigned int LowTemp=40;

unsigned int SetPress=0;
float Press=0;
const unsigned int LowPress=40;
//delay for normal operation and scale mode
int LongDelay=500;
int ShortDelay=1;
int Delay=0;


int HeatPowerStatus = 0; //Induction heater power 0=off >0=on
unsigned int NextTempCheck=0; //when did we do the last Temperature Check???
unsigned int RunTime; //we need a runtime in seconds

unsigned int StepRunTime = 0;
unsigned int StepEndTime = 0;


unsigned int MotorRpm =0; //0-255 
unsigned int SetMotorRpm =0; //0-255 
//int MotorDir =0; //0=cw 1=ccw
unsigned int MotorStartTime =0; //When did we start the motor?
unsigned int SetMotorOff =0;
unsigned int SetMotorOn =0;
unsigned int MotorOff =0;
unsigned int MotorOn =0;

int SetMode=0;
int Mode=0;

float ForceScaleFactor = 4; //12.33 Conversion between digital Units and grams
//float ForceScaleFactor = 1;
unsigned int RemainTime=0;
unsigned int BeepEndTime=0; //when to stop the beeper

//hardware timer stuff...
//volatile bool LastEncoder=false;
//volatile int EncoderChanges=0;
//unsigned int LastEncoderChanges=0;
unsigned int PrescaleFactor=1024; //15
unsigned int Overflow=300;

//Servo stuff
int ValveOpenAngle=30;
int ValveClosedAngle=90;
int ValveSetValue=0;

//Options
int doRememberBeep = 0;

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





unsigned int setTemp = 0;
unsigned int setPress = 0;
unsigned int setMotorRpm = 0;
unsigned int setMotorOn = 0;
unsigned int setMotorOff = 0;
unsigned int setWeight = 0;
unsigned int setTime = 0;
unsigned int setMode = 0;
unsigned int setStepId = 0;

unsigned int temp = 0; //float
unsigned int press = 0;
unsigned int motorRpm = 0;
unsigned int motorOn = 0;
unsigned int motorOff = 0;
unsigned int weight = 0;
unsigned int time = 0;
unsigned int mode = 0;
unsigned int stepId = 0;

unsigned int oldMode = 0;
unsigned int oldTemp = 0;
unsigned int oldPress = 0;
unsigned int oldWeight = 0;

unsigned int isBuzzing = 1; //to be sure first send a off to buzzer
const int ALLWAYS_STOP_BUZZING = 0;

char oldSegmentDisplay = ' ';
unsigned int blickState = 0;


const char *commandFile = "/var/www/writefile.txt";
const char *statusFile = "/var/www/readfile.txt";






























int main(void){
	initHardware();
	delay(30);
	StringClean(TotalUpdate, 512);
	ReadConfigurationFile();
	
	//setFreqTimer();

	while (1){
		ReadFile();
		ProcessCommand();
		WriteFile();
		delay(Delay);
	}
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
		
		OptionControl();
	}
	TempControl();
	PressControl();
	MotorControl();
	ValveControl();
	ScaleFunction();
	
	if (StepEndTime > RunTime) {
		RemainTime=StepEndTime-RunTime;
	} else if (Mode<MIN_STATUS_MODE) {
		RemainTime=0;
		if (Mode==MODE_COOK || Mode == MODE_PRESSHOLD){
			Mode=MODE_COOK_TIMEEND;
		} else {
			Mode=MODE_STANDBY;
		}
	}
	SegmentDisplay();
	Beep();
}


/******************* functions to evaluate **********************/


uint32_t readTemp(){
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
	
	uint32_t TempValue = readADC(ADC_TEMP);
	//TODO do calculate from value to °C
	return TempValue;
}


uint32_t readPress(){
/*
	float PressValue=0;
	int AverageOf=1000;
	for (int i =0; i< AverageOf; i++){
		PressValue+=analogRead(PressPin); //read pressure
	}
	//      Press=map(PressValue/AverageOf,8,3320,0,100); //origin
	Press=map(PressValue/AverageOf,8,1250,0,100);
	//      Press=PressValue/AverageOf;
	*/
	
	uint32_t PressValue = readADC(ADC_PRESS);
	//TODO do calculate from value to kpa
	return PressValue;
}

//Power control functions
void HeatOn() {
	if (HeatPowerStatus==0) { //if its off
		//  SerialUSB.println("heaton");
		/*
		digitalWrite(HeatStopPin,LOW);
		digitalWrite(HeatStartPin,HIGH);
		delay(50);
		digitalWrite(HeatStartPin,LOW);
		*/
		
		writeControllButtonPin(IND_KEY4, 1);
		
		HeatPowerStatus=1; //save that we turned it on
	}
}

void HeatOff(){
	if (HeatPowerStatus>0) { //if its on
		//digitalWrite(HeatStopPin,HIGH);
		writeControllButtonPin(IND_KEY4, 0);
		HeatPowerStatus=0; //save that we turned it off
	}
}

void setMotorPWM(uint32_t pwm){
	writeI2CPin(I2C_MOTOR, pwm);
}

void setServoOpen(uint8_t open){
	if (open){
		writeI2CPin(I2C_SERVO, ValveOpenAngle);
	} else {
		writeI2CPin(I2C_SERVO, ValveClosedAngle);
	}
}

uint32_t readWeight(){
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
	SerialUSB.print(ReferenceForce);
	SerialUSB.print(" Sum ");
	SerialUSB.println(SumOfForces);
	* /
*/
	
	uint32_t WeightValue1 = readADC(ADC_WEIGHT1);
	uint32_t WeightValue2 = readADC(ADC_WEIGHT2);
	uint32_t WeightValue3 = readADC(ADC_WEIGHT3);
	uint32_t WeightValue4 = readADC(ADC_WEIGHT4);
	
	uint32_t WeightValue = (WeightValue1+WeightValue2+WeightValue3+WeightValue4) / 4;
	//TODO do calculate from value to g
	return WeightValue;
}

/******************* processing functions **********************/
void OptionControl(){
  if (Mode>=MODE_OPTIONS_BEGIN){
    if (Mode==MODE_OPTION_REMEMBER_BEEP_ON){
      doRememberBeep=1;
    } else if (Mode==MODE_OPTION_REMEMBER_BEEP_OFF){
      doRememberBeep=0;
    }
    Mode=MODE_STANDBY;
  }
}

void TempControl(){
	if (Mode<MIN_COOK_MODE || Mode>MAX_COOK_MODE) HeatOff();
	if (Mode>=MIN_TEMP_MODE && Mode<=MAX_TEMP_MODE) {
		if (RunTime>=NextTempCheck || HeatPowerStatus==0){
			HeatOff();
			delay(100);
			uint32_t TempValue = readTemp();
			oldTemp = temp;
			temp=TempValue;
			
			//SerialUSB.println("Temperature control: ");
			//SerialUSB.print(temp);  SerialUSB.print(" Degree C - Bottom Temperature");
			int DeltaT=setTemp-temp;
			if (Mode==MODE_HEADUP || Mode==MODE_COOK) {
				if (DeltaT<=0) {
					NextTempCheck=RunTime+1;
					if (Mode==MODE_HEADUP) {//heatup function
						StepEndTime=RunTime-2;
						Mode=MODE_HOT; //we are hot
					}
				}
				else if (DeltaT <= 10) { NextTempCheck=RunTime+5; HeatOn(); }
				else if (DeltaT <= 50) { NextTempCheck=RunTime+10; HeatOn();}
				else {NextTempCheck=RunTime+20; HeatOn();}
				//  SerialUSB.print("RunTime is now: ");SerialUSB.print(RunTime);SerialUSB.print(" s Next Temperature Check at: ");SerialUSB.print(NextTempCheck);SerialUSB.println(" s"); 
			}
			if (Mode==MODE_HEADUP && HeatPowerStatus==1) { //heatup
				StepEndTime=NextTempCheck+1;
			} else if (Mode==MODE_COOLDOWN && DeltaT>=0) { //cooldown function
				StepEndTime=RunTime-2;
				Mode=MODE_COLD;
			} else if (Mode==MODE_COOLDOWN) {
				NextTempCheck=RunTime+1;
				StepEndTime=NextTempCheck+1;
			}
		}
	}
}

void PressControl(){
	if (Mode<MIN_COOK_MODE || Mode>MAX_COOK_MODE) HeatOff();
	if (Mode>=MIN_PRESS_MODE && Mode<=MAX_PRESS_MODE) {
		if (RunTime>=NextTempCheck || HeatPowerStatus==0){
			HeatOff();
			delay(100);
			uint32_t pressValue = readPress();
			oldPress = press;
			press=pressValue;
			//SerialUSB.println("Pressure control: ");
			//SerialUSB.print(Press);  SerialUSB.print(" kPa Pressure");
			int DeltaP=setPress-Press;
			if (Mode==MODE_PRESSUP || Mode==MODE_PRESSHOLD) {
				if (DeltaP<=0) {
					NextTempCheck=RunTime+1;
					if (Mode==MODE_PRESSUP) {//pressup function
						StepEndTime=RunTime-2;
						Mode=MODE_PRESSURIZED; //we are pressurized
					}
				}
				else if (DeltaP <= 10) { NextTempCheck=RunTime+5; HeatOn(); }
				else if (DeltaP <= 50) { NextTempCheck=RunTime+10; HeatOn();}
				else {NextTempCheck=RunTime+20; HeatOn();}
				//SerialUSB.print("RunTime is now: ");SerialUSB.print(RunTime);SerialUSB.print(" s Next Pressure Check at: ");SerialUSB.print(NextTempCheck);SerialUSB.println(" s"); 
			}
			if (Mode==MODE_PRESSUP && HeatPowerStatus==1) { //pressure up
				StepEndTime=NextTempCheck+1;
			} else if (Mode==MODE_PRESSDOWN && DeltaP>=0) { //pressure down function
				StepEndTime=RunTime-2;
				Mode=MODE_PRESSURELESS;
			} else if (Mode==MODE_PRESSDOWN) {
				NextTempCheck=RunTime+1;
				StepEndTime=NextTempCheck+1;
			}
		}
	} else if (Mode>=MIN_TEMP_MODE && Mode<=MAX_TEMP_MODE) {
		if (RunTime>=NextTempCheck || HeatPowerStatus==0){
			//Press=map(analogRead(PressPin),0,2000,0,100); //read pressure
			uint32_t pressValue = readPress();
			oldPress = press;
			press=pressValue;
		}
	}
}

void MotorControl (){
	if (Mode==MODE_CUT || Mode>=MIN_TEMP_MODE){
		if (Mode==MODE_CUT) StepEndTime=RunTime+1;
		if (setMotorRpm > 0 && Mode<=MAX_STATUS_MODE) {
			//  SerialUSB.println("Motor Control");
			// SerialUSB.print("Encoder Changes"); SerialUSB.println(EncoderChanges);
			// SerialUSB.print("Last Encoder Changes"); SerialUSB.println(LastEncoderChanges);

			if  (setMotorOn > 0 && setMotorOff > 0) {
				if (RunTime >= MotorStartTime+setMotorOn && RunTime < MotorStartTime+setMotorOn+setMotorOff) { 
					MotorRpm = 0; 
				} else if (MotorRpm==0){
					MotorRpm = setMotorRpm;
					MotorStartTime=RunTime;
				}
				//  SerialUSB.println("Interval Mode");
			} else {
				MotorRpm = setMotorRpm;
				MotorStartTime=RunTime;
			}
			/*  SerialUSB.print("RunTime: ");  SerialUSB.print(RunTime);  SerialUSB.print(" MotorStartTime: ");  SerialUSB.print(MotorStartTime);
			SerialUSB.print(" MotorRunTime: ");  SerialUSB.print(setMotorOn);  SerialUSB.print(" MotorPauseTime: ");  SerialUSB.print(setMotorOff);
			SerialUSB.print(" Motor Rpm: ");  SerialUSB.println(MotorRpm);
			*/
		}
	} else {
		//SerialUSB.println(" Motor off: ");
		MotorRpm=0;
	}
	/*
	if (MotorRpm>0 && LastEncoderChanges==EncoderChanges){
		Mode=42;
		MotorRpm=0;
	}
	*/
	//pwmWrite(RPMPin,MotorRpm);
	setMotorPWM(MotorRpm);
	//LastEncoderChanges=EncoderChanges;
}

void ValveControl(){
	if (Mode==MODE_PRESSVENT) {
		HeatOff();
		//servo1.write(ValveOpenAngle);
		setServoOpen(1);
		StepEndTime=RunTime+1;
		if (Press < LowPress) {
			delay(10000);
			Mode=MODE_PRESSURELESS;
		}
	} else {
		//servo1.write(ValveClosedAngle);
		setServoOpen(0);
	}
}


void ScaleFunction () {
	if (Mode==MODE_SCALE || Mode==MODE_WEIGHT_REACHED){
		StepEndTime=RunTime+2;
		uint32_t SumOfForces = readWeight();
		/*
		SerialUSB.print("Time ");
		SerialUSB.println(millis());
		SerialUSB.println(millis()-lastmillis);
		*/
		if (ScaleReady==0) { //we are not ready for weighting
			//SerialUSB.println("referencing");
			ReferenceForce=SumOfForces;
			ScaleReady=1;
			if (Delay == ShortDelay){
				Delay=LongDelay;
			}
		} else { //we have a reference and are ready
			Delay=ShortDelay;
			
			oldWeight = weight;
			weight=(SumOfForces-ReferenceForce);
			/*SerialUSB.print("Actual weight: ");
			SerialUSB.println(Weight);
			SerialUSB.print("set weight: ");
			SerialUSB.print(setWeight);
			SerialUSB.println(" g");*/
			if (Weight>=setWeight) {//If we have reached the required mass
				if (Mode != MODE_WEIGHT_REACHED){
					BeepEndTime=RunTime+1;
				}
				Mode=MODE_WEIGHT_REACHED;
				RemainTime=0;
				//SerialUSB.println("Mass reached");
			}
		}
	} else if (Delay == ShortDelay){
		ScaleReady=0;
		ReferenceForce=0;
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
	/*if (Mode==53) {
		digitalWrite(Seg1Pin,HIGH);
		digitalWrite(Seg2Pin,LOW);
		return;
	}*/ 
	
	//SegmentDisplaySimple();
	SegmentDisplayOptimized();
	
	//togglePin(I2C_7SEG_PERIOD);
	if (blickState){
		writeI2CPin(I2C_7SEG_PERIOD,LOW);
		blickState = 0;
	} else {
		blickState = 1;
		writeI2CPin(I2C_7SEG_PERIOD,HIGH);
	}
}

void SegmentDisplaySimple(){
	if (Press>LowPress){
		oldSegmentDisplay = 'P';
		writeI2CPin(I2C_7SEG_TOP,HIGH);
		writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
		writeI2CPin(I2C_7SEG_CENTER,HIGH);
		writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
		writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
		writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,LOW);
		writeI2CPin(I2C_7SEG_BOTTOM,LOW);
	} else if (temp>LowTemp){
		oldSegmentDisplay = 'H';
		writeI2CPin(I2C_7SEG_TOP,LOW);
		writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
		writeI2CPin(I2C_7SEG_CENTER,HIGH);
		writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
		writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
		writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,HIGH);
		writeI2CPin(I2C_7SEG_BOTTOM,LOW);
	} else {
		oldSegmentDisplay = '0';
		writeI2CPin(I2C_7SEG_TOP,HIGH);
		writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
		writeI2CPin(I2C_7SEG_CENTER,LOW);
		writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
		writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
		writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,HIGH);
		writeI2CPin(I2C_7SEG_BOTTOM,HIGH);
	}
}

void SegmentDisplayOptimized(){
	if (Press>LowPress){
		char curSegmentDisplay = 'P';
		if (curSegmentDisplay != oldSegmentDisplay){
			if (oldSegmentDisplay == 'H'){
				writeI2CPin(I2C_7SEG_TOP,HIGH);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
				//writeI2CPin(I2C_7SEG_CENTER,HIGH);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,LOW);
				//writeI2CPin(I2C_7SEG_BOTTOM,LOW);
			} else if (oldSegmentDisplay == '0'){
				//writeI2CPin(I2C_7SEG_TOP,HIGH);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
				writeI2CPin(I2C_7SEG_CENTER,HIGH);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,LOW);
				writeI2CPin(I2C_7SEG_BOTTOM,LOW);
			} else if (oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(I2C_7SEG_TOP,HIGH);
				writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
				writeI2CPin(I2C_7SEG_CENTER,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
				writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,LOW);
				writeI2CPin(I2C_7SEG_BOTTOM,LOW);
			}
			oldSegmentDisplay = curSegmentDisplay;
		}
	} else if (temp>LowTemp){
		char curSegmentDisplay = 'H';
		if (curSegmentDisplay != oldSegmentDisplay){
			if (oldSegmentDisplay == 'P'){
				writeI2CPin(I2C_7SEG_TOP,LOW);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
				//writeI2CPin(I2C_7SEG_CENTER,HIGH);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,HIGH);
				//writeI2CPin(I2C_7SEG_BOTTOM,LOW);
			} else if (oldSegmentDisplay == '0'){
				writeI2CPin(I2C_7SEG_TOP,LOW);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
				writeI2CPin(I2C_7SEG_CENTER,HIGH);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
				//writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM,LOW);
			} else if (oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(I2C_7SEG_TOP,LOW);
				writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
				writeI2CPin(I2C_7SEG_CENTER,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
				writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM,LOW);
			}
			oldSegmentDisplay = curSegmentDisplay;
		}
	} else {
		char curSegmentDisplay = '0';
		if (curSegmentDisplay != oldSegmentDisplay){
			if (oldSegmentDisplay == 'H'){
				writeI2CPin(I2C_7SEG_TOP,HIGH);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
				writeI2CPin(I2C_7SEG_CENTER,LOW);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
				//writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM,HIGH);
			} else if (oldSegmentDisplay == 'P'){
				//writeI2CPin(I2C_7SEG_TOP,HIGH);
				//writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
				writeI2CPin(I2C_7SEG_CENTER,LOW);
				//writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
				//writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM,HIGH);
			} else if (oldSegmentDisplay == ' '){
				//Is initial do all
				writeI2CPin(I2C_7SEG_TOP,HIGH);
				writeI2CPin(I2C_7SEG_TOP_LEFT,HIGH);
				writeI2CPin(I2C_7SEG_CENTER,LOW);
				writeI2CPin(I2C_7SEG_BOTTOM_LEFT,HIGH);
				writeI2CPin(I2C_7SEG_TOP_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM_RIGHT,HIGH);
				writeI2CPin(I2C_7SEG_BOTTOM,HIGH);
			}
			oldSegmentDisplay = curSegmentDisplay;
		}
	}
}

void Beep(){
	if(doRememberBeep==1){
		if (RunTime>StepEndTime){
			if (Mode>=30 && Mode<=39){
				int toLateTime = RunTime-StepEndTime;
				if (toLateTime==1){
					BeepEndTime=RunTime+1;
				} else if (toLateTime==5){
					BeepEndTime=RunTime+1;
				} else if (toLateTime==30){
					BeepEndTime=RunTime+1;
				} else if (toLateTime==60){
					BeepEndTime=RunTime+1;
				} else if (toLateTime>0 && toLateTime%300==0){ //each 5 min
					BeepEndTime=RunTime+5;
				}
			}
		}
	}
	
	//if (RunTime<BeepEndTime) digitalWrite(BeepPin,HIGH);
	//else digitalWrite(BeepPin,LOW);
	if (RunTime<BeepEndTime){
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
void WriteFile(void)
{
	char tempString[10];
	FILE *fp;
	
	StringUnion(TotalUpdate, "{");
	StringUnion(TotalUpdate, "\"T0\":");
	NumberConvertToString(temp, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	
	StringUnion(TotalUpdate, "\"P0\":");
	NumberConvertToString(press, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	
	StringUnion(TotalUpdate, "\"M0RPM\":");
	NumberConvertToString(motorRpm, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	
	StringUnion(TotalUpdate, "\"M0ON\":");
	NumberConvertToString(motorOn, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	
	StringUnion(TotalUpdate, "\"M0OFF\":");
	NumberConvertToString(motorOff, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	
	StringUnion(TotalUpdate, "\"W0\":");
	NumberConvertToString(weight, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	
	StringUnion(TotalUpdate, "\"STIME\":");
	NumberConvertToString(time, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	
	StringUnion(TotalUpdate, "\"SMODE\":");
	NumberConvertToString(mode, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, ",");
	
	StringUnion(TotalUpdate, "\"SID\":");
	NumberConvertToString(stepId, tempString);
	StringUnion(TotalUpdate, tempString);
	StringUnion(TotalUpdate, "}");
	
	
	fp = fopen(statusFile, "w");
	fputs(TotalUpdate, fp);
	StringClean(TotalUpdate, 512);
	fclose(fp);
}

//format: {"T0":000,"P0":000,"M0RPM":0000,"M0ON":000,"M0OFF":000,"W0":0000,"STIME":000000,"SMODE":00,"SID":000}

void ReadFile(void){
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
	if(setTemp>200) {setTemp=200;}
	if(setPress>200) {setPress=200;}
	if (setMotorOn>0 && setMotorOn<2) setMotorOn=2;
	if (setMotorOff>0 && setMotorOff<2) setMotorOff=2;
}

/* Convert a number to a string
 *
 */
void NumberConvertToString(uint32_t num, char *str)
{
	uint8_t i = 0;
	uint32_t temp, mutiplecand = 1;

	temp = num;
	do
	{
		mutiplecand = mutiplecand*10;
		i++;
	}while (temp >= mutiplecand || i > 9); //TODO: this would get en endloos loop it i>9...
	
	while (mutiplecand != 1)
	{
		mutiplecand = mutiplecand/10;
		*str++ = num/mutiplecand+48;
		num = num%mutiplecand;
	}
}
/* Clean the string
 *
 */
void StringClean(char *str, uint32_t len)
{
	uint32_t i = 0;

	for (; i< len; i++)
		str[i] = 0x00;
}
/* Combine two strings to one string
 *
 */
void StringUnion(char *fristString, char *secondString)
{
	uint8_t i = 0, fristEndPtr = 0;

	while (fristString[fristEndPtr]) fristEndPtr++;
	while (secondString[i])
	{
		fristString[fristEndPtr+i] = secondString[i];
		i++;
 	}
}
/* convert a string to a number
 *
 */
uint32_t StringConvertToNumber(char *str)
{
	uint32_t temp = 0 ,len = 0, mutiple = 1;

	while (str[len]) 
	{
		len++;
		mutiple *= 10;
	}
	len = 0;	
	while (str[len])
	{
		mutiple = mutiple/10;
		temp = temp + (str[len]-48)*mutiple;
		len++;
	}
	return temp;
}






/* Read the configuration of the amp of the reference voltage
 *
 */
void ReadConfigurationFile(void)
{
/*
	FILE *fp;
	char tempString[10];
	uint8_t i = 0;
	uint8_t ptr = 0;
	char c;
	StringClean(tempString, 10);
	fp = fopen("config.txt", "r");
	if (fp != NULL)
	{
		while ((c = fgetc(fp)) != 255)
		{
			if (c == '=') 
			{
				c = fgetc(fp);
				while (c != ';')
				{
					tempString[i] = c;
					c = fgetc(fp);
					i++;
				}
				i = 0;
				if (c == ';')
				{					
					ConfigurationReg[ptr] = StringConvertToNumber(tempString);
					ConfigurationReg[ptr] = POWNTimes(ConfigurationReg[ptr], 2)<<9   	 |
								1<<8 				 |
								1<<4 				 |
								ptr;
//printf("%d\n", ConfigurationReg[ptr]);
					StringClean(tempString, 10);
					ptr++;
				}
			}
		}
	}
	fclose(fp);
*/
}
/*
*/
int POWNTimes(uint32_t num, uint8_t n)
{
	int i = 0;

	while (num > 1)
	{
		num = num / n;
		i++;
	}
	return i;
}






































		
	
	
	
	
	
	