#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "mytypes.h"
#include "firmware.h"
#include "input.h"
#include "time.h"
#include "communication.h"
#include "motor.h"
#include "heating.h"
#include "status.h"
#include "DisplayHandler.h"
#include "Charliplexing.h"


#include <stdio.h>

/*
To compile for atmega644p
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o input.o input.c
avr-gcc -mmcu=atmega644p input.o -o input

then
avr-objcopy -O ihex -R .eeprom input input.hex

upload over Raspi GPIO to 644p:
avrdude -F -V -c gpio -p m644p -P gpio -b 57600 -U flash:w:input.hex 
*/ 

#define SPI_MODE_IDLE						0
#define SPI_MODE_GET_STATUS					1
#define SPI_MODE_MOTOR						2
#define SPI_MODE_HEATING					3
#define SPI_MODE_DISPLAY_TEXT				4
#define SPI_MODE_DISPLAY_TEXT_SMALL			5
#define SPI_MODE_DISPLAY_PERCENT			6
#define SPI_MODE_DISPLAY_PERCENT_TEXT		7
#define SPI_MODE_DISPLAY_CLEAR				8
#define SPI_MODE_DISPLAY_PICTURE			9
#define SPI_MODE_VENTIL						10



struct pinInfo PIN_RaspiReset = PA_0; //out 1 to remove power
//struct pinInfo PIN_IHTempSensor = PA_1; //Analog
//struct pinInfo PIN_MotorPosSensor = PA_2; //in		//Status Bit 6
struct pinInfo PIN_Ventil = PA_3; //out
struct pinInfo PIN_SafetyChain = PA_4; //out 1 to power
struct pinInfo PIN_PusherLocked = PA_5; //in		//Status Bit 3
struct pinInfo PIN_LidLocked = PA_6; //in			//Status Bit 2
struct pinInfo PIN_LidClosed = PA_7; //in			//Status Bit 1

//pb0-pb3	display
//struct pinInfo PIN_IHOff = PB_4; //out //deactivated -> not needed
//struct pinInfo PIN_CS = PB_4; //in  //chip select
//pb5-7		mosi miso sclk
//struct pinInfo PIN_MOSI = PB_5; //Master: out  //Slave:in
//struct pinInfo PIN_MISO = PB_6; //Master: in  //Slave:Out
//struct pinInfo PIN_SCK = PB_7; //Master: out  //Slave:in

//pc0-pc7	display
//pd0-pd1	rx/tx

struct pinInfo PIN_SimulateButton0 = PD_2; //out
//struct pinInfo PIN_isIHOn = PD_3; //in				//Status Bit 4
//struct pinInfo PIN_IHOn = PD_4; //out
//struct pinInfo PIN_MotorPWM = PD_5; //pwm	//OC1B
//struct pinInfo PIN_IHPowerPWM = PD_6; //pwm	//OC2B
//struct pinInfo PIN_IHFanPWM = PD_7; //pwm	//OC2A	//Status Bit 5

//uint8_t MotorPWM_TIMER = TIMER1B;
//uint8_t IHPowerPWM_TIMER = TIMER2B;
//uint8_t IHFanPWM_TIMER = TIMER2A;


uint8_t PusherLocked = 0;
uint8_t LidLocked = 0;
uint8_t LidClosed = 0;
uint8_t VentilState = 0;

uint8_t currentMode = 0;
uint8_t lastPercentValue = 0;
boolean updatePercentAgain = false;
uint16_t picture[9];
unsigned long lastTextUpdate = 0;
#define TEXT_UPDATE_TIMEOUT 100

uint16_t picture_hi[9];
uint16_t picture_bye[9];



boolean called __attribute__ ((section (".noinit")));

//if it was a watchdog reset, diesable watchdog in early startup (logic from avr/wdt.h)
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));


boolean wdtRestart __attribute__ ((section (".noinit")));
boolean wdtRestartLast __attribute__ ((section (".noinit")));

