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

/* Header of the virtualspiSlave.
 *
 */

//Pin defination for the SPI (gpio numbers)
#define SLAVE_MOSI	18
#define SLAVE_MISO	23
#define SLAVE_SCLK	24
//#define SLAVE_CS	?
/*
//Pin defination for the SPI (physics numbers)
#define SLAVE_MOSI	12
#define SLAVE_MISO	16
#define SLAVE_SCLK	18
//#define SLAVE_CS	?	//is fix set to ground=>selected
*/

#ifdef __cplusplus
extern "C" {
#endif


void VirtualSPISlaveInit(void);

uint8_t SPISlaveRead();

void SPISlaveWrite(uint8_t data);

#ifdef __cplusplus
}
#endif
