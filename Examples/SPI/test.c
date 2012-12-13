#include <bcm2835.h>
#include <stdio.h>
#include <string.h>

#define AD7794_GAIN_1		0	//2.5V
#define AD7794_GAIN_2		1	//1.25V
#define AD7794_GAIN_4		2	//625mV
#define AD7794_GAIN_8		3	//312.5mV
#define AD7794_GAIN_16		4	//156_2mV
#define AD7794_GAIN_32		5	//78.125mV
#define AD7794_GAIN_64		6	//39.06mV
#define AD7794_GAIN_128		7	//19.53mV

int currentInput=0;
//int gainSetting=AD7794_GAIN_64;
int gainSetting=AD7794_GAIN_1;
//int gainSetting=AD7794_GAIN_128;



#define RDY_PIN RPI_GPIO_P1_21
/*
char *byte_to_binary2(int x) {
    char b[9] = {'\0','\0','\0','\0','\0','\0','\0','\0'};
	char *p = b;
    int z;
    for (z = 128; z > 0; z >>= 1){
		*p++ = (x & z) ? '1' : '0';
    }

    return b;
}
const char *byte_to_binary(int x){
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}


void transfer_log(char buf[], int size){
	int i;
	printf("send:  ");
	for (i=0; i<size; ++i){
		printf(" %02X", buf[i]);
		printf(" %s", byte_to_binary(buf[i]));
	}
	bcm2835_spi_transfern(buf, size);
	
	printf("\nrecive:");
	for (i=0; i<size; ++i){
		printf(" %02X", buf[i]);
		printf(" %s", byte_to_binary(buf[i]));
	}
	printf("\n");
}
*/

void transfer(char buf[], int size){
	bcm2835_spi_transfern(buf, size);
	//transfer_log(buf, size);
}

int readStatus(){
	int currentStatus = 0;

	char buf[] = { 0x40, 0x00}; //select status reg for read, read status
	transfer(buf, sizeof(buf));
	
	currentStatus = buf[1];
	return currentStatus;
}

int readConfig(){
	int fullConfig = 0;

	char buf[] = { 0x50, 0x00, 0x00}; //select configuration reg for read, read configuration
	transfer(buf, sizeof(buf));

	fullConfig = buf[1];
	fullConfig = (fullConfig << 8) | buf[2];
	return fullConfig;
}

int readMode(){
	int fullMode = 0;

	char buf[] = { 0x48, 0x00, 0x00}; //select Mode reg for read, read Mode
	transfer(buf, sizeof(buf));

	fullMode = buf[1];
	fullMode = (fullMode << 8) | buf[2];
	return fullMode;
}

void softReset(){
	char buf[] = { 0xFF, 0xFF, 0xFF, 0xFF}; //reset by send 32 times 1:
	transfer(buf, sizeof(buf));
    //printf("reset: %02X %02X %02X %02X \n", buf[0], buf[1], buf[2], buf[3]);
	
	//reconfigure
	char bufConfig[] = { 0x10, 0x00 + gainSetting, 0x10 + currentInput}; //select comunication reg for write,  write config to comunication reg
	transfer(bufConfig, sizeof(bufConfig));
	
}

void changeInput(int input){
	if (currentInput != input){
		currentInput = input;
		
		//reconfigure
		char bufConfig[] = { 0x10, 0x00 + gainSetting, 0x10 + currentInput}; //select comunication reg for write,  write config to comunication reg
		transfer(bufConfig, sizeof(bufConfig));
		//printf("now on input %d\n", currentInput);
	}
}

void changeGain(int gain){
	if (gainSetting != gain){
		currentInput = gain;
		
		//reconfigure
		char bufConfig[] = { 0x10, 0x00 + gainSetting, 0x10 + currentInput}; //select comunication reg for write,  write config to comunication reg
		transfer(bufConfig, sizeof(bufConfig));
	}
}

