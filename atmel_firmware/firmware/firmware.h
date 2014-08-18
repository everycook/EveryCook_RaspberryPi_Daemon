#ifndef firmware_h
#define firmware_h

#include "mytypes.h"

void controlIHTemp();
int main (void);

void checkLocks();

void initWatchDog();
void triggerWatchDog(boolean pingFromDaemon);
//ISR(WDT_vect);
uint8_t Vdebug;

#endif