int main (void)
{
	//Set Pin Modes
	pinMode(PIN_RaspiReset, OUTPUT);
//	pinMode(PIN_IHTempSensor, INPUT/*_ANALOG*/);
//	pinMode(PIN_MotorPosSensor, INPUT_PULLUP);
	pinMode(PIN_Ventil, OUTPUT);
	pinMode(PIN_SafetyChain, OUTPUT);
	pinMode(PIN_PusherLocked, INPUT_PULLUP);
	pinMode(PIN_LidLocked, INPUT_PULLUP);
	pinMode(PIN_LidClosed, INPUT_PULLUP);
////	pinMode(PIN_SimulateButton0, OUTPUT);
//	pinMode(PIN_IHOff, OUTPUT);
//	pinMode(PIN_isIHOn, INPUT_PULLUP);
//	pinMode(PIN_IHOn, OUTPUT);
//	pinMode(PIN_MotorPWM, OUTPUT/*_PWM*/);
//	pinMode(PIN_IHPowerPWM, OUTPUT/*_PWM*/);
//	pinMode(PIN_IHFanPWM, OUTPUT/*_PWM*/);
	
	Motor_init();
	Heating_init();
		
	//initialice time messurement
	initTime();
	
	//Init LoL-Shield
	LedSign_Init(DOUBLE_BUFFER | GRAYSCALE);  //Initializes the screen
	LedSign_Clear(0);
	LedSign_Flip(false);
	/*
	LedSign_Clear(3);
	LedSign_Horizontal(5,PIXEL_ON);
	LedSign_Vertical(3,PIXEL_ON);
	LedSign_Flip(false);
	*/
	
	//Init SPI
	SPI_init(wdtRestart);
	
	//Disable JTAG interface
	MCUCR = _BV(JTD);
	MCUCR = _BV(JTD); //Need to write twice to disable it (Atmega_644.pdf, page 267, 23.8.1)
	
	
	//Enable Power for Motor / IH
	digitalWrite(PIN_SafetyChain, HIGH);
	//digitalWrite(IHOff, LOW);
	
	//Verify Raspi has Power
	digitalWrite(PIN_RaspiReset, LOW);
	
	
	
	//TODO remove, test was WTD reset
	if (wdtRestart && wdtRestartLast){
		wdtRestart = false;
	}
	wdtRestartLast = wdtRestart;
	digitalWrite(PIN_Ventil, wdtRestart);
	wdtRestart = false;
	
	/*
	picture_hi[0] = 0b00000000000000;
	picture_hi[1] = 0b00110110011000;
	picture_hi[2] = 0b00110110011000;
	picture_hi[3] = 0b00110110000000;
	picture_hi[4] = 0b00111110011000;
	picture_hi[5] = 0b00110110011000;
	picture_hi[6] = 0b00110110011000;
	picture_hi[7] = 0b00110110011000;
	picture_hi[8] = 0b00000000000000;
	
	picture_bye[0] = 0b00000000000000;
	picture_bye[1] = 0b01110010101110;
	picture_bye[2] = 0b01001010101000;
	picture_bye[3] = 0b01001010101000;
	picture_bye[4] = 0b01110001001110;
	picture_bye[5] = 0b01001001001000;
	picture_bye[6] = 0b01001001001000;
	picture_bye[7] = 0b01110001001110;
	picture_bye[8] = 0b00000000000000;

	DisplayHandler_displayProgress(80, false);
	_delay_ms(2000);
	LedSign_Clear(3);
	LedSign_Flip(true);
	_delay_ms(2000);
	
	uint16_t picture2[9]  = {	0b0010101010101010,
								0b0001010101010101,
								0b0010101010101010,
								0b0001010101010101,
								0b0010101010101010,
								0b0001010101010101,
								0b0010101010101010,
								0b0001010101010101,
								0b0010101010101010};
	DisplayHandler_setPicture(&picture2[0]);
	DisplayHandler_DisplayBitMap();
	_delay_ms(2000);
	
	DisplayHandler_setText("EveryCook is starting");
	currentMode = SPI_MODE_DISPLAY_TEXT_SMALL;
	
	
	
        //Hi
			DisplayHandler_setPicture(&picture_hi[0]);
			DisplayHandler_DisplayBitMap();
        
        //Bye
					DisplayHandler_setPicture(&picture_bye[0]);
					DisplayHandler_DisplayBitMap();
        break;
	*/
	
	if (mcusr_mirror & _BV(PORF)){
		//Was power On reset
		DisplayHandler_setText("EveryCook is starting");
		currentMode = SPI_MODE_DISPLAY_TEXT;	
	} else {
		//TODO Remove
		DisplayHandler_setPicture(&picture_hi[0]);
		DisplayHandler_DisplayBitMap();
	}

	LedSign_Clear(3);
	LedSign_Flip(true);
	_delay_ms(100);
	LedSign_Clear(0);
	LedSign_Flip(true);
	
	boolean lastVentil = HIGH;
	//initWatchDog();
	boolean pingFromDaemon;
	while(1){
		wdt_reset();
		/*
		Motor_motorControl();
		Heating_heatControl();
		Heating_controlIHTemp();
		checkLocks();
		*/
		pingFromDaemon=raspiRecived;
		raspiRecived=false;
		
		digitalWrite(PIN_Ventil, lastVentil);
		
		if (availableSPI()){
			if (lastVentil == HIGH){
				lastVentil = LOW;
			} else {
				lastVentil = HIGH;
			}
			//digitalWrite(PIN_Ventil, HIGH);
			//uint8_t data = peakSPI();
			uint8_t data = readSPI(false); //true
			char valueAsString[10];
			sprintf(valueAsString, "%X", data); //HEX
			//sprintf(valueAsString, "%d", data);  //DEC
			DisplayHandler_setText2(valueAsString);
			DisplayHandler_displayText(false);
			//digitalWrite(PIN_Ventil, LOW);
			nextResponse = SPI_CommandOK;
			//_delay_ms(100);
		}
		/*
		if (availableSPI() > 1) {
			uint8_t data;
			uint8_t newMode = readSPI(true);
			char* newText;
			uint8_t readAmount = 0;
			switch(newMode){
				case SPI_MODE_IDLE:
				break;
				case SPI_MODE_GET_STATUS:
					nextResponse = StatusByte;
				break;
				case SPI_MODE_DISPLAY_CLEAR:
					LedSign_Clear(0);
					LedSign_Flip(true);
					currentMode = 0;
					nextResponse = SPI_CommandOK;
				break;
				case SPI_MODE_MOTOR:
					data = readSPI(true);
					wdt_reset();
					Motor_setMotor(data);
					nextResponse = SPI_CommandOK;
				break;
				case SPI_MODE_HEATING:
					data = readSPI(true);
					wdt_reset();
					Heating_setHeating(data);
					nextResponse = SPI_CommandOK;
				break;
				case SPI_MODE_VENTIL:
					VentilState = readSPI(true);
					wdt_reset();
					digitalWrite(PIN_Ventil, VentilState);
					nextResponse = SPI_CommandOK;
				break;

				case SPI_MODE_DISPLAY_PERCENT:
				case SPI_MODE_DISPLAY_PERCENT_TEXT:
					data = readSPI(true);
					wdt_reset();
					DisplayHandler_displayProgress(data, false);
					lastPercentValue = data;
					if (newMode != SPI_MODE_DISPLAY_PERCENT_TEXT){
						nextResponse = SPI_CommandOK;
						currentMode = newMode;
						break;
					}
				case SPI_MODE_DISPLAY_TEXT:
				case SPI_MODE_DISPLAY_TEXT_SMALL:
					DisplayHandler_readText(); //[01]<textlen 1 Byte><text>[00<control byte>]
					if (newMode == SPI_MODE_DISPLAY_TEXT){
						DisplayHandler_displayText(false);
					} else {
						DisplayHandler_displayText(true);
					}
					lastTextUpdate = millis();
					if (newMode == SPI_MODE_DISPLAY_PERCENT_TEXT){
						updatePercentAgain = true;
					}
					nextResponse = SPI_CommandOK;
					currentMode = newMode;
				break;
				
				case SPI_MODE_DISPLAY_PICTURE:
					uint8_t readAmount = 0;
					while (readAmount < 9){
						if (availableSPI() >= 2) {
							data = readSPI(true);
							picture[readAmount] = data << 8;
							data = readSPI(true);
							picture[readAmount] |= data;
							readAmount++;
							wdt_reset();
//						} else {
//							_delay_ms(10);
						}
					}
					DisplayHandler_setPicture(&picture[0]);
					DisplayHandler_DisplayBitMap();
					nextResponse = SPI_CommandOK;
				break;
			}
		} else {
			switch(currentMode){
				case SPI_MODE_IDLE:
				break;
				case SPI_MODE_DISPLAY_PERCENT_TEXT:
					if (updatePercentAgain) {
						DisplayHandler_displayProgress(lastPercentValue, true);
						updatePercentAgain = false;
					}
				case SPI_MODE_DISPLAY_TEXT:
				case SPI_MODE_DISPLAY_TEXT_SMALL:
					if ((millis() - lastTextUpdate) > TEXT_UPDATE_TIMEOUT){
						lastTextUpdate = millis();
						if(!DisplayHandler_displayText(true)){
							currentMode = 0;
						}
					}
				break;
			}
		}
		*/
		
		/*
		if (pingFromDaemon){
			LedSign_Clear(PIXEL_OFF);
			uint8_t i, x, y=5;
			for (i=0;i<8;i++){
				if ((recivedCounter>>i) & 0x01){
					LedSign_Set(x,y,PIXEL_ON);
				}
			}
			y=6;
			for (i=0;i<8;i++){
				if ((recivedData>>i) & 0x01){
					LedSign_Set(x,y,PIXEL_ON);
				}
			}
			LedSign_Flip(false);
		}*/

		//triggerWatchDog(pingFromDaemon);
		
		
//		digitalWrite(PIN_Ventil, digitalRead(PIN_isIHOn));
		//if (StatusByte & _BV(StatusByte)){
		/*
		if (pingFromDaemon){
			digitalWrite(PIN_Ventil, HIGH);
		} else {
			digitalWrite(PIN_Ventil, LOW);
		}*/
	}
	
	return 0;
}

