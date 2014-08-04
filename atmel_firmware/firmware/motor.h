#ifndef motor_h
#define motor_h

#include "mytypes.h" //for boolean

//namespace Motor
//{
  extern void Motor_init();
  extern void Motor_motorControl();
  extern void Motor_setMotor(uint8_t speed);
  extern boolean Motor_isStopped();
  
  extern uint8_t MotorPWM_TIMER;
  extern uint8_t motorSpeed;
  extern int outputValueMotor;
  extern boolean lastSensorValue;
  extern int rpm;
//}

#endif


