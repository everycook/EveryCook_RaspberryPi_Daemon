#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/*
To compile for atmega644p
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o input.o input.c
avr-gcc -mmcu=atmega644p input.o -o input

then
avr-objcopy -O ihex -R .eeprom input input.hex

upload over Raspi GPIO to 644p:
avrdude -F -V -c gpio -p m644p -P gpio -b 57600 -U flash:w:input.hex 
*/ 


#define HIGH 0x1
#define LOW  0x0

#undef SPECIAL_PIN_MODE

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2
#ifdef SPECIAL_PIN_MODE
	#define INPUT_ANALOG 0x3
	#define OUTPUT_PWM 0x4
#endif

#define PA 0
#define PB 1
#define PC 2
#define PD 3



//ATmega_644.pdf, Page 248
//AREF is default
#define ANALOG_AREF				0x00
//others: with external capacitor at AREF pin
#define ANALOG_AVCC				0x01
#define ANALOG_INTERNAL_1_1		0x02
#define ANALOG_INTERNAL_2_56	0x03



#define TIMER0A 1
#define TIMER0B 2
#define TIMER1A 3
#define TIMER1B 4
#define TIMER2A 5
#define TIMER2B 6


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


typedef struct pinInfo {
    uint8_t port;
    uint8_t port_pin;
};

#define PIN(port, port_pin)	{ port, port_pin }


#define PA_0 PIN(PA, 0);
#define PA_1 PIN(PA, 1);
#define PA_2 PIN(PA, 2);
#define PA_3 PIN(PA, 3);
#define PA_4 PIN(PA, 4);
#define PA_5 PIN(PA, 5);
#define PA_6 PIN(PA, 6);
#define PA_7 PIN(PA, 7);

#define PB_0 PIN(PB, 0);
#define PB_1 PIN(PB, 1);
#define PB_2 PIN(PB, 2);
#define PB_3 PIN(PB, 3);
#define PB_4 PIN(PB, 4);
#define PB_5 PIN(PB, 5);
#define PB_6 PIN(PB, 6);
#define PB_7 PIN(PB, 7);

#define PC_0 PIN(PC, 0);
#define PC_1 PIN(PC, 1);
#define PC_2 PIN(PC, 2);
#define PC_3 PIN(PC, 3);
#define PC_4 PIN(PC, 4);
#define PC_5 PIN(PC, 5);
#define PC_6 PIN(PC, 6);
#define PC_7 PIN(PC, 7);

#define PD_0 PIN(PD, 0);
#define PD_1 PIN(PD, 1);
#define PD_2 PIN(PD, 2);
#define PD_3 PIN(PD, 3);
#define PD_4 PIN(PD, 4);
#define PD_5 PIN(PD, 5);
#define PD_6 PIN(PD, 6);
#define PD_7 PIN(PD, 7);



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
uint16_t analogRead(uint8_t analogPin) {
	uint8_t low, high;
	// set the analog reference (high two bits of ADMUX) and select the
	// channel (low 4 bits).  this also sets ADLAR (left-adjust result)
	// to 0 (the default).
	ADMUX = (ANALOG_AREF << 6) | (analogPin & 0x07);
	
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
	

struct pinInfo ButtonLed = PB_0;
struct pinInfo AnalogLed = PB_3;

struct pinInfo ButtonIn = PD_3;
struct pinInfo AnalogIn = PA_2;

 
int main (void)
{
	//Enable ADC
	ADCSRA |= _BV(ADEN);		//ATmega_644.pdf, Page 249
	
	
	//Set Phase-Correct PWM mode for OC0A / OC0B 		//ATmega_644.pdf, Page 99
	TCCR0A |= _BV(WGM00);
	//Set No prescaling, this enable the timer
	TCCR0B |= _BV(CS00);
	
	//Set Phase-Correct PWM mode for OC1A / OC1B 8bit mode 		//ATmega_644.pdf, Page 127
	TCCR1A |= _BV(WGM10);
	//Set No prescaling, this enable the timer
	TCCR1B |= _BV(CS10);
	
	//Set Phase-Correct PWM mode for OC2A / OC2B 		//ATmega_644.pdf, Page 148
	TCCR2A |= _BV(WGM20);
	//Set No prescaling, this enable the timer
	TCCR2B |= _BV(CS20);
	
	
	
	// set pin 0 of PORTB for output
	//DDRB |= _BV(DDB0);
	pinMode(ButtonLed, OUTPUT);
	
	
	//set pin 3 of PORTD for input_pullup
	//DDRD &= ~_BV(DDD3);
	//PORTD |= _BV(PD3);
	pinMode(ButtonIn, INPUT_PULLUP);
	
	//set PB3/OC0A to output (needed for pwm)
	//DDRB |= _BV(DDB3);
	pinMode(AnalogLed, OUTPUT);
	
	//Not needed because its analog read
	//pinMode(AnalogIn, INPUT);
	
	uint16_t analog = 0;
	uint8_t outputValue = 0;
	while(1){
		//Control led-pwm by analog input
		analog = analogRead(DDA2);
		
		outputValue = analog >> 2;		//map(analog, 0, 1023, 0, 255);  
		analogWrite(TIMER0A, outputValue);
		
		/*
		//Controll led by digital input
		if (PIND & _BV(PIND3)){ //is pullup, so pressed if 0
			// set pin 0 low to turn led off
			PORTB &= ~_BV(PB0);
		} else {
			// set pin 0 high to turn led on
			PORTB |= _BV(PB0);
		}
		*/
		digitalWrite(ButtonLed, digitalRead(ButtonIn));
	}
 
	return 0;
}
