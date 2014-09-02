#include <avr/interrupt.h>
#include <util/delay.h>
#include "motor.h"
#include "input.h"
#include "time.h"
#include "mytypes.h"
#include "status.h"
#include "firmware.h"

struct pinInfo MotorPosSensor = PA_2; //in
struct pinInfo MotorPWM = PD_5; //pwm	//OC1A

//uint8_t MotorPWM_TIMER = TIMER1A;

uint8_t stopSpeed = 256/6;
int slowRotationTime=500;


boolean motorStop = true;
boolean motorStoped = false;
uint8_t motorSpeed = 0;



//Motor vars



long previousMillis = 0;
long interval = 1000;   
int rotationTime=0;

uint8_t sensorValue = 0;        
int outputValueMotor = 0;        // value output to the PWM (analog out)

long lastSensorTrigger = 0;
boolean lastSensorValue=false;
int rpm=0;
//new code
#define STOP 0
#define RUNNING 1
#define WILLSTOP 2
#define SPEEDMINMOTOR 60
int mode=STOP;
uint8_t speedRequired;
long startStopTime;
long timeNow;
uint8_t chrono=0;
long startChrono;
void Motor_init(){
	//Set Pin Modes
	pinMode(MotorPosSensor, INPUT_PULLUP);
	pinMode(MotorPWM, OUTPUT/*_PWM*/);
	//analogWrite(MotorPWM_TIMER, 0);
	
	
	cli();
	//Set Phase-Correct PWM mode for OC1A / OC1B 8bit mode 		//ATmega_644.pdf, Page 127
	TCCR1A |= _BV(WGM10);
	
	TCCR1B |= _BV(CS10) | _BV(CS20) ; //=685 Hz
	sei();
}


void Motor_setMotor(uint8_t speed){
	if(speed>SPEEDMINMOTOR){
		mode=RUNNING;
		speedRequired=speed;
	}else{
		if(mode==RUNNING){
			mode=WILLSTOP;
			startStopTime=millis();
		}
	}
}

boolean Motor_isStopped(){
	return motorStoped;
}

void Motor_motorControl() {
	sensorValue = digitalRead(MotorPosSensor);
	if(mode==RUNNING || mode==WILLSTOP){
        if (sensorValue==0 && lastSensorValue==1) {
			rotationTime=millis()-lastSensorTrigger;
			rpm=60000/rotationTime;
			//Vdebug=rpm;
			lastSensorTrigger=millis();
		}
	}else{
		rpm=0;
	}
	switch (mode){
		case RUNNING :
			//analogWrite(MotorPWM_TIMER, speedRequired);
			digitalWrite(MotorPWM,HIGH);
		break;
		case STOP :
			//analogWrite(MotorPWM_TIMER, 0);
			digitalWrite(MotorPWM,LOW);
		break;
		case WILLSTOP :
			//analogWrite(MotorPWM_TIMER, SPEEDMINMOTOR);
			timeNow=millis();
			if((timeNow-startStopTime>5000) || (sensorValue==1 && lastSensorValue==0)){
				mode=STOP;
				_delay_ms(40);
				digitalWrite(MotorPWM,LOW);
			}
		break;
	}	
	lastSensorValue=sensorValue;
	if(mode==STOP){	
		StatusByte |= _BV(SB_MotorStoped);
    } else {
        StatusByte&= ~_BV(SB_MotorStoped);
    }

	
	/*
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
*/
}
