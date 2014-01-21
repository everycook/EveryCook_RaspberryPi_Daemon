#ifndef firmware_h
#define firmware_h

typedef uint8_t boolean;

void controlIHTemp();
int main (void);
void initWatchDog();
void triggerWatchDog(boolean pingFromDaemon);

#endif