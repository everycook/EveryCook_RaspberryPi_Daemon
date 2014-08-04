#include <stdint.h>

int atmelConnenct(char* devPath);
void atmelClose(int fd);
void atmelClear(int fd);
void atmelShowText(int fd, char* text);
void atmelShowPercent(int fd, uint8_t percent);
void atmelShowPicture(int fd, uint16_t* picture);
void atmelShowPercentText(int fd, uint8_t percent, char* text);