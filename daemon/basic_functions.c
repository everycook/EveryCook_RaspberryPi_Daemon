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
//#include <math.h>

#include <softPwm.h>
#include <wiringPi.h>

#include "virtualspi.h"
#include "virtuali2c.h"
#include "ad7794_interface.h"
#include "basic_functions.h"

uint32_t ConfigurationReg[] = {0x390, 0x391, 0x392, 0x393, 0x914, 0x295};//For AD7794 conf-reg
uint16_t ModeReg = 0x008;

uint8_t* PinHeaterLeds;
uint8_t PinHeaterLeds_v1[] = {24, 25, 27, 22, 7, 17};
uint8_t PinHeaterLeds_v2[] = {4,14,15,7};
uint8_t PinHeaterLeds_v4[] = {};

uint8_t* PinHeaterControllButtons;
uint8_t PinHeaterControllButtons_v1[] = {14, 15, 18, 23};
uint8_t PinHeaterControllButtons_v2[] = {18, 23, 24, 25};
uint8_t PinHeaterControllButtons_v4[] = {};

uint8_t* PinButtons;
uint8_t PinButtons_v1[] = {};
uint8_t Data[10];
uint8_t* InverseButtons;

struct adc_private adc;

bool debug_enabled2 = false;

bool use_spi_dev = false;

void setDebugEnabled(bool value){
	debug_enabled2 = value;
}

void initHardware(uint32_t shieldVersion, uint8_t* buttonPins, uint8_t* buttonInverse){
	if (debug_enabled2){printf("initHardware\n");}
	
	if(shieldVersion == 1){
		PinHeaterLeds = &PinHeaterLeds_v1[0];
		PinHeaterControllButtons = &PinHeaterControllButtons_v1[0];
		PinButtons = &PinButtons_v1[0];
		InverseButtons = &PinButtons_v1[0];
	} else if(shieldVersion == 2 || shieldVersion == 3){
		PinHeaterLeds = &PinHeaterLeds_v2[0];
		PinHeaterControllButtons = &PinHeaterControllButtons_v2[0];
		PinButtons = &buttonPins[0];
		InverseButtons = &buttonInverse[0];
	} else if(shieldVersion == 4){
		PinHeaterLeds = &PinHeaterLeds_v4[0];
		PinHeaterControllButtons = &PinHeaterControllButtons_v4[0];
		PinButtons = &buttonPins[0];
		InverseButtons = &buttonInverse[0];
	}
	if (!use_spi_dev){
		VirtualSPIInit();
	}
	VirtualI2CInit();
	GPIOInit();
	
	PCA9685Init(shieldVersion);
	AD7794Init();
}

void setADCConfigReg(uint32_t newConfig[]){
	if (debug_enabled2){printf("setADCConfigReg...\n");}
	uint8_t i=0;
	for ( ;i<6; ++i){
		ConfigurationReg[i] = newConfig[i];
		if (debug_enabled2){printf("\t%d: %04X\n", i, newConfig[i]);}
	}
	if (debug_enabled2){printf("done.\n");}
}

void setADCModeReg(uint16_t newModeReg){
	ModeReg = newModeReg;
}

//GPIO PCA9685 initialization
void GPIOInit(void){
	if (debug_enabled2){printf("GPIOInit\n");}
	//heater led pins
	if (debug_enabled2){printf("init heater leds\n");}
	uint8_t heaterLedCount = sizeof(PinHeaterLeds);
	uint8_t i=0;
	for(;i<heaterLedCount;++i){
		pinMode(PinHeaterLeds[i], INPUT);
	}
	//heater button pins
	if (debug_enabled2){printf("init heater buttons\n");}
	uint8_t heaterButtonCount = sizeof(PinHeaterControllButtons);
	i=0;
	for(;i<heaterButtonCount;++i){
		pinMode(PinHeaterControllButtons[i], OUTPUT);
		writeRaspberryPin(PinHeaterControllButtons[i],1);
	}
	//button pins
	if (debug_enabled2){printf("init buttons\n");}
	uint8_t buttonCount = sizeof(PinButtons);
	i=0;
	for(;i<buttonCount;++i){
		pinMode(PinButtons[i], INPUT);
	}
	if (debug_enabled2){printf("done\n");}
}
void PCA9685Init(uint32_t shieldVersion){
	if (debug_enabled2){printf("PCA9685Init\n");}
	Data[0] = 0x80;
	Data[1] = 0x00;
	Data[2] = 0x31; //0x11
	I2CWriteBytes(Data, 3);
	
	//PWM frequency
	//prescale value = round (osc_clock / 4096 × update_rate) – 1
	//update_rate = osc_clock / (prescale value +1) * 4096
	Data[1] = 0xfe;
	if (shieldVersion < 3){
		Data[2] = 0x79; 	//50Hz
	} else {
		Data[2] = 0x05; 	//1000Hz (1017Hz)
		//Data[2] = 0x0B; 	//500Hz (508Hz)
		//Data[2] = 0x79; 	//50Hz
	}
	I2CWriteBytes(Data, 3);
	
	Data[1] = 0x00;
	Data[2] = 0xa1; // 0x01
	I2CWriteBytes(Data, 3);
	
	Data[1] = 0x01;
	Data[2] = 0x04;
	I2CWriteBytes(Data, 3);
}
void AD7794Init(void){
	if (debug_enabled2){printf("AD7794Init\n");}
	if (use_spi_dev){
		int r;
		if ((r=ad7794_init(&adc, 0, 0))){
			printf("Could not init AD7794... errno=%d\n",r);
			exit(r);
		}
	
		uint8_t value[2];
		value[0] = ModeReg & 0x000000FF;
		value[1] = (ModeReg & 0x0000FF00) >> 8;
		ad7794_communicate(&adc, AD7794_MODE, AD7794_DIRECTION_WRITE, 2, &value[0]);
	} else {
		SPIReset();	
		delay(30);
		SPIWrite2Bytes(WRITE_MODE_REG, ModeReg);
	}
}




