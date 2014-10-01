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

#include <wiringPi.h>
#include "virtualspi.h"
#include "bool.h"

uint8_t Data[10];
uint8_t CS_current;
uint32_t selectedShieldVersion;

/*******************PI Driver Code**********************/
/** @brief VirtualSPIInit
 *  @param shieldVersion:the configured shield Version
 */
void VirtualSPIInit(uint32_t shieldVersion){
	wiringPiSetupGpio();
	selectedShieldVersion = shieldVersion;
	
	pinMode(MOSI, OUTPUT);
	pinMode(MISO, INPUT);
	pinMode(SCLK, OUTPUT);
	pinMode(CS, OUTPUT);
	pinMode(CS2, OUTPUT);
	
	digitalWrite(CS, HIGH);
	digitalWrite(CS2, HIGH);
	CS_current = CS;
	delay(30);
}

/** @brief SPISelectChip: Change the selected chip.
 *  @param index:the index of the chip
 *  @return if it's 24bit ADC
 */
bool SPISelectChip(uint8_t index){
	if (index>0){
		CS_current=CS2;
//		digitalWrite(CS, HIGH);
//		digitalWrite(CS2, LOW);
		return true;
	} else {
		CS_current=CS;
//		digitalWrite(CS2, HIGH);
//		digitalWrite(CS, LOW);
		return false;
	}
}

/** @brief SPIReset: Reset the AD7796 chip, write 4 0xff.
 */
void SPIReset(void){
	digitalWrite(CS_current, LOW);
		delayMicroseconds(100);
	SPIWrite(0xff);
	SPIWrite(0xff);
	SPIWrite(0xff);
	SPIWrite(0xff);
		delayMicroseconds(100);
	digitalWrite(CS_current, HIGH);
	
}

/** @brief SPIWrite: Write one byte data to the register in AD7796
 *  @param data:datas write to register
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

/** @brief Read one byte data from the AD7796
 *  @return  data from register
 */
uint8_t SPIRead(void){
	int i = 7;
	uint8_t data = 0;
	
	for (i = 7; i >= 0; i--){
		digitalWrite(SCLK, LOW);
		delayMicroseconds(100);
		digitalWrite(SCLK, HIGH);
		delayMicroseconds(100);
		if (digitalRead(MISO)){
			data = data | (1<<i);
		}
	}

	return data;
}

/* SPIWriteByte: Wirte one byte to denstination register. 
 * 
 */
void SPIWriteByte(uint8_t reg, uint8_t data){
	digitalWrite(CS_current, LOW);
		delayMicroseconds(100);
	SPIWrite(reg);
	SPIWrite(data);
		delayMicroseconds(100);
	digitalWrite(CS_current, HIGH);
}

/** @brief SPIWrite2Bytes: Wirte one byte to denstination register. 
 *  @param reg : register
 *  @param data
 */
void SPIWrite2Bytes(uint8_t reg, uint32_t data){
	digitalWrite(CS_current, LOW);
		delayMicroseconds(100);
	SPIWrite(reg);
	SPIWrite((data>>8)&0xff);
	SPIWrite(data&0xff);
		delayMicroseconds(100);
	digitalWrite(CS_current, HIGH);
}

/** @brief Get one byte from denstination register.
 *  @param reg : register
 *  @return 1 byte data
 */
uint8_t SPIReadByte(uint8_t reg){
	uint8_t data;

	digitalWrite(CS_current, LOW);
		delayMicroseconds(100);
	SPIWrite(reg);
	data = SPIRead();
		delayMicroseconds(100);
	digitalWrite(CS_current, HIGH);

	return data;
}

/** @brief Get 2 bytes from denstination register.
 *  @param reg : register
 * @return  2 bytes data
 */
uint32_t SPIRead2Bytes(uint8_t reg){
	uint32_t data;

	digitalWrite(CS_current, LOW);
		delayMicroseconds(100);
	SPIWrite(reg);
	data = SPIRead();
	data = (data << 8)| SPIRead();
		delayMicroseconds(100);
	digitalWrite(CS_current, HIGH);
	
	return data;
}

/** @brief Get 3 bytes from denstination register.
 *  @param reg : register
 * @return  2 bytes data
 */
uint32_t SPIRead3Bytes(uint8_t reg){
	uint32_t data;

	digitalWrite(CS_current, LOW);
		delayMicroseconds(100);
	SPIWrite(reg);
	data = SPIRead();
	data = (data << 8)| SPIRead();
	data = (data << 8)| SPIRead();
		delayMicroseconds(100);
	digitalWrite(CS_current, HIGH);

	return data;
}
