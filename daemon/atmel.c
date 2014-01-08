#include <atmel.h>
#include <stdio.h>
#include <stdint.h>
#include <wiringSerial.h>
#include <string.h>


#include <sys/types.h>
#include <sys/stat.h>

int atmelConnenct(char* devPath){
	int fd;
	if ((fd = serialOpen (devPath, 9600)) < 0){
		//fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
		//fprintf (stderr, "Unable to open serial device: %d\n", errno) ;
		fprintf (stderr, "Unable to open serial device: %d\n", fd) ;
		return -1;
	}
	return fd;
}

void atmelClose(int fd){
	serialClose(fd);
}


void atmelClear(int fd){
	serialPutchar(fd, 0x00);
}

void atmelShowText(int fd, char* text){
	serialPutchar(fd, 0x01);
	uint8_t len = strlen(text);
	serialPutchar(fd, len);
	serialPuts(fd, text);
	//serialPutchar(fd, 0x00);
}

void atmelShowPercent(int fd, uint8_t percent){
	serialPutchar(fd, 0x02);
	serialPutchar(fd, percent);
}

void atmelShowPicture(int fd, uint16_t* picture){
	serialPutchar(fd, 0x03);
	uint8_t i=0;
	for(; i<9;++i){
		serialPutchar(fd, (picture[i] >> 8) & 0xFF);
		serialPutchar(fd, picture[i] & 0xFF);
	}
}

void atmelShowPercentText(int fd, uint8_t percent, char* text){
	serialPutchar(fd, 0x04);
	serialPutchar(fd, percent);
	uint8_t len = strlen(text);
	serialPutchar(fd, len);
	serialPuts(fd, text);
	//serialPutchar(fd, 0x00);
}