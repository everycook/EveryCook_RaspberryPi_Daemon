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

//Pin defination for the SPI (gpio numbers)
#define MOSI	10
#define MISO	9
#define SCLK	11
//CS is fix set to ground=>selected since ShieldVersion 2
#define CS	8
/*
//Pin defination for the SPI (physics numbers)
#define MOSI	19
#define MISO	21
#define SCLK	23
//CS is fix set to ground=>selected since ShieldVersion 2
#define CS		24
*/

//AD774 Register defination
#define READ_STATUS_REG		0x40
#define READ_MODE_REG		0x48
#define WRITE_MODE_REG		0x08
#define READ_CONFIG_REG		0x50
#define WRITE_CONFIG_REG	0x10
#define READ_DATA_REG		0x58
#define READ_SERIAL_REG		0x60
#define READ_IO_REG 		0x68
#define WRITE_IO_REG		0x28
#define READ_SHIFT_REG		0x70
#define READ_FULLSCALE_REG	0x78

#define internalZeroScaleCalibration	0x8000;
#define internalFullScaleCalibration	0xA000;
#define systemZeroScaleCalibration		0xC000;
#define systemFullScaleCalibration		0xE000;

#ifdef __cplusplus
extern "C" {
#endif

/*
extern void VirtualSPIInit(void);
extern void SPIReset(void);
extern void SPIWrite(uint8_t data);	
extern uint8_t SPIRead(void);
extern uint8_t SPIReadByte(uint8_t register);
extern uint32_t SPIRead2Bytes(uint8_t register);
extern uint32_t SPIRead3Bytes(uint8_t register);
*/

//virtualSPI function delaration
/** @brief VirtualSPIInit
 *  @param shieldVersion:the configured shield Version
 */
void VirtualSPIInit(uint32_t shieldVersion);
/** @brief SPIReset: Reset the AD7794 chip, write 4 0xff.
 */
void SPIReset(void);
/** @brief SPIWrite: Write one byte data to the register in AD7794
 *  @param data:datas write to register
 */
void SPIWrite(uint8_t data);	
/** @brief Read one byte data from the AD7794
 *  @return  data from register
 */
uint8_t SPIRead(void);
void SPIWriteByte(uint8_t reg, uint8_t data);
/** @brief SPIWrite2Bytes: Wirte one byte to denstination register. 
 *  @param reg : register
 *  @param data
 */
void SPIWrite2Bytes(uint8_t reg, uint32_t data);
/** @brief Get one byte from denstination register.
 *  @param reg : register
 *  @return 1 byte data
 */
uint8_t SPIReadByte(uint8_t register);
/** @brief Get 2 bytes from denstination register.
 *  @param reg : register
 * @return  2 bytes data
 */
uint32_t SPIRead2Bytes(uint8_t register);
/** @brief Get 3 bytes from denstination register.
 *  @param reg : register
 * @return  2 bytes data
 */
uint32_t SPIRead3Bytes(uint8_t register);

#ifdef __cplusplus
}
#endif