long readSingleValue(){
	char bufDataRead[] = { 0x58 }; //select Data reg for read
	transfer(bufDataRead, sizeof(bufDataRead));
	/*
	int rdy=1;
	int rdyTimes=0;
	while(rdy==1 && rdyTimes<10000){
		delayMicroseconds(100);
		rdy = bcm2835_gpio_lev(RDY_PIN);
		++rdyTimes;
	}
	printf("%d times check if ready\n",rdyTimes);
	*/
	
	char bufDataValue[] = { 0x00, 0x00, 0x00}; //read Data
	transfer(bufDataValue, sizeof(bufDataValue));
	
	long fullData = bufDataValue[0];
	fullData = (fullData << 8) | bufDataValue[1];
	fullData = (fullData << 8) | bufDataValue[2];
	return fullData;
}


long readMultiValueAVG(int amount, int delayMilis){
	long value = 0;
	
	int valuesRead=0;
	int emptyCount=0;
	long fullData=0;
	while (emptyCount<20 && valuesRead<amount) {
		char bufDataRead[] = { 0x58 }; //select Data reg for read
		transfer(bufDataRead, sizeof(bufDataRead));
		/*
		int rdy=1;
		int rdyTimes=0;
		while(rdy==1 && rdyTimes<10000){
			delayMicroseconds(100);
			rdy = bcm2835_gpio_lev(RDY_PIN);
			++rdyTimes;
		}
		printf("%d times check if ready\n",rdyTimes);
		*/
		
		char bufDataValue[] = { 0x00, 0x00, 0x00}; //read Data
		transfer(bufDataValue, sizeof(bufDataValue));
		
		fullData = bufDataValue[0];
		fullData = (fullData << 8) | bufDataValue[1];
		fullData = (fullData << 8) | bufDataValue[2];
		if (fullData>0){
			value +=fullData;
			++valuesRead;
		} else {
			++emptyCount;
		}
		/*if (valuesRead>0){
			printf("Value at input %d: %d sum %d avg %d\n", currentInput, fullData, value, value / valuesRead);
		} else {
			printf("Value at input %d: %d sum %d\n", currentInput, fullData, value);
		}*/
		if (delayMilis>0){
			delay(delayMilis);
		}
	}
	
	if (valuesRead == 0){
		return -1;
	} else {
		return value / valuesRead;
	}
}

//this one is slow and unsave....
long readMultiValueContinuousAVG(int amount, int delayMilis){
	long value = 0;
	char bufDataRead[] = { 0x5C }; //select Data reg for continous read 
	transfer(bufDataRead, sizeof(bufDataRead));
	
	int valuesRead=0;
	int emptyCount=0;
	long fullData=0;
	int rdy=1;
	while (emptyCount<20 && valuesRead<amount) {
		/*
		char buf[] = { 0x00 };
		transfer(buf, 1);
		
		if (buf[0] == 0xFF){
			delay(100); //Data not yet available, wait 100ms
			++emptyCount;
		} else {
			fullData = buf[0];
			
			char buf[] = { 0x00, 0x00 };
			transfer(buf, 2);
			
			fullData = (fullData << 8) | buf[0];
			fullData = (fullData << 8) | buf[1];
			if (fullData != 0){ //it's not possible to be value 0
				value += fullData;
				valuesRead++;
			} else {
				++emptyCount;
			}
		}
		*/
		
		/*
		char buf[] = { 0x00, 0x00, 0x00 };
		transfer(buf, 3);
		
		fullData = buf[0];
		fullData = (fullData << 8) | buf[1];
		fullData = (fullData << 8) | buf[2];
		if (fullData != 0 && fullData != 0xFFFFFF){
			value += fullData;
			valuesRead++;
		}
		*/
		
		/* //This is not the correct way for check rdy-pin is fall low
		rdy=1;
		int rdyTimes=0;
		while(rdy==1 && rdyTimes<10000){
			delayMicroseconds(100);
			rdy = bcm2835_gpio_lev(RDY_PIN);
			++rdyTimes;
		}
		printf("%d times check if ready\n",rdyTimes);
		
		char buf[] = { 0x00, 0x00, 0x00 };
		transfer(buf, 3);
		
		fullData = buf[0];
		fullData = (fullData << 8) | buf[1];
		fullData = (fullData << 8) | buf[2];
		value += fullData;
		valuesRead++;
		*/
		
		/* //logic error, this wouldn't use value if a byte of it have realy the value 0xFF
		if (buf[0] == 0xFF){
			nanosleep(100000); //Data not yet available, wait 100ms
			emptyCount++;
			if (valIndex != 0){
				printf("uncomplete data");
				valIndex=0;
			}
		} else {
			value[valIndex] = buf[0];
			if (valIndex<2) {
				valIndex++;
			} else {
				long fullData = value[0];
				fullData = (fullData << 8) | value[1];
				fullData = (fullData << 8) | value[2];
				valIndex=0;
				value += fullData;
				valuesRead++;
			}
		}
		*/
		
		/*if (valuesRead>0){
			printf("Value at input %d: %d / %06X sum %d avg %d\n", currentInput, fullData, fullData, value, value / valuesRead);
		} else {
			printf("Value at input %d: %d / %06X sum %d\n", currentInput, fullData, fullData, value);
		}*/
		if (delayMilis>0){
			delay(delayMilis);
		}
	}
	
	char bufDataEndRead[] = { 0x58, 0x00, 0x00, 0x00 }; //stop continous read -> select Data reg for read   and read 3 bytes(to be sure it's back in command promt mode.)
	transfer(bufDataEndRead, sizeof(bufDataEndRead));
	
	if (valuesRead == 0){
		return -1;
	} else {
		return value / valuesRead;
	}
}


