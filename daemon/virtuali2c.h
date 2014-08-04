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

/* Header of the virtuali2c.
 *
 */

//Pin defination for the I2C (gpio numbers)
#define SDA	      2
#define SCL	      3
/*
//Pin defination for the I2C (physics numbers)
#define SDA	      3
#define SCL	      5
*/

//I2C_DELAY_TIME
#define DELAY_TIME   10

#ifdef __cplusplus
extern "C" {
#endif

/** @brief virtualI2C function delaration
*/
void VirtualI2CInit(void);
void I2CStart(void);
void I2CStop(void);
int CheckAck(void);
uint8_t I2CReadByte(void);
/** @brief I2CWriteByte: Write one Byte to the PCA9685
 *  @param data:Data to the PCA9685
 */
void I2CWriteByte(uint8_t data);
/** @brief I2CWriteBytes: Write one Byte to the PCA9685
 *  @param data:The frist byte must be the address of the 'CHIP', then the data to the chip.
 *  @param len : data size
 */
void I2CWriteBytes(uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif


