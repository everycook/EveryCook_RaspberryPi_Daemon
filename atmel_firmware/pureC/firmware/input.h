#ifndef input_h
#define input_h

#include <avr/io.h>


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


extern volatile uint8_t * port_to_mode[];
extern volatile uint8_t * port_to_output[];
extern volatile uint8_t * port_to_input[];


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

extern uint8_t currentPinMode[4][8];

#define currentMode(port, port_pin)	currentPinMode[port][port_pin]

void pinMode(struct pinInfo pin, uint8_t mode);
void digitalWrite(struct pinInfo pin, uint8_t val);
uint8_t digitalReadSimple(struct pinInfo pin);
uint8_t digitalRead(struct pinInfo pin);
uint16_t analogRead(struct pinInfo pin);
void analogWrite(uint8_t timer, int val);

#endif