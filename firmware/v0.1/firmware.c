#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <wiringPi.h>
#include <softPwm.h>
#include <string.h>


//Pin defination for the SPI
#define MOSI	10
#define MISO	9
#define SCLK	11
#define CS	8
//Pin defination for the I2C
#define SDA	      2
#define SCL	      3
//AD774 Register defination
#define READ_STATUS_REG		0x40
#define READ_MODE_REG		0x48
#define WRITE_MODE_REG		0x08
#define READ_STRUCT_REG		0x50
#define WRITE_STRUCT_REG	0x10
#define READ_DATA_REG		0x58
#define READ_SERIAL_REG		0x60
#define READ_IO_REG 		0x68
#define WRITE_IO_REG		0x28
#define READ_SHIFT_REG		0x70
//I2C_DELAY_TIME
#define DELAY_TIME   10


//virtualSPI function delaration
void VirtualSPIInit(void);
void SPIReset(void);
void SPIWrite(uint8_t data);	
uint8_t SPIRead(void);
void SPIWriteByte(uint8_t reg, uint8_t data);
void SPIWrite2Bytes(uint8_t reg, uint32_t data);
uint8_t SPIReadByte(uint8_t register);
uint32_t SPIRead2Bytes(uint8_t register);
uint32_t SPIRead3Bytes(uint8_t register);
//virtualI2C function delaration
void VirtualI2CInit(void);
void I2CStart(void);
void I2CStop(void);
int CheckAck(void);
uint8_t I2CReadByte(void);
void I2CWriteByte(uint8_t data);
void I2CWriteBytes(uint8_t *data, uint8_t len);
//GPIO PCA9685 initialization
void GPIOInit(void);
void AD7794Init(void);
void PCA9685Init(void);




uint32_t readADC(uint8_t i);
uint32_t readSignPin(uint8_t i);
uint32_t readRaspberryPin(uint8_t i);
void writeControllButtonPin(uint8_t i, uint8_t on);
void writeRaspberryPin(uint8_t i, uint8_t on);
void buzzer(uint8_t on, uint32_t pwm);
void writeI2CPin(uint8_t i, uint32_t value);



//Other functions
void ProcessCommand(void);
void WriteFile(void);
void ReadFile(void);
void ReadConfigurationFile(void);
void NumberConvertToString(uint32_t num, char *str);
void StringClean(char *str, uint32_t len);
void StringUnion(char *fristString, char *secondString);
uint32_t StringConvertToNumber(char *str);
int POWNTimes(uint32_t num, uint8_t n);

//	uint32_t adc0;
//uint8_t structregpointer = 0;
char TotalUpdate[512];

//HardwareTimer FreqTimer(3); //timer 2 conflicts with servo!!!


void OptionControl();
void TempControl();
void PressControl();
void MotorControl();
void ValveControl();
void ScaleFunction();
void setFreqTimer();
void FreqHandler(); 
void SegmentDisplay();
void SegmentDisplaySimple();
void SegmentDisplayOptimized();
void Beep();
















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




uint32_t ConfigurationReg[] = {0x710, 0x711, 0x712, 0x713, 0x714, 0x115};//For AD7794 conf-reg

uint8_t PinNumSign[] = {24, 25, 27, 22, 7, 17};

uint8_t PinNumControllButtons[] = {14, 15, 18, 23};

const int IND_KEY1 = 0; //14
const int IND_KEY2 = 1; //15
const int IND_KEY3 = 2; //18
const int IND_KEY4 = 3; //23

const int ADC_WEIGHT1 = 0;
const int ADC_WEIGHT2 = 1;
const int ADC_WEIGHT3 = 2;
const int ADC_WEIGHT4 = 3;
const int ADC_PRESS = 4;
const int ADC_TEMP = 5;

const int I2C_MOTOR = 0;
const int I2C_SERVO = 1;
const int I2C_7SEG_TOP = 2;				//SegAPin
const int I2C_7SEG_TOP_LEFT = 3;		//SegBPin
const int I2C_7SEG_TOP_RIGHT = 4;		//SegFPin
const int I2C_7SEG_CENTER = 5;			//SegGPin
const int I2C_7SEG_BOTTOM_LEFT = 6;		//SegEPin
const int I2C_7SEG_BOTTOM_RIGHT = 7;	//SegCPin
const int I2C_7SEG_BOTTOM = 8;			//SegDPin
const int I2C_7SEG_PERIOD = 9;			//SegDPPin

const int PI_PIN_BUZZER = 4; //7?
const int BUZZER_PWM = 1000;

const char *commandFile = "/var/www/writefile.txt";
const char *statusFile = "/var/www/readfile.txt";


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



const int MIN_COOK_MODE = 10;
const int MAX_COOK_MODE = 29;

const int MIN_TEMP_MODE = 10;
const int MAX_TEMP_MODE = 19;

const int MIN_PRESS_MODE = 20;
const int MAX_PRESS_MODE = 29;

const int MIN_STATUS_MODE = 30;
const int MAX_STATUS_MODE = 39;

const int MIN_ERROR_MODE = 40;
const int MAX_ERROR_MODE = 49;


const int MODE_STANDBY = 0;
const int MODE_CUT = 1;			//STIME, MORPM(, MOON, MOOFF)
const int MODE_SCALE = 2;		//W0, STIME

const int MODE_HEADUP = 10; 	//T0, STIME, MORPM, MOON, MOOFF
const int MODE_COOK = 11; 		//T0, STIME, MORPM, MOON, MOOFF
const int MODE_COOLDOWN = 12; 	//T0, STIME, MORPM, MOON, MOOFF

const int MODE_PRESSUP = 20;	//P0, STIME, MORPM, MOON, MOOFF
const int MODE_PRESSHOLD = 21;	//P0, STIME, MORPM, MOON, MOOFF
const int MODE_PRESSDOWN = 22;	//P0, STIME, MORPM, MOON, MOOFF
const int MODE_PRESSVENT = 23;	//P0, STIME, MORPM, MOON, MOOFF

const int MODE_HOT = 30;
const int MODE_PRESSURIZED = 31;
const int MODE_COLD = 32;
const int MODE_PRESSURELESS = 33;
const int MODE_WEIGHT_REACHED = 34;
const int MODE_COOK_TIMEEND = 35;
const int MODE_RECIPE_END = 39;

const int MODE_INPUT_ERROR = 40;
const int MODE_EMERGANCY_SHUTDOWN = 41;
const int MODE_MOTOR_OVERLOAD = 42;

const int MODE_WATCHDOW = 50;
const int MODE_COMMUNICATION_ERROR = 53;



const int MODE_OPTIONS_BEGIN = 100;
const int MODE_OPTION_REMEMBER_BEEP_ON = 100;
const int MODE_OPTION_REMEMBER_BEEP_OFF = 101;

//$#define DEBUG_ENABLED = 1;
const int DEBUG_ENABLED = 1;



























