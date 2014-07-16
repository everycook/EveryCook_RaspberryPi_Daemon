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

void SPI_SLAVE_ISR (void);

uint8_t currentInByte = 0;
uint8_t currentOutByte = 0;
uint8_t bitRead = 0;

uint8_t spiOutData[100];
uint8_t spiOutPos = 0;
uint8_t spiOutLen = 0;

uint8_t spiInData[100];
uint8_t spiInPos = 0;
uint8_t spiInLen = 0;

/** @brief Pin initialisation
*/
void VirtualSPISlaveInit(void){
	wiringPiSetupGpio();
	
	pinMode(SLAVE_MOSI, INPUT);
	pinMode(SLAVE_MISO, OUTPUT);
	pinMode(SLAVE_SCLK, INPUT);
	//pinMode(SLAVE_CS, OUTPUT);
	
	wiringPiISR(SLAVE_SCLK, INT_EDGE_FALLING, &SPI_SLAVE_ISR) ;
}

uint8_t SPISlaveRead(){
}

void SPISlaveWrite(uint8_t data){
}

void SPI_SLAVE_ISR (void) {
	uint8_t bit = digitalRead(SLAVE_MOSI);
	currentInByte = currentInByte << 1 | bit;
	if (currentOutByte & 0x80){
		digitalWrite(SLAVE_MISO, HIGH);
	} else {
		digitalWrite(SLAVE_MISO, LOW);
	}
	currentOutByte <<= 1;
	bitRead++;
	if (bitRead == 0){
		spiInData
	}
}

/** @brief SPIReset: Reset the AD7794 chip, write 4 0xff.
 */
void SPIReset(void){
//	digitalWrite(SLAVE_CS, LOW);
	SPIWrite(0xff);
	SPIWrite(0xff);
	SPIWrite(0xff);
	SPIWrite(0xff);
//	digitalWrite(SLAVE_CS, HIGH);	
}

/** @brief ** @bSPIWrite: Write one byte data to the register in AD7794
 * @param data :datas write to register
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
//	digitalWrite(SLAVE_CS, LOW);
	SPIWrite(reg);
	SPIWrite(data);
//	digitalWrite(SLAVE_CS, HIGH);
}

/* SPIWrite2Bytes: Wirte one byte to denstination register. 
 * 
 */
void SPIWrite2Bytes(uint8_t reg, uint32_t data){
//	digitalWrite(SLAVE_CS, LOW);
	SPIWrite(reg);
	SPIWrite((data>>8)&0xff);
	SPIWrite(data&0xff);
//	digitalWrite(SLAVE_CS, HIGH);
}

/* SPIReadByte: Get one byte from denstination register. 
 * return: 1 byte data
 */
uint8_t SPIReadByte(uint8_t reg){
	uint8_t data;

//	digitalWrite(SLAVE_CS, LOW);
	SPIWrite(reg);
	data = SPIRead();
//	digitalWrite(SLAVE_CS, HIGH);

	return data;
}

/* SPIReadByte: Get two bytes from denstination register. 
 * return: 2 bytes data
 */
uint32_t SPIRead2Bytes(uint8_t reg){
	uint32_t data;

//	digitalWrite(SLAVE_CS, LOW);
	SPIWrite(reg);
	data = SPIRead();
	data = (data << 8)| SPIRead();
//	digitalWrite(SLAVE_CS, HIGH);
	
	return data;
}

/* SPIReadByte: Get three bytes from denstination register. 
 * return: 3 bytes data
 */
uint32_t SPIRead3Bytes(uint8_t reg){
	uint32_t data;

//	digitalWrite(SLAVE_CS, LOW);
	SPIWrite(reg);
	data = SPIRead();
	data = (data << 8)| SPIRead();
	data = (data << 8)| SPIRead();
//	digitalWrite(SLAVE_CS, HIGH);

	return data;
}
