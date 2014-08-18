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
uint8_t SPImode=0;
bool debug = false;
bool debug2 = false;
pthread_t SPIWriterReader;
void *SPIWriterReaderFunction(void *ptr);

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

void virtualAtmelStartSPI(){
	pthread_create(&SPIWriterReader, NULL, SPIWriterReaderFunction, NULL);
}
void virtualAtmelStopSPI(){
	pthread_join(SPIWriterReader, NULL);
}	
void *SPIWriterReaderFunction(void *ptr){
	char* text;
	uint8_t dataReceipt;
	uint8_t i;
	uint16_t *picture;
	uint8_t len;
	while(daemonGetStateRunning()){
		switch (SPImode) {
			case SPI_MODE_IDLE:
			SPImode=SPI_MODE_GET_STATUS;
			break;
			case SPI_MODE_GET_STATUS:
				SPImode=SPI_MODE_MOTOR;
				SPIAtmelWrite(SPI_MODE_GET_STATUS);
				dataReceipt=getValidResultOrReset();
				daemonSetStateLidClosed((dataReceipt & (1<<SB_LidClosed))>0);
				daemonSetStateLidLocked((dataReceipt & (1<<SB_LidLocked))>0);
				daemonSetStatePusherLocked((dataReceipt & (1<<SB_PusherLocked))>0);
				heaterSetStatusIsOn((dataReceipt & (1<<SB_isIHOn))>0);
				if ((dataReceipt & (1<<SB_MotorStoped))>0){
					motorSetI2cValuesMotorRpm(0);
				}
				//fanSetIsOn((dataReceipt & (1<<SB_IHFanOn))>0);		
			break;
			case SPI_MODE_DISPLAY_CLEAR:
				SPImode=SPI_MODE_MOTOR;
				SPIAtmelWrite(SPI_MODE_DISPLAY_CLEAR);		
			break;
			case SPI_MODE_MOTOR:
				SPImode=SPI_MODE_HEATING;
				SPIAtmelWrite(SPI_MODE_MOTOR);
				SPIAtmelWrite(motorGetI2cValuesMotorRpm());		
			break;
			case SPI_MODE_HEATING:
				SPImode=SPI_MODE_VENTIL;
				SPIAtmelWrite(SPI_MODE_HEATING);
				if(heaterGetPowerStatus()){
					SPIAtmelWrite(0x01);
				} else{
					SPIAtmelWrite(0x00);
				}				
			break;
			case SPI_MODE_VENTIL:
				SPImode=SPI_MODE_GET_MOTOR_SPEED;
				SPIAtmelWrite(SPI_MODE_VENTIL);
				if(solenoidGetOpen()){
					SPIAtmelWrite(0x01);
				} else{
					SPIAtmelWrite(0x00);
				}	
			break;
			case SPI_MODE_DISPLAY_PERCENT:
				SPImode=SPI_MODE_GET_MOTOR_SPEED;
				SPIAtmelWrite(SPI_MODE_DISPLAY_PERCENT);
				SPIAtmelWrite(displayGetPercentToShow());
			break;
			case SPI_MODE_DISPLAY_PERCENT_TEXT:
				SPImode=SPI_MODE_GET_MOTOR_SPEED;
				SPIAtmelWrite(SPI_MODE_DISPLAY_PERCENT_TEXT);
				SPIAtmelWrite(displayGetPercentToShow());
				text=displayGetTextToShow();
				len = strlen(text);
				SPIAtmelWrite(len);
				for(i=0;i<len;i++){
					SPIAtmelWrite(text[i]);
				}
				SPIAtmelWrite(0x00);
				getValidResultOrResetAdditionalValid(SPI_Error_Text_Invalid);
			break;
			case SPI_MODE_DISPLAY_TEXT:
				SPImode=SPI_MODE_GET_MOTOR_SPEED;
				SPIAtmelWrite(SPI_MODE_DISPLAY_TEXT);
				text=displayGetTextToShow();
				len = strlen(text);
				SPIAtmelWrite(len);
				for(i=0;i<len;i++){
					SPIAtmelWrite(text[i]);
				}
				SPIAtmelWrite(0x00);
				getValidResultOrResetAdditionalValid(SPI_Error_Text_Invalid);				
			case SPI_MODE_DISPLAY_TEXT_SMALL:
			break;
			case SPI_MODE_DISPLAY_PICTURE:
				SPImode=SPI_MODE_GET_MOTOR_SPEED;
				SPIAtmelWrite(SPI_MODE_DISPLAY_PICTURE);
				picture=displayGetPictureToShow();
				for(i=0; i<9;++i){
					SPIAtmelWrite((picture[i] >> 8) & 0xFF);
					SPIAtmelWrite(picture[i] & 0xFF);
				}
				SPIAtmelWrite(0x00);
				getValidResultOrResetAdditionalValid(SPI_Error_Picture_Invalid);			
			break;	
			case SPI_MODE_MAINTENANCE:	
			break;		
			case SPI_MODE_GET_MOTOR_SPEED:
				SPImode=SPI_MODE_GET_IGBT_TEMP;
				SPIAtmelWrite(SPI_MODE_GET_MOTOR_SPEED);
				motorSetRPMTrue(getResult());
			break;
			case SPI_MODE_GET_IGBT_TEMP:
				SPImode=SPI_MODE_GET_HEATING_OUTPUT_LEVEL;
				SPIAtmelWrite(SPI_MODE_GET_IGBT_TEMP);
				heaterSetTempTrans(getResult());
			break;
			case SPI_MODE_GET_HEATING_OUTPUT_LEVEL:
				SPImode=SPI_MODE_GET_MOTOR_POS_SENSOR;
				SPIAtmelWrite(SPI_MODE_GET_HEATING_OUTPUT_LEVEL);
				heaterSetPWMTrue(getResult());
			break;
			case SPI_MODE_GET_MOTOR_POS_SENSOR:
				SPImode=SPI_MODE_GET_MOTOR_RPM;
				SPIAtmelWrite(SPI_MODE_GET_MOTOR_POS_SENSOR);
				motorSetSensor(getResult());
			break;
			case SPI_MODE_GET_MOTOR_RPM:		
				SPImode=SPI_MODE_GET_FAN_PWM;
				SPIAtmelWrite(SPI_MODE_GET_MOTOR_RPM);
				motorSetRPMTrue(getResult());
			break;
			case SPI_MODE_GET_FAN_PWM:
				SPImode=SPI_MODE_GET_DEBUG;
				SPIAtmelWrite(SPI_MODE_GET_FAN_PWM);
				heaterSetFanPWM(getResult());
			break;
			case SPI_MODE_GET_DEBUG:
				SPImode=SPI_MODE_IDLE;
				SPIAtmelWrite(SPI_MODE_GET_DEBUG);
				daemonSetVdebug(getResult());
			break;
			default:
				SPImode=SPI_MODE_IDLE;
			break;
		}
		delay(10);
	}
	return 0;
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

uint8_t atmelGetIGBTTemp(){
	SPIAtmelWrite(SPI_MODE_GET_IGBT_TEMP);
	return getResult();
}

void virtualSpiAtmelSetNewMode(uint8_t newMode){
	SPImode=newMode;
}

bool atmelGetDebug(){
	return debug;
}

bool atmelGetDebug2(){
	return debug2;
}
