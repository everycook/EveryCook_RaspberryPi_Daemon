#ifndef firmware_h
#define firmware_h

#include "mytypes.h"

void controlIHTemp();
int main (void);
void initWatchDog();
void triggerWatchDog(boolean pingFromDaemon);

#endif