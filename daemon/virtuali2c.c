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
#include "virtuali2c.h"

/** VirtualI2CInit: Initializate the virtual I2C-BUS protocal
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

/** @brief I2CWriteByte: Write one Byte to the PCA9685
 *  @param data:Data to the PCA9685
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

/** @brief I2CWriteBytes: Write one Byte to the PCA9685
 *  @param data:The frist byte must be the address of the 'CHIP', then the data to the chip.
 *  @param len : data size
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
