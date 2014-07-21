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
#include <stdint.h>
#include <string.h>

#include <wiringPi.h>
#include "virtualspiAtmel.h"
#include "bool.h"

bool debug = false;
bool debug2 = false;

/** @brief initialization PIN for SPI and debug=debugEnabled
 *  @param debugEnabled 
 */
void VirtualSPIAtmelInit(bool debugEnabled){
	debug = debugEnabled;
	wiringPiSetupGpio();
	
	pinMode(MOSI_ATMEL, OUTPUT);
	pinMode(MISO_ATMEL, INPUT);
	pinMode(SCLK_ATMEL, OUTPUT);
	//pinMode(CS_ATMEL, OUTPUT);
	pinMode(ATMEL_RESET, OUTPUT);
	
	digitalWrite(MOSI_ATMEL, LOW);
	digitalWrite(SCLK_ATMEL, LOW);
	
	//digitalWrite(CS_ATMEL, HIGH);
	delay(100);
}

void SPIAtmelReset(void){
	//Do reset
	digitalWrite(ATMEL_RESET, HIGH);
	delay(100);
	digitalWrite(ATMEL_RESET, LOW);
	digitalWrite(SCLK_ATMEL, LOW);
	if (debug || debug2) {printf("SPI-Atmel Reset\n");}
	//wait it's ready
	delay(250);
}

/** @brief Write one byte data to teh atmel
 *  @param data:datas write
 */
uint8_t SPIAtmelWrite(uint8_t data){
	int i = 7;
	uint8_t readed = 0;
	digitalWrite(MOSI_ATMEL, LOW);
	//digitalWrite(CS_ATMEL, LOW);
	//delayMicroseconds(SPI_ATMEL_CS_DELAY);
	for (i = 7; i >= 0; i--){
		if (data & (1<<i)){
			digitalWrite(MOSI_ATMEL, HIGH);
		} else {
			digitalWrite(MOSI_ATMEL, LOW);
		}
		digitalWrite(SCLK_ATMEL, HIGH);
		delayMicroseconds(SPI_ATMEL_CLOCK_DELAY);
		digitalWrite(SCLK_ATMEL, LOW);
		delayMicroseconds(SPI_ATMEL_CLOCK_DELAY);
		if (digitalRead(MISO_ATMEL)){
			readed = readed | (1<<i);
		}
	}
	//delayMicroseconds(SPI_ATMEL_CS_DELAY);
	//digitalWrite(CS_ATMEL, HIGH);
	//printf("readed %X\n", readed);
	//if (readed != SPI_NoResponse){
		if (debug) {printf("readed %02X on write(%02X)\n", readed, data);}
	//}
	delay(10);
	return readed;
}

/* SPIAtmelRead: Read one byte data from the Atmel
 * return: data from register
 */
uint8_t SPIAtmelRead(void){
	int i = 7;
	uint8_t data = 0;
	
	//digitalWrite(CS_ATMEL, LOW);
	//delayMicroseconds(SPI_ATMEL_CS_DELAY);
	digitalWrite(MOSI_ATMEL, LOW);
	for (i = 7; i >= 0; i--){
		digitalWrite(SCLK_ATMEL, HIGH);
		delayMicroseconds(SPI_ATMEL_CLOCK_DELAY);
		digitalWrite(SCLK_ATMEL, LOW);
		delayMicroseconds(SPI_ATMEL_CLOCK_DELAY);
		if (digitalRead(MISO_ATMEL)){
			data = data | (1<<i);
		}
	}
	//delayMicroseconds(SPI_ATMEL_CS_DELAY);
	//digitalWrite(CS_ATMEL, HIGH);
	if (debug) {printf("SPIAtmelRead: %02X\n", data);}
	delay(100);
	return data;
}

uint8_t getResult(){
	uint8_t tryCount = 0;
	uint8_t data;
	do {
		data = SPIAtmelRead();
		tryCount++;
	} while (data == SPI_NoResponse && tryCount<10);
	if (debug) {printf("readed %X, tryCount %d\n", data, tryCount);}
	
	return data;
}

uint8_t getValidResultOrReset(){
	uint8_t data = getResult();
	
	if ((data & 0x81) != 0x01){
		if (((data & 0x81) != 0x81) && (data != SPI_NoResponse)){
			SPIAtmelReset();
		}
	}
	
	return data;
}

uint8_t getValidResultOrResetAdditionalValid(uint8_t validByte){
	uint8_t data = getResult();
	
	if ((data & 0x81) != 0x01){
		if (((data & 0x81) != 0x81) && (data != SPI_NoResponse) && (data != validByte)){
			SPIAtmelReset();
		}
	}
	
	return data;
}

uint8_t atmelGetStatus(){
	SPIAtmelWrite(SPI_MODE_GET_STATUS);
	return getValidResultOrReset();
}


void atmelSetHeating(bool on){
	if (debug2) {printf("--->atmelSetHeating\n");}
	SPIAtmelWrite(SPI_MODE_HEATING);
	if(on){
		SPIAtmelWrite(0x01);
	} else{
		SPIAtmelWrite(0x00);
	}
	getValidResultOrReset();
}

void atmelSetMotorRPM(uint8_t rpm){
	if (debug2) {printf("--->atmelSetMotorRPM\n");}
	SPIAtmelWrite(SPI_MODE_MOTOR);
	SPIAtmelWrite(rpm);
	getValidResultOrReset();
}

void atmelSetSolenoidOpen(bool open){
	if (debug2) {printf("--->atmelSetSolenoidOpen\n");}
	SPIAtmelWrite(SPI_MODE_VENTIL);
	if(open){
		SPIAtmelWrite(0x01);
	} else{
		SPIAtmelWrite(0x00);
	}
	getValidResultOrReset();
}

void atmelSetMaintenance(bool on){
	if (debug2) {printf("--->atmelSetMaintenance\n");}
	SPIAtmelWrite(SPI_MODE_MAINTENANCE);
	if(on){
		SPIAtmelWrite(0x99);
	} else{
		SPIAtmelWrite(0x22);
	}
	//be sure its change to maintenance, it has the be send 2 times
	SPIAtmelWrite(SPI_MODE_MAINTENANCE);
	if(on){
		SPIAtmelWrite(0x99);
	} else{
		SPIAtmelWrite(0x22);
	}
	//00 as command end mark
	SPIAtmelWrite(0x00);
	getValidResultOrReset();
}

uint8_t atmelGetMotorSpeed(){
	SPIAtmelWrite(SPI_MODE_GET_MOTOR_SPEED);
	return getResult();
}

uint8_t atmelGetIGBTTemp(){
	SPIAtmelWrite(SPI_MODE_GET_IGBT_TEMP);
	return getResult();
}

uint8_t atmelGetHeatingOutputLevel(){
	SPIAtmelWrite(SPI_MODE_GET_HEATING_OUTPUT_LEVEL);
	return getResult();
}

bool atmelGetMotorPosSensor(){
	SPIAtmelWrite(SPI_MODE_GET_MOTOR_POS_SENSOR);
	return getResult();
}

uint8_t atmelGetMotorRPM(){
	SPIAtmelWrite(SPI_MODE_GET_MOTOR_RPM);
	return getResult();
}

bool atmelGetDebug(){
	return debug;
}

bool atmelGetDebug2(){
	return debug2;
}
