#include <avr/interrupt.h>
#include <util/delay.h>

#include "heating.h"
#include "input.h"
#include "status.h"
#include "time.h"
#include "firmware.h"
struct pinInfo IHTempSensor = PA_1; //Analog

struct pinInfo isIHOn = PD_4; //in
struct pinInfo IHOn = PD_6; //out
struct pinInfo IHFanPWM = PD_7; //pwm	//OC2A

uint8_t IHFanPWM_TIMER = TIMER2A;

uint8_t lastIHFanPWM = 0;

long timeButLow =0;
int pressButDuration=500;
int butStatus=0;

uint16_t ihTemp = 0;
uint8_t ihTemp8bit;
void Heating_controlIHTemp(){
	//controll IHTemp
	ihTemp = analogRead(IHTempSensor);
	ihTemp8bit = ihTemp >> 2;
	uint8_t IHFanPWMValue = 0;
	if (ihTemp8bit > 150){//27°
		IHFanPWMValue = 100+((ihTemp8bit-150)*3);
	}
	if (IHFanPWMValue != 0){
		StatusByte |= _BV(SB_IHFanOn);
	}else{
		StatusByte&= ~_BV(SB_IHFanOn);
	}
	if (lastIHFanPWM != IHFanPWMValue){
		analogWrite(IHFanPWM_TIMER, IHFanPWMValue);
		lastIHFanPWM = IHFanPWMValue;
	}
}





//IH vars
boolean heatingLed;
boolean isHeating = false;
boolean heatingCommand = false;
#define NBCYCLE 200
uint8_t i;
void Heating_init(){
	//Set Pin Modes
	pinMode(IHTempSensor, INPUT/*_ANALOG*/);
	pinMode(isIHOn, INPUT);
	pinMode(IHFanPWM, OUTPUT/*_PWM*/);
	pinMode(IHOn, OUTPUT);
	digitalWrite(IHOn,HIGH);
	//digitalWrite(IHOn,LOW);
	cli();
	//Enable ADC (for IHTempSensor)
	ADCSRA |= _BV(ADEN);		//ATmega_644.pdf, Page 249
		
	//Set Phase-Correct PWM mode for OC2A / OC2B 		//ATmega_644.pdf, Page 148
	TCCR2A |= _BV(WGM20);
	//Set prescaling 64, this enable the timer
	TCCR2B |= _BV(CS10) | _BV(CS20);
	sei();
	
}


void Heating_setHeating(boolean on){
	heatingCommand=on;
}
bool heating_GetisHeating(){
	return isHeating;
}
void Heating_heatControl(){
	isHeating=true;
	for(i=0;i<NBCYCLE;i++){
		heatingLed=digitalRead(isIHOn);
		if(heatingLed){
			isHeating=false;
		}	
	}
	if(isHeating!=heatingCommand && butStatus==0){
		digitalWrite(IHOn,LOW);
		timeButLow=millis();
		butStatus=1;
		
	 }
	if (millis()>=timeButLow+pressButDuration) {
		digitalWrite(IHOn,HIGH);
			if (millis()>=timeButLow+pressButDuration*4) butStatus=0;
	}
	if (isHeating){
		StatusByte |= _BV(SB_isIHOn);
	} else {
		StatusByte&= ~_BV(SB_isIHOn);
	}
}
