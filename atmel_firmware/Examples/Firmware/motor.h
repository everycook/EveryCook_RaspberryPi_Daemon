#ifndef motor_h
#define motor_h

#include <arduino.h> //for boolean

namespace Motor
{
  extern void init();
  extern void motorControl();
  extern void setMotor(uint8_t speed);
  extern boolean isStopped();
}

#endif


