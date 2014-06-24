avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o input.o input.c
avr-gcc -mmcu=atmega644p input.o -o input
avr-objcopy -O ihex -R .eeprom input input.hex

