
/*note the two important libraries are at:
 /usr/lib/avr/include/avr/iom328p.h
 /usr/lib/avr/include/avr/sfr_defs.h
To compile:
avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o echo.o echo.c
avr-gcc -mmcu=atmega328p echo.o -o echo
avr-objcopy -O ihex -R .eeprom echo echo.hex

upload
avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyUSB0 -b 57600 -U flash:w:echo.hex 
all taken from:
http://www.avrfreaks.net/?name=PNphpBB2&file=viewtopic&t=48188%5B

http://brittonkerin.com/cduino/lessons.html#usart-out

*/ 

#include <avr/io.h> 

#define USART_BAUDRATE 9600 
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1) 

void serial_init() {
 
	power_usart0_enable();
 
	// configure ports double mode
	UCSR0A = _BV(U2X0);
 
	// configure the ports speed
	UBRR0H = 0x00;
	UBRR0L = 34;
 
	// asynchronous, 8N1 mode
	UCSR0C |= 0x06;
	
	// rx/tx enable
	UCSR0B |= _BV(RXEN0);
	UCSR0B |= _BV(TXEN0);
}


unsigned char serial_poll_recv(unsigned char *a_data, unsigned int a_size) {
	unsigned int cnt = a_size;
 
	while (a_size) {
		/* Wait for data to be received */
		while ( !(UCSR0A & (1<<RXC0)) );
 
		/* Get and return received data from buffer */
		*a_data = UDR0;
		a_data++;
		a_size--;
	}
 
	return cnt;
}
 
 
unsigned int serial_poll_send(void *data, unsigned int a_size) {
	unsigned int i = 0x00;
	while (i < a_size) {
		// wait until usart buffer is empty
		while ( bit_is_clear(UCSR0A, UDRE0) );
 
		// send the data
		UDR0 = ((unsigned char *)data)[i++];
	}
 
	return i;
}




int main (void) 
{ 
   char ReceivedByte; 

   UCSR0B = (1 << RXEN) | (1 << TXEN);   // Turn on the transmission and reception circuitry 
   UCSR0C = (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1); // Use 8-bit character sizes 

   UBRR0H = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register 
   UBRR0L = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register 

   for (;;) // Loop forever 
   { 
      while ((UCSRA & (1 << RXC)) == 0) {}; // Do nothing until data have been received and is ready to be read from UDR 
      ReceivedByte = UDR; // Fetch the received byte value into the variable "ByteReceived" 

      while ((UCSRA & (1 << UDRE)) == 0) {}; // Do nothing until UDR is ready for more data to be written to it 
      UDR = ReceivedByte; // Echo back the received byte back to the computer 
   }    
}


