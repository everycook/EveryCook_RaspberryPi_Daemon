#include <avr/io.h>
#include <util/delay.h>

/*note the two important libraries are at:
 /usr/lib/avr/include/avr/iom328p.h
 /usr/lib/avr/include/avr/sfr_defs.h
To compile for arduino:
avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o led.o led.c
avr-gcc -mmcu=atmega328p led.o -o led

for atmega644p
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o led.o led.c
avr-gcc -mmcu=atmega644p led.o -o led

then
avr-objcopy -O ihex -R .eeprom led led.hex

upload to arduino
avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyUSB0 -b 57600 -U flash:w:led.hex 

upload over Raspi GPIO to 644p:
avrdude -F -V -c gpio -p m644p -P gpio -b 57600 -U flash:w:led.hex 
all taken from:
http://balau82.wordpress.com/2011/03/29/programming-arduino-uno-in-pure-c/

fuse calculator:
http://www.engbedded.com/fusecalc/
setting used:
Ext. Crystal 8.0MHz startup 258 CK + 4.1 ms. CKSEL=1110 SUT=00

avrdude -P gpio -c gpio -p m644p -U lfuse:w:0xce:m -U hfuse:w:0x99:m -U efuse:w:0xff:m
these fuse settings seem to give a correct freq.

and check ~/.avrdude  settings
*/ 

enum {
 BLINK_DELAY_HIGH = 2000,
 BLINK_DELAY_LOW = 1000,
};

 
int main (void)
{
 /* set pin 5 of PORTB for output*/
 DDRB |= _BV(DDB0);
 //DDRB |= _BV(DDB5);
 
 while(1) {
  /* set pin 5 high to turn led on */
  PORTB |= _BV(PORTB0);
  //PORTB |= _BV(PORTB5);
  _delay_ms(BLINK_DELAY_HIGH);
 
  /* set pin 5 low to turn led off */
  PORTB &= ~_BV(PORTB0);
  //PORTB &= ~_BV(PORTB5);
  _delay_ms(BLINK_DELAY_LOW);
 }
 
 return 0;
}
