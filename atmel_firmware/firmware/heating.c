#include <avr/interrupt.h>
#include <util/delay.h>

#include "heating.h"
#include "input.h"
#include "status.h"
#include "time.h"
#include "firmware.h"
boolean isHeating = false;

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
uint8_t lastIhTemp8bit;
void Heating_controlIHTemp(){
	//controll IHTemp
	ihTemp = analogRead(IHTempSensor);
	ihTemp8bit = ihTemp >> 2;
	uint8_t IHFanPWMValue = 0;
	/*if (ihTemp8bit > 150){//27°
		IHFanPWMValue = 100+((ihTemp8bit-150)*3);
	}
	if(IHFanPWMValue<lastIHFanPWM){
		IHFanPWMValue=((lastIHFanPWM-IHFanPWMValue)*4/5)+IHFanPWMValue;
	}*/
	if(isHeating){
		IHFanPWMValue=200;
	}else{
		IHFanPWMValue=50;
	}
	if (IHFanPWMValue != 0){
		StatusByte |= _BV(SB_IHFanOn);
	}else{
		StatusByte&= ~_BV(SB_IHFanOn);
	}
	lastIhTemp8bit=ihTemp8bit;
	if (lastIHFanPWM != IHFanPWMValue){
		analogWrite(IHFanPWM_TIMER, IHFanPWMValue);
		lastIHFanPWM = IHFanPWMValue;
	}
}





//IH vars
boolean heatingLed;

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
	//digitalWrite(IHOn,HIGH);
	//digitalWrite(IHOn,LOW);
	isHeating=true;
	//Vdebug=heatingCommand;
	if (heatingCommand) Vdebug=33;
	else Vdebug=44;
	for(i=0;i<NBCYCLE;i++){
		heatingLed=digitalRead(isIHOn);
		//_delay_ms(20);
		if(heatingLed){
			isHeating=false;
		}
	}
	if(isHeating!=heatingCommand && butStatus==0){
		digitalWrite(IHOn,LOW);
		timeButLow=millis();
		butStatus=1;
		//digitalWrite(IHOn,HIGH);
	//	_delay_ms(20);
	//	digitalWrite(IHOn,HIGH);
		//digitalWrite(IHOn,LOW);
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
