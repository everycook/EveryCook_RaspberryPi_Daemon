#include <stdio.h>
#include <stdint.h>

#include <wiringPi.h>
#include "virtualspi.h"

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