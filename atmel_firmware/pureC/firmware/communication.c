#include <avr/interrupt.h>
#include <util/delay.h>

#include "input.h"
#include "mytypes.h"
#include "communication.h"
#include "status.h"

uint8_t nextResponse = SPI_NoResponse;

uint8_t reciveBuffer[512];
uint8_t recivePos=0;
uint8_t reciveLen=0;

boolean lastClock = false;
uint8_t shiftcount = 0;
uint8_t inValue = 0;
uint8_t outValue = 0;

struct pinInfo PIN_MOSI = PB_5; //Master: out  //Slave:in
struct pinInfo PIN_MISO = PB_6; //PB_6; //Master: in  //Slave:Out
struct pinInfo PIN_SCK = PB_7; //Master: out  //Slave:in

void SPI_init(boolean wdtRestart){
/* //SPI don't work: data cannot be read completly and answare is not defined...
	SPCR = _BV(SPIE) | _BV(SPE); //enable SPI & interrupt
	pinMode(PIN_MISO, OUTPUT);
	
	if (wdtRestart){
		SPDR = SPI_Error_WDT_Reset;
	} else {
		SPDR = SPI_NoResponse;
	}
*/
	//Bit baging
	SPCR = 0; //Disable SPI -> do it the bit banging way
	PRR |= _BV(PRSPI); //Disable SPI, Power reduction
	
	pinMode(PIN_MOSI, INPUT);
	pinMode(PIN_MISO, OUTPUT);
	pinMode(PIN_SCK, INPUT);
	
	digitalWrite(PIN_MISO, LOW);
	
	PCMSK1 |= _BV(PCINT15); //Enable PinChange interrupt on SPI-Clock
	PCICR |= _BV(PCIE1); //Enable Interrupt for Pins 8-15
	
	if (wdtRestart){
		outValue = SPI_Error_WDT_Reset;
	} else {
		outValue = SPI_NoResponse;
	}
}

uint16_t availableSPI(){
	return reciveLen - recivePos;
}

uint8_t peekSPI(){
	if (availableSPI()>0){
		return reciveBuffer[recivePos];
	} else {
		return 0;
	}
}

uint8_t readSPI(boolean blocking){
	if (availableSPI()>0){
		uint8_t data = reciveBuffer[recivePos];
		recivePos++;
		uint8_t oldSREG = SREG;
		cli();
		if (recivePos == reciveLen && recivePos>100){
			recivePos=0;
			reciveLen=0;
		}
		SREG = oldSREG;
		return data;
	} else {
		if (blocking){
			while(reciveLen == recivePos){
				_delay_ms(1);
				//_delay_us(500);
			}			
			uint8_t data = reciveBuffer[recivePos];
			recivePos++;
			return data;
		} else {
			return 0;
		}
	}
}

//############################ PinChange Interrupt ############################
ISR(PCINT1_vect){
	boolean clock = digitalRead(PIN_SCK);
	if (lastClock != clock && clock){ //rising edge
		inValue <<= 1;
		boolean inBit = digitalRead(PIN_MOSI);
		if (inBit){
			inValue |= 1;
		}
		
		//digitalWrite(PIN_MISO, inBit);
		if ((outValue & 0x80) == 0x80){
			digitalWrite(PIN_MISO, HIGH);
		} else {
			digitalWrite(PIN_MISO, LOW);
		}
		outValue <<= 1;
		++shiftcount;
		if (shiftcount == 8){
			shiftcount = 0;
			reciveBuffer[reciveLen] = inValue;
			++reciveLen;
			outValue = nextResponse;
			nextResponse = SPI_NoResponse;
		}
	}
	lastClock = clock;
}

//don't work.... 
//############################ SPI Interrupt ############################
ISR(SPI_STC_vect){
	uint8_t data = SPDR;
	SPDR = nextResponse;
	//SPDR = data;
	
	nextResponse = SPI_NoResponse;
	reciveBuffer[reciveLen] = data;
	reciveLen++;
}