void checkLocks(){
	PusherLocked = digitalRead(PIN_PusherLocked);
	LidLocked = digitalRead(PIN_LidLocked);
	LidClosed = digitalRead(PIN_LidClosed);

	if (PusherLocked){
		StatusByte |= _BV(SB_PusherLocked);
	}else{
		StatusByte &= ~_BV(SB_PusherLocked);
	}

	if (LidLocked){
		StatusByte |= _BV(SB_LidLocked);
	}else{
		StatusByte &= ~_BV(SB_LidLocked);
	}
	if (LidClosed){
		StatusByte |= _BV(SB_LidClosed);
	}else{
		StatusByte &= ~_BV(SB_LidClosed);
	}
}


/*

Ich würde den Watchdog nicht vom Raspi aus direkt triggern. Der Atmel triggert sich selbst um sicher zu sein, dass er noch lebt und gibt dem Raspi mindestens 30s zum reagieren. 
Nach 10s ohne Antwort schalten wir die Heizung ab.
Nach 20s ohne Antwort machen wir einen Daemon reset (über den Button)
Nach daemon reset warten wir ob er wieder hochkommt.
Und erst nachher reseten wir den Raspi auf die harte tour.
*/



void get_mcusr(void) \
  __attribute__((naked)) \
  __attribute__((section(".init3")));
