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

/* Header of the basic_functions.
 *
 */

#include "bool.h"

#define IND_KEY_MODE	0
#define IND_KEY_MINUS	1
#define IND_KEY_PLUS	2
#define IND_KEY_POWER	3

#define IND_LED_POWER			0
#define IND_LED_MODE_KEEPWARM	1
#define IND_LED_MODE_HEATING	2
#define IND_LED_TEMP_MAX		3
#define IND_LED_TEMP_MIDDLE		4
#define IND_LED_TEMP_MIN		5

#define I2C_MOTOR				0	//ENLEVE

#define PI_PIN_BUZZER	4
#define BUZZER_PWM		1000




/** @brief modifie debug_enabled2 avec value
 *  @param value
*/
void setDebugEnabled(bool value);

/** @brief initialization PIN
 *  @param shieldVersion 
 *  @param buttonPins 
 *  @param buttonInverse 
 */
void initHardware(uint32_t shieldVersion, uint8_t* buttonPins, uint8_t* buttonInverse);

/** @brief remplit le tableau ConfigurationReg avec newConfig
 *  @param newConfig
*/
void setADCConfigReg(uint32_t newConfig[]);

/** @brief modifie ModeReg avec newModeReg
 *  @param newModeReg
*/
void setADCModeReg(uint16_t newModeReg);

/** @brief GPIO PCA9685 initialization
*/
void GPIOInit(void);

/** @brief initialization
*/
void AD7794Init(void);

/** @brief initialization according to shieldVersion
 *  @param shieldVersion
*/
void PCA9685Init(uint32_t shieldVersion);

//read/write functions
/** @brief return the register's value of ADC 
 *  @param i : register
 *  @return value of register
*/
uint32_t readADC(uint8_t i);

/** @brief read the value at Pin i
 *  @param i
 * @return value at Pin i
*/
uint32_t readSignPin(uint8_t i);

uint32_t readRaspberryPin(uint8_t i);

void writeControllButtonPin(uint8_t i, uint8_t on);

/** @brief return the value of i button
 *  @param i
 *  @return value
*/
uint32_t readButton(uint8_t i);

/** @brief write on the Pin i the value (1/0) on
 *  @param i
 *  @param on
*/
void writeRaspberryPin(uint8_t i, uint8_t on);

void buzzer(uint8_t on, uint32_t pwm);

/** @brief put value in the right structure with 
 *  @param i : what value
 *  @param value : value you want send
 */
void writeI2CPin(uint8_t i, uint32_t value);