int main(void){
	VirtualSPIInit();
	VirtualI2CInit();
	GPIOInit();
	PCA9685Init();
	AD7794Init();
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






































uint8_t Data[10];

/*******************PI Driver Code**********************/
/* VirtualSPIInit
 */
void VirtualSPIInit(void){
	wiringPiSetupGpio();
	
	pinMode(MOSI, OUTPUT);
	pinMode(MISO, INPUT);
	pinMode(SCLK, OUTPUT);
	pinMode(CS, OUTPUT);	
	delay(30);
}
/* SPIReset: Reset the AD7794 chip, write 4 0xff.
 *
 */
void SPIReset(void){
	digitalWrite(CS, LOW);
	SPIWrite(0xff);
	SPIWrite(0xff);
	SPIWrite(0xff);
	SPIWrite(0xff);
	digitalWrite(CS, HIGH);	
}
/* SPIWrite: Write one byte data to the register in AD7794
 * data:datas write to register
 */
void SPIWrite(uint8_t data){
	int i = 7;
	
	for (i = 7; i >= 0; i--){
		digitalWrite(SCLK, LOW);	
		if (data & (1<<i)){
			digitalWrite(MOSI, HIGH);
		} else {
			digitalWrite(MOSI, LOW);
		}
		delayMicroseconds(100);
		digitalWrite(SCLK, HIGH);
		delayMicroseconds(100);		
	}	
}
/* SPIRead: Read one byte data from the AD7794
 * return: data from register
 */
uint8_t SPIRead(void){
	int i = 7;
	uint8_t temp = 0;
	
	for (i = 7; i >= 0; i--){
		digitalWrite(SCLK, LOW);
		delayMicroseconds(100);
		digitalWrite(SCLK, HIGH);
		delayMicroseconds(100);
		if (digitalRead(MISO)){
			temp = temp | (1<<i);
		}
	}

	return temp;
}
/* SPIWriteByte: Wirte one byte to denstination register. 
 * 
 */
void SPIWriteByte(uint8_t reg, uint8_t data){
	digitalWrite(CS, LOW);
	SPIWrite(reg);
	SPIWrite(data);
	digitalWrite(CS, HIGH);
}
/* SPIWrite2Bytes: Wirte one byte to denstination register. 
 * 
 */
void SPIWrite2Bytes(uint8_t reg, uint32_t data){
	digitalWrite(CS, LOW);
	SPIWrite(reg);
	SPIWrite((data>>8)&0xff);
	SPIWrite(data&0xff);
	digitalWrite(CS, HIGH);
}
/* SPIReadByte: Get one byte from denstination register. 
 * return: 1 byte data
 */
uint8_t SPIReadByte(uint8_t reg){
	uint8_t data;

	digitalWrite(CS, LOW);
	SPIWrite(reg);
	data = SPIRead();
	digitalWrite(CS, HIGH);

	return data;
}
/* SPIReadByte: Get two bytes from denstination register. 
 * return: 2 bytes data
 */
uint32_t SPIRead2Bytes(uint8_t reg){
	uint32_t data;

	digitalWrite(CS, LOW);
	SPIWrite(reg);
	data = SPIRead();
	data = (data << 8)| SPIRead();
	digitalWrite(CS, HIGH);
	
	return data;
}
/* SPIReadByte: Get three bytes from denstination register. 
 * return: 3 bytes data
 */
uint32_t SPIRead3Bytes(uint8_t reg){
	uint32_t data;

	digitalWrite(CS, LOW);
	SPIWrite(reg);
	data = SPIRead();
	data = (data << 8)| SPIRead();
	data = (data << 8)| SPIRead();
	digitalWrite(CS, HIGH);

	return data;
}
/* VirtualI2CInit: Initializate the virtual I2C-BUS protocal
 *
 */
void VirtualI2CInit(void){
	wiringPiSetupGpio();
	pinMode(SDA, OUTPUT);
	pinMode(SCL, OUTPUT);
}
/* I2CStart: Simulate the I2C start
 *
 */ 
void I2CStart(void){
	digitalWrite(SDA, HIGH);
	delayMicroseconds(DELAY_TIME);
	digitalWrite(SCL, HIGH);
	delayMicroseconds(DELAY_TIME);
	digitalWrite(SDA, LOW);
	delayMicroseconds(DELAY_TIME);
	digitalWrite(SCL, LOW);
	delayMicroseconds(DELAY_TIME);
}
/* I2CStop: Simulate the I2C stop
 *
 */
void I2CStop(void){
	digitalWrite(SDA, LOW);
	delayMicroseconds(DELAY_TIME);
	digitalWrite(SCL, HIGH);
	delayMicroseconds(DELAY_TIME);
	digitalWrite(SDA, HIGH);
	delayMicroseconds(DELAY_TIME);
	digitalWrite(SCL, LOW);
	delayMicroseconds(DELAY_TIME);
}
/* CheckAck: Checking Acknowleage in the SDA
 * return:1- Got the ACK, 0-Did got the ACK
 */
int CheckAck(void){
	int temp;
	
	digitalWrite(SDA, HIGH);
	delayMicroseconds(DELAY_TIME);
	digitalWrite(SCL, HIGH);
	delayMicroseconds(DELAY_TIME);
	pinMode(SDA, INPUT);
	delayMicroseconds(1);
	temp = digitalRead(SDA);
	pinMode(SDA, OUTPUT);
	digitalWrite(SCL, LOW);
	delayMicroseconds(DELAY_TIME);
	if (temp == 1){
		return 0;
	}
	return 1;
}
/* I2CWriteByte: Write one Byte to the PCA9685
 * data:Data to the PCA9685
 */
void I2CWriteByte(uint8_t data){
	uint8_t i = 0;

	for (; i < 8; ++i){
		if ((data&0x80)==0x80){
			digitalWrite(SDA, HIGH);
		} else {
			digitalWrite(SDA, LOW);
		}
		data = data << 1;
		delayMicroseconds(DELAY_TIME);		
		digitalWrite(SCL, HIGH);
		delayMicroseconds(DELAY_TIME);
		digitalWrite(SCL, LOW);
		delayMicroseconds(DELAY_TIME);
	}
	if (!CheckAck()){
		I2CStop();
	}
}
/* I2CReadByte: Read one Byte from the PCA9685
 * return:Data from the PCA9685
 */
uint8_t I2CReadByte(void){
	uint8_t i = 0, temp = 0, data = 0;

	for (; i < 8; ++i){
		digitalWrite(SDA, HIGH);
		delayMicroseconds(DELAY_TIME);
		digitalWrite(SCL, HIGH);
		pinMode(SDA, INPUT);
		delayMicroseconds(1);
		temp = digitalRead(SDA);
		pinMode(SDA, OUTPUT);
		delayMicroseconds(DELAY_TIME);
		digitalWrite(SCL, LOW);
		if (temp == 1){
			data = data << 1;
			data = data | 0x01;
		} else {
			data = data << 1;
		}
	}
	digitalWrite(SCL, LOW);
	return data;
}
/* I2CWriteBytes: Write one Byte to the PCA9685
 * data:The frist byte must be the address of the 'CHIP', then the data to the
 *      chip.
 */
void I2CWriteBytes(uint8_t *data, uint8_t len){
	uint8_t i = 0;

	I2CStart();
	for (; i < len; ++i){
		I2CWriteByte(*data++);
	}
	I2CStop();
	delay(3);
}
void GPIOInit(void){
	pinMode(24 ,INPUT);	//SIG1
	pinMode(25 ,INPUT);	//SIG2
	pinMode(27 ,INPUT);	//SIG3
	pinMode(22 ,INPUT);	//SIG4
	pinMode(7 ,INPUT);	//SIG5
	pinMode(17 ,INPUT);	//SIG6

	pinMode(14, OUTPUT);	//KEY1
	pinMode(15, OUTPUT);	//KEY2
	pinMode(18, OUTPUT);	//KEY3
	pinMode(23, OUTPUT);	//KEY4
	
}
void PCA9685Init(void){
	Data[0] = 0x80;
	Data[1] = 0x00;
	Data[2] = 0x31;
	I2CWriteBytes(Data, 3);
	Data[1] = 0xfe;
	Data[2] = 0x04;
	I2CWriteBytes(Data, 3);
	Data[1] = 0x00;
	Data[2] = 0xa1;
	I2CWriteBytes(Data, 3);
	Data[1] = 0x01;
	Data[2] = 0x04;
	I2CWriteBytes(Data, 3);
}
void AD7794Init(void){
	SPIReset();	
	delay(30);
	SPIWrite2Bytes(WRITE_MODE_REG, 0x0002);
}





uint32_t readADC(uint8_t i){
	uint32_t temp;
	SPIWrite2Bytes(WRITE_STRUCT_REG, ConfigurationReg[i]);
	delay(50);
	temp = SPIRead3Bytes(READ_DATA_REG);
	if (DEBUG_ENABLED){printf("readADC(%d): %06X\n", i, temp);}
	return temp;
}

uint32_t readSignPin(uint8_t i){
	uint32_t temp;
	temp = digitalRead(PinNumSign[i]);
	if (DEBUG_ENABLED){printf("readSignPin(%d): %06X\n", i, temp);}
	return temp;
}

uint32_t readRaspberryPin(uint8_t i){
	uint32_t temp;
	temp = digitalRead(i);
	if (DEBUG_ENABLED){printf("readRaspberryPin(%d): %06X\n", i, temp);}
	return temp;
}

void writeControllButtonPin(uint8_t i, uint8_t on){
	if (DEBUG_ENABLED){printf("writeControllButtonPin(%d): %d\n", i, on);}
	if (on){
		digitalWrite(PinNumControllButtons[i], HIGH);
	} else {
		digitalWrite(PinNumControllButtons[i], LOW);
		/*
		delay(500);
		digitalWrite(PinNumControllButtons[i], HIGH);
		*/
	}
}

void writeRaspberryPin(uint8_t i, uint8_t on){
	if (DEBUG_ENABLED){printf("writeRaspberryPin(%d): %d\n", i, on);}
	if (on){
		digitalWrite(i, HIGH);
	} else {
		digitalWrite(i, LOW);
		/*
		//TODO set back to high, but not here, to long delay while running firmware...
		delay(500);
		digitalWrite(i, HIGH);
		*/
	}
}

void buzzer(uint8_t on, uint32_t pwm){
	if (DEBUG_ENABLED){printf("buzzer(=pin %d): on:%d pwm:%d\n", PI_PIN_BUZZER, on, pwm);}
	if (on){
		softPwmWrite(PI_PIN_BUZZER, 0);
	} else {
		softPwmCreate(PI_PIN_BUZZER, 0, pwm);
		softPwmWrite(PI_PIN_BUZZER, pwm/10);
	}
}


void writeI2CPin(uint8_t i, uint32_t value){
	if (DEBUG_ENABLED){printf("writeI2CPin(%d): %04X\n", i, value);}
	//Data[0] = 0x80; //set in PCA9685Init()
	Data[1] = 0x06+i*4;
	Data[2] = 0x00;
	Data[3] = 0x00;//value&0xff;
	Data[4] = value&0xff;
	Data[5] = (value>>8)&0xff;
	I2CWriteBytes(Data, 6);		
	delay(3);
}


		
	
	
	
	
	
	