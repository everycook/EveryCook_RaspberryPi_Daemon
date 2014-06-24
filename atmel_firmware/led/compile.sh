avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o led.o led.c
avr-gcc -mmcu=atmega644p led.o -o led
avr-objcopy -O ihex -R .eeprom led led.hex