long readSingleValueFromInput(int input){
	changeInput(input);
	delay(120);
	return readSingleValue();
}


long readAvgValueFromInput(int input, int avgOver, int delayMilis){
	changeInput(input);
	delay(120);
	return readMultiValueAVG(avgOver, delayMilis);
}

int main(int argc, char **argv)
{
	if (!bcm2835_init())
		return 1;

	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST); // The default
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0); // The default
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_65536); // The default
	bcm2835_spi_chipSelect(BCM2835_SPI_CS0); // The default
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW); // the default
	
	softReset();
	/*
	printf("status: %d \n", readStatus());
	printf("config: %d \n", readConfig());
	printf("mode: %d \n", readMode());
	printf("gain: %d \n", gainSetting);
	*/
	
	//changeInput(0);
	//changeGain(AD7794_GAIN_8);
	//delay(120); //Time to value is ready
	
	long fullData;
	int readAmount = 1;
	int delayMilis=20;
	int loopc=0;
	while(1){
		long input0 = readAvgValueFromInput(0, readAmount, delayMilis);
		//printf("Value at input %d: %d / %06X\n", currentInput, input0, input0);
//		printf("Value %d: %d ", currentInput, input0, input0);
		printf("LoadCell %d: %d ", currentInput, input0, input0);
		long input1 = readAvgValueFromInput(1, readAmount, delayMilis);
		//printf("Value at input %d: %d / %06X\n", currentInput, input1, input1);
		printf("LoadCell %d: %d ", currentInput, input1, input1);
		long input2 = readAvgValueFromInput(2, readAmount, delayMilis);
		//printf("Value at input %d: %d / %06X\n", currentInput, input2, input2);
		printf("LoadCell %d: %d ", currentInput, input2, input2);
		long input3 = readAvgValueFromInput(3, readAmount, delayMilis);
		//printf("Value at input %d: %d / %06X\n", currentInput, input3, input3);
		printf("LoadCell %d: %d ", currentInput, input3, input3);
		
		long avg = (input1 + input2 + input3) / 300;
//		printf("avg: %d,  ", avg, avg);
		
		long press = readAvgValueFromInput(4, readAmount, delayMilis); //needs gain 1
		printf("Press %d: %d ", currentInput, press);
		
		long temp = readAvgValueFromInput(5, readAmount, delayMilis); //needs gain 1
		printf("Temp %d: %d \n", currentInput, temp);
		
		//fullData = readSingleValue();
		//printf("Value at input %d: %d / %06X\n", currentInput, fullData, fullData);
		
		/*
		fullData = readMultiValueAVG(readAmount, delayMilis);
		printf("Value at input %d: %d / %06X\n", currentInput, fullData, fullData);
		
		printf("status: %d \n", readStatus());
		++loopc;
		if (loopc>=10){
			if (currentInput<3){
				changeInput(currentInput+1);
			} else {
				changeInput(0);
			}
			delay(120);
			loopc=0;
		} else {
			//delay(delayMilis);
		}*/
	}
	
	bcm2835_spi_end();
	return 0;
}
