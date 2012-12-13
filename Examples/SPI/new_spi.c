#include <bcm2835.h>
#include <stdio.h>

#define AD7794_GAIN_1		0	//2.5V
#define AD7794_GAIN_2		1	//1.25V
#define AD7794_GAIN_4		2	//625mV
#define AD7794_GAIN_8		3	//312.5mV
#define AD7794_GAIN_16		4	//156_2mV
#define AD7794_GAIN_32		5	//78.125mV
#define AD7794_GAIN_64		6	//39.06mV
#define AD7794_GAIN_128		7	//19.53mV

int currentInput=-1;
int gainSetting=AD7794_GAIN_1;

int readStatus(){
	int currentStatus = 0;

	char buf[] = { 0x40, 0x00}; //select status reg for read, read status
	bcm2835_spi_transfern(buf, sizeof(buf));
	
	currentStatus = buf[1];
	return currentStatus;
}

int readConfig(){
	int fullConfig = 0;

	char buf[] = { 0x50, 0x00, 0x00}; //select configuration reg for read, read configuration
	bcm2835_spi_transfern(buf, sizeof(buf));

	fullConfig = buf[1];
	fullConfig = (fullConfig << 8) | buf[2];
	return fullConfig;
}

int readMode(){
	int fullMode = 0;

	char buf[] = { 0x48, 0x00, 0x00}; //select Mode reg for read, read Mode
	bcm2835_spi_transfern(buf, sizeof(buf));

	fullMode = buf[1];
	fullMode = (fullMode << 8) | buf[2];
	return fullMode;
}

void softReset(){
	char buf[] = { 0xFF, 0xFF, 0xFF, 0xFF}; //reset by send 32 times 1:
	bcm2835_spi_transfern(buf, sizeof(buf));
    //printf("reset: %02X %02X %02X %02X \n", buf[0], buf[1], buf[2], buf[3]);
	
	//reconfigure
	char bufConfig[] = { 0x10, 0x00 + gainSetting, 0x10 + currentInput}; //select comunication reg for write,  write config to comunication reg
	bcm2835_spi_transfern(bufConfig, sizeof(bufConfig));
	
}

void changeInput(int input){
	if (currentInput != input){
		currentInput = input;
		
		//reconfigure
		char bufConfig[] = { 0x10, 0x00 + gainSetting, 0x10 + currentInput}; //select comunication reg for write,  write config to comunication reg
		bcm2835_spi_transfern(bufConfig, sizeof(bufConfig));
	}
}

void changeGain(int gain){
	if (gainSetting != gain){
		currentInput = gain;
		
		//reconfigure
		char bufConfig[] = { 0x10, 0x00 + gainSetting, 0x10 + currentInput}; //select comunication reg for write,  write config to comunication reg
		bcm2835_spi_transfern(bufConfig, sizeof(bufConfig));
	}
}

long readSingleValue(){
	char bufDataRead[] = { 0x58, 0x00, 0x00, 0x00}; //select Data reg for read, read Data
	bcm2835_spi_transfern(bufDataRead, sizeof(bufDataRead));
	
	long fullData = bufDataRead[1];
	fullData = (fullData << 8) | bufDataRead[2];
	fullData = (fullData << 8) | bufDataRead[3];
	return fullData;
}

long readMultiValueAVG(int amount){
	long value = 0;
	char bufDataRead[] = { 0x5C }; //select Data reg for continous read 
	bcm2835_spi_transfern(bufDataRead, sizeof(bufDataRead));
	
	int valuesRead=0;
	int emptyCount=0;
	//int valIndex=0;
	//char value[] = { 0x00, 0x00, 0x00 };
	while (emptyCount<20 && valuesRead<amount) {
		char buf[] = { 0x00 };
		bcm2835_spi_transfern(buf, 1);
		
		if (buf[0] == 0xFF){
			nanosleep(100000); //Data not yet available, wait 100ms
			emptyCount++;
		} else {
			long fullData = buf[0];
			
			char buf[] = { 0x00, 0x00 };
			bcm2835_spi_transfern(buf, 2);
			
			fullData = (fullData << 8) | buf[0];
			fullData = (fullData << 8) | buf[1];
			value += fullData;
			valuesRead++;
		}
		
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
	}
	
	char bufDataEndRead[] = { 0x58, 0x00, 0x00, 0x00 }; //stop continous read -> select Data reg for read   and read 3 bytes(to be sure it's back in command promt mode.)
	bcm2835_spi_transfern(bufDataEndRead, sizeof(bufDataEndRead));
	
	if (valuesRead == 0){
		return -1;
	} else {
		return value / valuesRead;
	}
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
	
	
    printf("status: %d \n", readStatus());
    printf("config: %d \n", readConfig());
    printf("mode: %d \n", readMode());
	
	long fullData;
	int i;
	for(i=0; i<10; ++i){
		changeInput(0);
		fullData = readSingleValue();
		printf("Value at input %d: %d \n", currentInput, fullData);
		
		changeInput(1);
		fullData = readSingleValue();
		printf("Value at input %d: %d \n", currentInput, fullData);
		
		changeInput(2);
		fullData = readSingleValue();
		printf("Value at input %d: %d \n", currentInput, fullData);
		
		changeInput(3);
		fullData = readSingleValue();
		printf("Value at input %d: %d \n", currentInput, fullData);
	}
	
	int readAmount = 10;
	for(i=0; i<10; ++i){
		changeInput(0);
		fullData = readMultiValueAVG(readAmount);
		printf("Value at input %d(avg over %d): %d \n", currentInput, readAmount, fullData);
		
		changeInput(1);
		fullData = readMultiValueAVG(readAmount);
		printf("Value at input %d(avg over %d): %d \n", currentInput, readAmount, fullData);
		
		changeInput(2);
		fullData = readMultiValueAVG(readAmount);
		printf("Value at input %d(avg over %d): %d \n", currentInput, readAmount, fullData);
		
		changeInput(3);
		fullData = readMultiValueAVG(readAmount);
		printf("Value at input %d(avg over %d): %d \n", currentInput, readAmount, fullData);
	}
	
	bcm2835_spi_end();
	return 0;
}