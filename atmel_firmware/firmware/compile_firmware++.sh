rm firmware.o
rm firmware
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o firmware.o firmware.c
avr-g++ -mmcu=atmega644p *.o -o firmware
avr-objcopy -O ihex -R .eeprom firmware firmware.hex

