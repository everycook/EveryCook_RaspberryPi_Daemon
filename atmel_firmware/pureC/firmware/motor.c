#include <avr/interrupt.h>

#include "motor.h"
#include "input.h"
#include "time.h"
#include "mytypes.h"

struct pinInfo MotorPosSensor = PA_2; //in
struct pinInfo MotorPWM = PD_5; //pwm	//OC1B

uint8_t MotorPWM_TIMER = TIMER1B;

uint8_t stopSpeed = 256/6;
int slowRotationTime=500;


boolean motorStop = false;
boolean motorStoped = false;
uint8_t motorSpeed = 0;



//Motor vars
long LastSensorTrigger = 0;
boolean LastSensorValue=false;

long previousMillis = 0;
long interval = 1000;   
int rotationTime=0;

int sensorValue = 0;        
int outputValueMotor = 0;        // value output to the PWM (analog out)
int setValue=0;
int rpm=0;


void Motor_init(){
	//Set Pin Modes
	pinMode(MotorPosSensor, INPUT_PULLUP);
	pinMode(MotorPWM, OUTPUT/*_PWM*/);
	
	cli();
	
	//Set Phase-Correct PWM mode for OC1A / OC1B 8bit mode 		//ATmega_644.pdf, Page 127
	TCCR1A |= _BV(WGM10);
	//Set prescaling 64, this enable the timer //Set it to 64 because of time messurement in time.c (other values are also possible, you must call "initTime();" after change this)
	TCCR1B |= _BV(CS11) | _BV(CS10);
		
	sei();
}


void Motor_setMotor(uint8_t speed){
  motorSpeed = speed;
  motorStop = (motorSpeed == 0);
  if (motorStop){
    //outputValueMotor=min(outputValueMotor,stopSpeed);
	outputValueMotor=(outputValueMotor<stopSpeed)?outputValueMotor:stopSpeed; // min(outputValueMotor,stopSpeed);
  } else {
    outputValueMotor=motorSpeed;
motorStoped=false;
  }
}

boolean Motor_isStopped(){
  return motorStoped;
}

void Motor_motorControl() {
    if (outputValueMotor != 0){
      sensorValue = digitalRead(MotorPosSensor);
      if (sensorValue &&!LastSensorValue) {
        rotationTime=millis()-LastSensorTrigger;
        LastSensorTrigger=millis();
        if (motorStop && rotationTime>=slowRotationTime) {
          outputValueMotor=0;
          motorStoped = true;
        }
      }
      LastSensorValue=sensorValue;

      analogWrite(MotorPWM_TIMER, outputValueMotor);       
		rpm=60000/rotationTime;
/*
        // print the results to the serial monitor:        
        Serial.print("sensorvalue = " );                       
        Serial.print(sensorValue);      

        Serial.print("\t RPM = " );                       
        Serial.print(RPM);      
        Serial.print("\t rTime = " );                       
        Serial.print(rotationTime);      
        Serial.print("\t output = ");      
        Serial.println(outputValueMotor);   
*/
    }

}