void get_mcusr(void)
{
  mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
  called = true;
  wdtRestart = (mcusr_mirror & _BV(WDRF)) ? true : false;
}


void initWatchDog(){
/*
	//  delay(2000);
	Serial.print("mcusr_mirror:");
	Serial.println(mcusr_mirror,BIN);
	Serial.print("called:"); 
	if (called){
		Serial.println("true"); 
	} else {
		Serial.println("false"); 
	}
*/
	called = false;
	/*
	MCUSR – MCU Status Register
	• Bit 3 – WDRF: Watchdog System Reset Flag
	• Bit 2 – BORF: Brown-out Reset Flag
	• Bit 1 – EXTRF: External Reset Flag
	• Bit 0 – PORF: Power-on Reset Flag
	*/


	//init Watchdog
	// allow changes, disable reset
	WDTCSR = _BV (_WD_CHANGE_BIT) | _BV (WDE);  //_WD_CHANGE_BIT(new) or WDCE
	// set interrupt mode and an interval
	//WDTCSR = _BV (WDIE) | _BV (WDP3) | _BV (WDP0);    // set WDIE(=interrupt mode), and 8 seconds delay
	WDTCSR = _BV (WDIE) | _BV (WDE) | _BV (WDP2) | _BV (WDP1) | _BV (WDP0);    // set WDIE(=interrupt mode), and 2 seconds delay

	//wdt_enable(WDTO_2S); //15MS,30MS,60MS,120MS,250MS,500MS,1S,2S,4S,8S
	wdt_reset();
	sei(); //Start intervals
}

