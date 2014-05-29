#include <avr/interrupt.h>
#include <util/delay.h>

#include "heating.h"
#include "input.h"
#include "status.h"

struct pinInfo IHTempSensor = PA_1; //Analog

//struct pinInfo IHOff = PB_4; //out, not connected/needed
struct pinInfo isIHOn = PD_3; //in
struct pinInfo IHOn = PD_4; //out
struct pinInfo IHPowerPWM = PD_6; //pwm	//OC2B
struct pinInfo IHFanPWM = PD_7; //pwm	//OC2A

uint8_t IHPowerPWM_TIMER = TIMER2B;
uint8_t IHFanPWM_TIMER = TIMER2A;

uint8_t lastIHFanPWM = 0;

uint16_t ihTemp = 0;
uint8_t ihTemp8bit;
void Heating_controlIHTemp(){
	//controll IHTemp
	ihTemp = analogRead(IHTempSensor);
	ihTemp8bit = ihTemp >> 2;
	uint8_t IHFanPWMValue = 0;
	if (ihTemp8bit > 200){
		IHFanPWMValue = 255;
	} else if (ihTemp8bit > 150){
		IHFanPWMValue = 200;
	} else if (ihTemp8bit > 100){
		IHFanPWMValue = 150;
	} else if (ihTemp8bit > 64){
		IHFanPWMValue = 100;
	} else if (ihTemp8bit < 32){
		IHFanPWMValue = 0;
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




boolean heating = false;

//IH vars
const int Increment = 5;
const int pause=100;
int initialOutputValue = 50;
boolean isHeating = false;
boolean lastIsHeating = false;
int outputValueIH = 50; //initialOutputValue
int lastOutputValueIHStart = 0;
int lastOutputValueIHRun = 0;


void Heating_init(){
	//Set Pin Modes
	pinMode(IHTempSensor, INPUT/*_ANALOG*/);
//	pinMode(IHOff, OUTPUT);
	pinMode(isIHOn, INPUT_PULLUP);
	pinMode(IHOn, OUTPUT);
	pinMode(IHPowerPWM, OUTPUT/*_PWM*/);
	pinMode(IHFanPWM, OUTPUT/*_PWM*/);
	
	cli();
	//Enable ADC (for IHTempSensor)
	ADCSRA |= _BV(ADEN);		//ATmega_644.pdf, Page 249
		
	//Set Phase-Correct PWM mode for OC2A / OC2B 		//ATmega_644.pdf, Page 148
	TCCR2A |= _BV(WGM20);
	//Set prescaling 64, this enable the timer
	TCCR2B |= _BV(CS21);
	
	sei();
}


void Heating_setHeating(boolean on){
	if (on && !heating){
		outputValueIH = initialOutputValue;
	}
	heating = on;
}

int Heating_getLastOnPWM(){
	return lastOutputValueIHRun;
}

boolean Heating_isHeating(){
	return isHeating;
}

void Heating_heatControl(){
	isHeating=digitalRead(isIHOn);
	if (heating){
		if(isHeating)  {
			if(!lastIsHeating){
				lastOutputValueIHStart = outputValueIH;
			}
			lastOutputValueIHRun = outputValueIH;
		} else{
			outputValueIH+=Increment;
			if (outputValueIH>255){
				outputValueIH = 255;
			}
			analogWrite(IHPowerPWM_TIMER, outputValueIH);
			digitalWrite(IHOn,HIGH);
			_delay_ms(20);
			//digitalWrite(IHOn,LOW);
		}
	} else if (isHeating){
		/*
		digitalWrite(IHOff,HIGH);
		_delay_ms(20);
		digitalWrite(IHOff,LOW);
		*/
		outputValueIH = 0;
		analogWrite(IHPowerPWM_TIMER, outputValueIH);
	} else {
		digitalWrite(IHOn,LOW);
	}
	lastIsHeating=isHeating;
	if (isHeating){
		StatusByte |= _BV(SB_isIHOn);
	} else {
		StatusByte&= ~_BV(SB_isIHOn);
	}
}
