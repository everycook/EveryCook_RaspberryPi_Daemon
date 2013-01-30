#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include <math.h>

#include <softPwm.h>
#include <wiringPi.h>

#include "virtualspi.h"
#include "virtuali2c.h"
#include "basic_functions.h"

uint32_t ConfigurationReg[] = {0x710, 0x711, 0x712, 0x713, 0x714, 0x115};//For AD7794 conf-reg

uint8_t PinNumSign[] = {24, 25, 27, 22, 7, 17};

uint8_t PinNumControllButtons[] = {14, 15, 18, 23};


uint8_t Data[10];


void initHardware(){
	VirtualSPIInit();
	VirtualI2CInit();
	GPIOInit();
	PCA9685Init();
	AD7794Init();
}

//GPIO PCA9685 initialization
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




//read/write functions
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

	
	
	
	
	