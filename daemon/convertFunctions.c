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

#include <math.h>

#include "convertFunctions.h"

/* Convert a number to a string
 *
 */
void NumberConvertToString(uint32_t num, char *str){
	uint8_t i = 0;
	uint32_t value, mutiplecand = 1;

	value = num;
	do {
		mutiplecand = mutiplecand*10;
		i++;
	//} while (value >= mutiplecand || i > 9); //TODO: this would get en endloos loop if i>9...
	} while (value >= mutiplecand);
	
	while (mutiplecand != 1){
		mutiplecand = mutiplecand/10;
		*str++ = num/mutiplecand+48;
		num = num%mutiplecand;
	}
}

/* Clean the string
 *
 */
void StringClean(char *str, uint32_t len){
	uint32_t i = 0;

	for (; i< len; i++){
		str[i] = 0x00;
	}
}

/* Combine two strings to one string
 *
 */
void StringUnion(char *fristString, char *secondString){
	uint8_t i = 0, fristEndPtr = 0;

	while (fristString[fristEndPtr]){
		fristEndPtr++;
	}
	while (secondString[i]){
		fristString[fristEndPtr+i] = secondString[i];
		i++;
 	}
}

/* convert a string to a number
 *
 */
uint32_t StringConvertToNumber(char *str){
	uint32_t value = 0 ,len = 0, mutiple = 1;

	while (str[len]){
		len++;
		mutiple *= 10;
	}
	len = 0;	
	while (str[len]){
		mutiple = mutiple/10;
		value = value + (str[len]-48)*mutiple;
		len++;
	}
	return value;
}

double StringConvertToDouble(char *str){
	double value = 0.0, mutiple = 1.0;
	uint32_t len = 0;

	while (str[len]){
		if (str[len] == '.'){
			break;
		}
		len++;
		mutiple *= 10.0;
	}
	len = 0;	
	while (str[len]){
		if (str[len] != '.'){
			mutiple = mutiple/10.0;
			value = value + (str[len]-48)*mutiple;
		}
		len++;
	}
	return value;
}

/*
*/
int POWNTimes(uint32_t num, uint8_t n){
	int i = 0;

	while (num > 1){
		num = num / n;
		i++;
	}
	return i;
}
