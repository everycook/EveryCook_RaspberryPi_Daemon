#include "input.h"

#include <avr/interrupt.h>

volatile uint8_t * port_to_mode[] = {
	&DDRA,
	&DDRB,
	&DDRC,
	&DDRD,
};

volatile uint8_t * port_to_output[] = {
	&PORTA,
	&PORTB,
	&PORTC,
	&PORTD,
};

volatile uint8_t * port_to_input[] = {
	&PINA,
	&PINB,
	&PINC,
	&PIND,
};

uint8_t currentPinMode[4][8] = {
	{INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT},
	{INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT},
	{INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT},
	{INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT, INPUT}
};

#define currentMode(port, port_pin)	currentPinMode[port][port_pin]


void pinMode(struct pinInfo pin, uint8_t mode){
	uint8_t bit = _BV(pin.port_pin);
	volatile uint8_t *reg, *out;
	
	reg = port_to_mode[pin.port];
	out = port_to_output[pin.port];
	
	uint8_t oldSREG = SREG;
	cli();
	if (mode == INPUT) {
		*reg &= ~bit;
		*out &= ~bit;
	} else if (mode == INPUT_PULLUP) {
		*reg &= ~bit;
		*out |= bit;
	#ifdef SPECIAL_PIN_MODE
	} else if (mode == INPUT_ANALOG) {
		*reg &= ~bit;
		*out &= ~bit;
		if (pin.port == PA){
			DIDR0 |= bit;
		}
	#endif
	} else { //OUTPUT or OUTPUT_PWM
		*reg |= bit;
		#ifdef SPECIAL_PIN_MODE
		if (pin.port == PA){
			DIDR0 &= ~bit;
		}
		#endif
	}
	SREG = oldSREG;
	currentPinMode[pin.port][pin.port_pin] = mode;
}

void digitalWrite(struct pinInfo pin, uint8_t val){
	uint8_t bit = _BV(pin.port_pin);
	volatile uint8_t *out;
	out = port_to_output[pin.port];
	
	uint8_t oldSREG = SREG;
	cli();
	
	if (val == LOW) {
		*out &= ~bit;
	} else {
		*out |= bit;
	}
	
	SREG = oldSREG;
}

uint8_t digitalReadSimple(struct pinInfo pin) {
	uint8_t bit = _BV(pin.port_pin);
	
	if (*port_to_input[pin.port] & bit) return HIGH;
	return LOW;
}

uint8_t digitalRead(struct pinInfo pin) {
	uint8_t bit = _BV(pin.port_pin);
	
	if (currentPinMode[pin.port][pin.port_pin] == INPUT_PULLUP){
		if (*port_to_input[pin.port] & bit) return LOW;
		return HIGH;
	} else {
		if (*port_to_input[pin.port] & bit) return HIGH;
		return LOW;
	}
}




//from wiring_analog.c from arduino library
uint16_t analogRead(struct pinInfo pin) {
	if (pin.port != PA){
		return 0;
	}
	uint8_t low, high;
	// set the analog reference (high two bits of ADMUX) and select the
	// channel (low 4 bits).  this also sets ADLAR (left-adjust result)
	// to 0 (the default).
	ADMUX = (ANALOG_AREF << 6) | (pin.port_pin & 0x07);
	
	// start the conversion
	//sbi(ADCSRA, ADSC);
	ADCSRA |= _BV(ADSC);		//ATmega_644.pdf, Page 250

	// ADSC is cleared when the conversion finishes
	while (bit_is_set(ADCSRA, ADSC));		//ATmega_644.pdf, Page 250

	// we have to read ADCL first; doing so locks both ADCL
	// and ADCH until ADCH is read.  reading ADCL second would
	// cause the results of each conversion to be discarded,
	// as ADCL and ADCH would be locked when it completed.
	low  = ADCL;
	high = ADCH;
	
	return (high << 8) | low;
}


//from wiring_analog.c from arduino library
void analogWrite(uint8_t timer, int val)
{
	switch(timer)
	{
		#if defined(TCCR0A) && defined(COM0A1)
		case TIMER0A:
			// connect pwm to pin on timer 0, channel A
			//sbi(TCCR0A, COM0A1);
			TCCR0A |= _BV(COM0A1);
			OCR0A = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR0A) && defined(COM0B1)
		case TIMER0B:
			// connect pwm to pin on timer 0, channel B
			//sbi(TCCR0A, COM0B1);
			TCCR0A |= _BV(COM0B1);
			OCR0B = val; // set pwm duty
			break;
		#endif

		
		#if defined(TCCR1A) && defined(COM1A1)
		case TIMER1A:
			// connect pwm to pin on timer 1, channel A
			//sbi(TCCR1A, COM1A1);
			TCCR1A |= _BV(COM1A1);
			OCR1A = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR1A) && defined(COM1B1)
		case TIMER1B:
			// connect pwm to pin on timer 1, channel B
			//sbi(TCCR1A, COM1B1);
			TCCR1A |= _BV(COM1B1);
			OCR1B = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR2A) && defined(COM2A1)
		case TIMER2A:
			// connect pwm to pin on timer 2, channel A
			//sbi(TCCR2A, COM2A1);
			TCCR2A |= _BV(COM2A1);
			OCR2A = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR2A) && defined(COM2B1)
		case TIMER2B:
			// connect pwm to pin on timer 2, channel B
			//sbi(TCCR2A, COM2B1);
			TCCR2A |= _BV(COM2B1);
			OCR2B = val; // set pwm duty
			break;
		#endif
	}
}