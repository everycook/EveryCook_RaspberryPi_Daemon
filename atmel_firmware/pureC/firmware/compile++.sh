rm *.o
rm firmware
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o input.o input.c
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o status.o status.c
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o time.o time.c
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o Charliplexing.o Charliplexing.c
#avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o spihandling.o spihandling.c
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o heating.o heating.c
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o motor.o motor.c
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o FontHandler.o FontHandler.c
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o NormalFont.o NormalFont.c
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o SmallFont.o SmallFont.c
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o DisplayHandler.o DisplayHandler.c
#avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o .o .c
avr-g++ -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o firmware.o firmware.c
#avr-g++ -mmcu=atmega644p firmware.o input.o time.o Charliplexing.o -o firmware
#avr-g++ -mmcu=atmega644p firmware.o input.o time.o Charliplexing.o spihandling.o -o firmware
avr-g++ -mmcu=atmega644p *.o -o firmware
avr-objcopy -O ihex -R .eeprom firmware firmware.hex

