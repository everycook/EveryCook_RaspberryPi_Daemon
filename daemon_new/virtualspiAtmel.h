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

/* Header of the virtualspi.
 *
 */

#include "bool.h"
#include "daemon.h"

//Pin defination for the SPI (gpio numbers)
#define MOSI_ATMEL	18
#define MISO_ATMEL	23
#define SCLK_ATMEL	24
//#define CS_ATMEL	21	//is fix set to ground=>selected
/*
//Pin defination for the SPI (physics numbers)
#define MOSI_ATMEL	12
#define MISO_ATMEL	16
#define SCLK_ATMEL	18
//#define CS_ATMEL	13	//is fix set to ground=>selected
*/
#define ATMEL_RESET	4
//micro secounds -> 1 milis
#define SPI_ATMEL_CLOCK_DELAY	1000
//mili secounds
#define SPI_ATMEL_CS_DELAY		500



//Status Byte Bit possition
#define SB_CommandOK		0
#define SB_LidClosed		1
#define SB_LidLocked		2
#define SB_PusherLocked		3
#define SB_isIHOn			4
#define SB_IHFanOn			5
#define SB_MotorStoped		6
#define SB_CommandError		7

#define SPI_CommandOK 0x01
#define SPI_NoResponse 0x7E
#define SPI_Error_WDT_Reset (0x80 | 0x02)

#define SPI_Error_Unknown_Command 				(0x80 | 0x04)
#define SPI_Error_Text_Invalid 			 (0x80 | 0x08 | 0x04)
#define SPI_Error_Picture_Invalid (0x80 | 0x10 | 0x08 | 0x04)


#define SPI_MODE_IDLE						0
#define SPI_MODE_GET_STATUS					1
#define SPI_MODE_MOTOR						2
#define SPI_MODE_HEATING					3
#define SPI_MODE_VENTIL						10
#define SPI_MODE_MAINTENANCE				11
#define SPI_MODE_GET_MOTOR_SPEED			12
#define SPI_MODE_GET_IGBT_TEMP				13
#define SPI_MODE_GET_HEATING_OUTPUT_LEVEL	14
#define SPI_MODE_GET_MOTOR_POS_SENSOR		15
#define SPI_MODE_GET_MOTOR_RPM				16

#ifdef __cplusplus
extern "C" {
#endif

//virtualSPI function delaration
/** @brief initialization PIN for SPI and debug=debugEnabled
 *  @param debugEnabled 
 */
void VirtualSPIAtmelInit(bool debugEnabled);
void SPIAtmelReset(void);
uint8_t SPIAtmelWrite(uint8_t data);	
uint8_t SPIAtmelRead(void);

uint8_t atmelGetStatus();

void atmelSetMaintenance(bool on);

uint8_t atmelGetIGBTTemp();

uint8_t getResult();
uint8_t getValidResultOrReset();
uint8_t getValidResultOrResetAdditionalValid(uint8_t validByte);

void virtualAtmelStopSPI();
void virtualAtmelStartSPI();
void virtualSpiAtmelSetNewMode(uint8_t newMode);
bool atmelGetDebug();
bool atmelGetDebug2();


#ifdef __cplusplus
}
#endif
