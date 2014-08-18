rm *.o
rm firmware
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o input.o input.c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o time.o time.c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o Charliplexing.o Charliplexing.c
#avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o spihandling.o spihandling.c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o heating.o heating.c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o motor.o motor.c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o FontHandler.o FontHandler.c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o NormalFont.o NormalFont.c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o SmallFont.o SmallFont.c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o DisplayHandler.o DisplayHandler.c
#avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o .o .c
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o firmware.o firmware.c
#avr-gcc -mmcu=atmega644p firmware.o input.o time.o Charliplexing.o -o firmware
#avr-gcc -mmcu=atmega644p firmware.o input.o time.o Charliplexing.o spihandling.o -o firmware
avr-gcc -mmcu=atmega644p *.o -o firmware
avr-objcopy -O ihex -R .eeprom firmware firmware.hex

