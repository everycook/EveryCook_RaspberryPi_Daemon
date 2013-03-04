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
