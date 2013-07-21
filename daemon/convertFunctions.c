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

/* Clean the string
 *
 */
void StringClean(char *str, uint32_t len){
	uint32_t i = 0;

	for (; i< len; i++){
		str[i] = 0x00;
	}
}


/* convert a string to a number
 *
 */
uint32_t StringConvertToNumber(const char *str){
	uint32_t value = 0 ,len = 0, mutiple = 1;

	while (str[len]){
		if (str[len]<48 || str[len] > 57){
			break;
		}
		len++;
		mutiple *= 10;
	}
	len = 0;	
	while (str[len]){
		if (str[len]<48 || str[len] > 57){
			break;
		}
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
		if (str[len]<48 || str[len] > 57){
			break;
		}
		len++;
		mutiple *= 10.0;
	}
	len = 0;	
	while (str[len]){
		if (str[len] != '.'){
			if (str[len]<48 || str[len] > 57){
				break;
			}
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


char* my_itoa(int val, int base){
	static char buf[32]={0};
	int i=30;
	for(; val && i; --i, val /= base){
		buf[i] = "0123456789abcdef"[val % base];
	}
	return &buf[i+1];
}