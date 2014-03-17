#ifndef communication_h
#define communication_h

#include "mytypes.h"

extern uint8_t nextResponse;


extern uint8_t recivedCounter;
extern uint8_t recivedData;
extern boolean raspiRecived;

void SPI_init(boolean wdtRestart);
uint16_t availableSPI();
uint8_t peekSPI();
uint8_t readSPI(boolean blocking);

//ISR(SPI_STC_vect);

#endif