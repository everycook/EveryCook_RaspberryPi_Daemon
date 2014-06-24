#include "time.h"

#include <avr/interrupt.h>

//main idea from wiring.c from arduino

#define TIMER1_PRESCALE_OFF		0b00000000
#define TIMER1_PRESCALE_1		0b00000001
#define TIMER1_PRESCALE_8		0b00000010
#define TIMER1_PRESCALE_64		0b00000011
#define TIMER1_PRESCALE_256		0b00000100
#define TIMER1_PRESCALE_1024	0b00000101


// the prescaler is set so that timer1 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks. (-> max value of 8 bit is reaced)
unsigned long MICROSECONDS_PER_TIMER1_OVERFLOW = ((64 * 256) / ( F_CPU / 1000000L ) );

// the whole number of milliseconds per timer1 overflow
//unsigned long MILLIS_INC = (MICROSECONDS_PER_TIMER1_OVERFLOW / 1000);
unsigned long MILLIS_INC = (((64 * 256) / ( F_CPU / 1000000L ) ) / 1000);

// the fractional number of milliseconds per timer1 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
//unsigned long FRACT_INC = ((MICROSECONDS_PER_TIMER1_OVERFLOW % 1000) >> 3);
unsigned long FRACT_INC = ((((64 * 256) / ( F_CPU / 1000000L ) ) % 1000) >> 3);
unsigned char FRACT_MAX  = (1000 >> 3);

void initTime(){
	cli();
	
	unsigned int prescaler = 64;
	// get timer 1 prescale factor
	switch (TCCR1B & 0x07){
		case TIMER1_PRESCALE_1:
			prescaler = 1;
			break;
		case TIMER1_PRESCALE_8:
			prescaler = 8;
			break;
		case TIMER1_PRESCALE_64:
			prescaler = 64;
			break;
		case TIMER1_PRESCALE_256:
			prescaler = 256;
			break;
		case TIMER1_PRESCALE_1024:
			prescaler = 1024;
			break;
			
		//case  TIMER1_PRESCALE_OFF:
		default:
			//Set prescale to 64
			TCCR1B |= _BV(CS11) | _BV(CS10);
			break;
	}
	
	//Recalculate values
	MICROSECONDS_PER_TIMER1_OVERFLOW = ((prescaler * 256) / ( F_CPU / 1000000L ) );
	MILLIS_INC = (MICROSECONDS_PER_TIMER1_OVERFLOW / 1000);
	FRACT_INC = ((MICROSECONDS_PER_TIMER1_OVERFLOW % 1000) >> 3);
	
	// enable timer 1 overflow interrupt
	TIMSK1 |= _BV(TOIE1);
	
	sei();
}

volatile unsigned long timer1_millis = 0;
static unsigned char timer1_fract = 0;

ISR(TIMER1_OVF_vect) {
	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	unsigned long m = timer1_millis;
	unsigned char f = timer1_fract;

	m += MILLIS_INC;
	f += FRACT_INC;
	if (TCCR1A & _BV(WGM10)){
		//using phase-correct PWM would mean that timer 1 overflowed half as often
		//fix this by increase it twice
		f += FRACT_INC;
	}
	if (f >= FRACT_MAX) {
		f -= FRACT_MAX;
		m += 1;
	}

	timer1_fract = f;
	timer1_millis = m;
}

unsigned long millis() {
	unsigned long m;
	uint8_t oldSREG = SREG;
	
	cli();
	m = timer1_millis;
	SREG = oldSREG;

	return m;
}
