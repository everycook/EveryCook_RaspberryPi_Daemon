#include <avr/interrupt.h>

#include "motor.h"
#include "input.h"
#include "time.h"
#include "mytypes.h"
#include "status.h"

struct pinInfo MotorPosSensor = PA_2; //in
struct pinInfo MotorPWM = PD_5; //pwm	//OC1A

uint8_t MotorPWM_TIMER = TIMER1A;

uint8_t stopSpeed = 256/6;
int slowRotationTime=500;


boolean motorStop = true;
boolean motorStoped = false;
uint8_t motorSpeed = 0;



//Motor vars
long lastSensorTrigger = 0;
boolean lastSensorValue=false;

long previousMillis = 0;
long interval = 1000;   
int rotationTime=0;

uint8_t sensorValue = 0;        
int outputValueMotor = 0;        // value output to the PWM (analog out)
int rpm=0;


void Motor_init(){
	//Set Pin Modes
	pinMode(MotorPosSensor, INPUT_PULLUP);
	pinMode(MotorPWM, OUTPUT/*_PWM*/);
	analogWrite(MotorPWM_TIMER, 0);
	
	
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
		if (sensorValue &&!lastSensorValue) {
			rotationTime=millis()-lastSensorTrigger;
			rpm=60000/rotationTime;
			lastSensorTrigger=millis();
			if (motorStop && rotationTime>=slowRotationTime) {
				outputValueMotor=0;
				rpm=0;
				motorStoped = true;
			}
		}
		if (outputValueMotor == 0){
			StatusByte |= _BV(SB_MotorStoped);
		} else {
			StatusByte&= ~_BV(SB_MotorStoped);
		}
		lastSensorValue=sensorValue;

		analogWrite(MotorPWM_TIMER, outputValueMotor);
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
	} else {
		lastSensorValue = digitalRead(MotorPosSensor);
		if (lastSensorValue){
			if (motorStop) {
				motorStoped = true;
			}
			StatusByte |= _BV(SB_MotorStoped);
		} else {
			StatusByte&= ~_BV(SB_MotorStoped);
		}
	}

}
