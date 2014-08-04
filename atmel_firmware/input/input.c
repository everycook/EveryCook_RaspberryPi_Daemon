#include <avr/io.h>
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

/*
//set mode
before:
	uint8_t oldSREG = SREG;
	cli();
after:
	SREG = oldSREG;
//otherwise the change of value could do an interrupt

//set outputmode
DDRB |= _BV(port_bit);
//set input mode
DDRB &= ~_BV(port_bit);
PORTB &= ~_BV(port_bit);
//set input pullup
DDRB &= ~_BV(port_bit);
PORTB |= _BV(port_bit);

//set analog input
//Analog input don't need set special, but you can disable die Digital input for it by:
DIDR0 |= _BV(analog_pin);


//set pin on
PORTB |= _BV(port_bit);
//set pin off
PORTB &= ~_BV(port_bit);


//read pin digital
if (PINB & _BV(port_bit)) return HIGH;//true	//if normal input
if (PINB & _BV(port_bit)) return LOW;//false	//if pullup input

//read pin analog
analogRead(analogPin);

*/


//ATmega_644.pdf, Page 248
//AREF is default
#define ANALOG_AREF				0x00
//others: with external capacitor at AREF pin
#define ANALOG_AVCC				0x01
#define ANALOG_INTERNAL_1_1		0x02
#define ANALOG_INTERNAL_2_56	0x03


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



#define TIMER0A 1
#define TIMER0B 2
#define TIMER1A 3
#define TIMER1B 4
#define TIMER2A 5
#define TIMER2B 6

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
	DDRB |= _BV(DDB0);
	
	
	//set pin 3 of PORTD for input_pullup
	DDRD &= ~_BV(DDD3);
	//PORTD &= ~_BV(DDD3);
	PORTD |= _BV(DDD3);
	
	//set PB3/OC0A to output (needed for pwm)
	DDRB |= _BV(DDB3);
	
	uint16_t analog = 0;
	uint8_t outputValue = 0;
	while(1){
		//Control led-pwm by analog input
		analog = analogRead(DDA2);
		
		outputValue = analog >> 2;		//map(analog, 0, 1023, 0, 255);  
		analogWrite(TIMER0A, outputValue);
		
		//Controll led by digital input
		if (PIND & _BV(DDD3)){ //is pullup, so pressed if 0
			// set pin 0 low to turn led off
			PORTB &= ~_BV(PORTB0);
		//	_delay_ms(BLINK_DELAY_LOW);
		} else {
			// set pin 0 high to turn led on
			PORTB |= _BV(PORTB0);
		//	_delay_ms(BLINK_DELAY_HIGH);
		}
	}
 
	return 0;
}