#define DAEMON_RESET_BUTTON_TIME 40000
#define RASPI_RESET_TIME 1000



#define DAEMON_RESET_WAIT_TIME 20000
#define RASPI_RESET_WAIT_TIME 60000


uint16_t resetAtmelTime __attribute__ ((section (".noinit")));
uint16_t resetDaemonTime __attribute__ ((section (".noinit")));
uint16_t resetRaspiTime __attribute__ ((section (".noinit")));


void triggerWatchDog(boolean pingFromDaemon){
	if (pingFromDaemon){
		wdt_reset();
		if (resetDaemonTime != 0) {
			resetAtmelTime = 0;
			resetDaemonTime = 0;
			resetRaspiTime = 0;
			initWatchDog();
		} else if (resetAtmelTime != 0) {
			//if only resetAtmelTime is != 0 WatchDog is still running
			resetAtmelTime = 0;
		}
	} else {
		if (resetDaemonTime != 0) {
			if (resetRaspiTime != 0){
				if (millis() - resetRaspiTime > RASPI_RESET_WAIT_TIME){
					//Raspi should be started now, and pingFromDaemon should come up
					//If i come here the reset dosn't fix the problem, what to do???
					//initWatchDog();
				}
			} else {
				if (millis() - resetDaemonTime > DAEMON_RESET_WAIT_TIME){
					//Next WatchDog Timeout Will reset whole raspi
					initWatchDog();
				}
			}
		}
	}
}

//############################ Watchdog timeout exeded ############################
ISR(WDT_vect){
	cli();
	
	//  Serial.println("ISR(WDT_vect)");  
	
	wdt_reset(); //say wdt all is ok
	mcusr_mirror = MCUSR;
	
	/* Clear WDRF in MCUSR */
	MCUSR &= ~_BV (WDRF);
	/* Write logical one to WDCE and WDE, to enable changes */
	/* Keep old prescaler setting to prevent unintentional time-out */
	WDTCSR |= _BV (_WD_CHANGE_BIT) | _BV (WDE); 
	/* Turn off WDT */
	WDTCSR = 0x00;
	
	
	//--- what to do? ---
	//Disable safety chain
	digitalWrite(PIN_SafetyChain, LOW);
	//stop heating
	//digitalWrite(PIN_IHOff, HIGH);
	//TODO analogWrite(IHPowerPWM_TIMER, 0);
	//stop motor
	//TODO analogWrite(MotorPWM_TIMER, 0);
	
	//try restart atmel -> System reset
//	if (resetAtmelTime == 0) {
		//reenable wdt to do systemreset use following lines of code for it:
		// allow changes, disable reset
		WDTCSR = _BV (_WD_CHANGE_BIT) | _BV (WDE);
		// set reset mode and an interval
		WDTCSR = _BV (WDE);    // set WDE(=restart mode), and delay to default 16ms
		
		resetAtmelTime = millis();
/*		
		sei();
		return;
	}
	*/
	
	/*
	//try restart deamon -> send command/ set pin to high to do so
	if (resetDaemonTime == 0) {
		digitalWrite(PIN_SimulateButton0, HIGH);
		_delay_ms(DAEMON_RESET_BUTTON_TIME);
		digitalWrite(PIN_SimulateButton0, LOW);
		resetDaemonTime = millis();
		sei();
		return;
	}
	
	//if demaon restart don't work, restart raspi
	if (resetRaspiTime == 0) {
		digitalWrite(PIN_RaspiReset, HIGH);
		_delay_ms(RASPI_RESET_TIME);
		digitalWrite(PIN_RaspiReset, LOW);
		resetRaspiTime = millis();
		//resetRaspiTime = 1;
		resetAtmelTime = 0;
		sei();
		return;
	}
	*/
	
	sei();
}
