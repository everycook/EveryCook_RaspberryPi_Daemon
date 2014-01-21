avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o input.o input.c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o time.o time.c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o firmware.o firmware.c
avr-gcc -mmcu=atmega644p firmware.o input.o time.o -o firmware
avr-objcopy -O ihex -R .eeprom firmware firmware.hex