//read/write functions
uint32_t readADC(uint8_t i){
	uint32_t data;
	if (use_spi_dev){
		ad7794_select_channel2(&adc, i, ConfigurationReg[i]);
	} else {
		SPIWrite2Bytes(WRITE_CONFIG_REG, ConfigurationReg[i]);
	}
	
	bool readyToRead = false;
	if (use_spi_dev){
		while (!readyToRead){
			if (ad7794_check_if_ready(&adc)){
				readyToRead = true;
			} else {
				delay(5);
			}
		}
		data = ad7794_read_data(&adc);
	} else {
		while (!readyToRead){
			uint8_t adcState = SPIReadByte(READ_STATUS_REG);
			if ((adcState & 0x80) == 0){
				readyToRead = true;
			} else {
				delay(5);
			}
		}
		data = SPIRead3Bytes(READ_DATA_REG);
	}
	if (debug_enabled2){printf("readADC(%d): %06X\n", i, data);}
	return data;
}

uint32_t readSignPin(uint8_t i){
	uint32_t data;
	data = digitalRead(PinHeaterLeds[i]);
	if (debug_enabled2){printf("readSignPin(%d): %06X\n", i, data);}
	return data;
}

uint32_t readRaspberryPin(uint8_t i){
	uint32_t data;
	data = digitalRead(i);
	if (debug_enabled2){printf("readRaspberryPin(%d): %06X\n", i, data);}
	return data;
}

void writeControllButtonPin(uint8_t i, uint8_t on){
	if (debug_enabled2){printf("writeControllButtonPin(%d): %d\n", i, on);}
	if (on){
		digitalWrite(PinHeaterControllButtons[i], HIGH);
	} else {
		digitalWrite(PinHeaterControllButtons[i], LOW);
	}
}

uint32_t readButton(uint8_t i){
	uint32_t data;
	data = digitalRead(PinButtons[i]);
	if (InverseButtons[i]){
		data = !data;
	}
	if (debug_enabled2){printf("readButton(%d): %06X\n", i, data);}
	return data;
}

void writeRaspberryPin(uint8_t i, uint8_t on){
	if (debug_enabled2){printf("writeRaspberryPin(%d): %d\n", i, on);}
	if (on){
		digitalWrite(i, HIGH);
	} else {
		digitalWrite(i, LOW);
	}
}

void buzzer(uint8_t on, uint32_t pwm){
	if (debug_enabled2){printf("buzzer(=pin %d): on:%d pwm:%d\n", PI_PIN_BUZZER, on, pwm);}
	if (on){
	//	softPwmWrite(PI_PIN_BUZZER, 0);
	} else {
	//	softPwmCreate(PI_PIN_BUZZER, 0, pwm);
	//	softPwmWrite(PI_PIN_BUZZER, pwm/10);
	}
}


void writeI2CPin(uint8_t i, uint32_t value){
	if (debug_enabled2){printf("writeI2CPin(%d): %04X\n", i, value);}
	//Data[0] = 0x80; //set in PCA9685Init()
	Data[1] = 0x06+i*4;
	Data[2] = 0x00;
	Data[3] = 0x00;//value&0xff;
	Data[4] = value&0xff;
	Data[5] = (value>>8)&0xff;
	I2CWriteBytes(Data, 6);		
	delay(3);
}

	
	
	
	
	