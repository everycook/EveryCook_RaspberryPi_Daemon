#include "spihandling.h"
#include "mytypes.h"

#include <avr/interrupt.h>

uint8_t spiOutData[100];
uint8_t spiOutPos = 0;
uint8_t spiOutLen = 0;

uint8_t spiInData[100];
uint8_t spiInPos = 0;
uint8_t spiInLen = 0;

void initSPI(){
	//SPI_MasterInit
	/* Set MOSI and SCK output, MISO input */
	//				MOSI		SCK			MISO
	DDRB = (DDRB | _BV(DD5) | _BV(DD7)) & ~_BV(DD6);
	/* Enable SPI interrupt, Enable SPI, set Master mode, set clock rate fck/16 */
	SPCR = /*_BV(SPIE) |*/ _BV(SPE) | _BV(MSTR) | _BV(SPR0);
}

void transmitSPI(uint8_t data){
	if (spiOutLen < 100){
		spiOutData[spiOutLen] = data;
		spiOutLen++;
	} else {
		//TODO move data and pos
	}
}

uint8_t availableSPI(){
	return spiInLen - spiInPos;
}

uint8_t readSPI(boolean blocking){
	if (availableSPI()>0){
		uint8_t data = spiInData[spiInPos];
		spiInPos++;
		if (spiInPos == spiInLen){
			spiInPos=0;
			spiInLen=0;
		}
		return data;
	} else {
		if (blocking){
			//TODO what do I here?
		} else {
			return 0;
		}
	}
}

	
	
	
	
void SPI_MasterTransmit(char cData)
{
/* Start transmission */
SPDR = cData;
/* Wait for transmission complete */
while(!(SPSR & (1<<SPIF)))
;
}

//SPI Serial Transfer Complete
ISR(SPI_STC_vect) {
	uint8_t data = SPDR;
	
	if (spiInLen < 100){
		spiInData[spiInLen] = data;
		spiInLen++;
	} else {
		//TODO move data and pos
	}
	//transmit next byte
	if (spiOutLen > spiOutPos){
		data = spiOutData[spiOutPos];
		spiOutPos++;
		if (spiOutLen == spiOutPos){
			spiOutLen = 0;
			spiOutPos = 0;
		}
		SPDR = data;
	}
}