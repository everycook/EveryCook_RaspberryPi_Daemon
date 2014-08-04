#ifndef status_h
#define status_h

#include "mytypes.h"

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

extern uint8_t StatusByte;

#endif