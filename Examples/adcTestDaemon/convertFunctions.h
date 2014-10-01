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

#include <stdint.h>
/** @brief Clean the string
 */
void StringClean(char *str, uint32_t len);

/** @brief convert a string to a number
 */
uint32_t StringConvertToNumber(const char *str);

/** @brief convert a string to a double
 */
double StringConvertToDouble(char *str);

/** @brief calculates n^i that is the higher and lower than num
 *  @param num :number to divide
 *  @param n : divisor
 *  @return i : max(n^i)<num
*/
int POWNTimes(uint32_t num, uint8_t n);

char* my_itoa(int val, int base);
