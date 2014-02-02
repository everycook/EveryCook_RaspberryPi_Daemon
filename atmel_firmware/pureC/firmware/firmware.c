#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "mytypes.h"
#include "firmware.h"
#include "input.h"
#include "time.h"
#include "motor.h"
#include "heating.h"
#include "DisplayHandler.h"
#include "Charliplexing.h"

/*
To compile for atmega644p
avr-gcc -Os -DF_CPU=8000000UL -mmcu=atmega644p -c -o input.o input.c
avr-gcc -mmcu=atmega644p input.o -o input

then
avr-objcopy -O ihex -R .eeprom input input.hex

upload over Raspi GPIO to 644p:
avrdude -F -V -c gpio -p m644p -P gpio -b 57600 -U flash:w:input.hex 
*/ 




struct pinInfo RaspiReset = PA_0; //out 1 to remove power
//struct pinInfo IHTempSensor = PA_1; //Analog
//struct pinInfo MotorPosSensor = PA_2; //in		//Status Bit 6
struct pinInfo Ventil = PA_3; //out
struct pinInfo SafetyChain = PA_4; //out 1 to power
struct pinInfo PusherLocked = PA_5; //in		//Status Bit 3
struct pinInfo LidLocked = PA_6; //in			//Status Bit 2
struct pinInfo LidClosed = PA_7; //in			//Status Bit 1

//pb0-pb3	display
//struct pinInfo IHOff = PB_4; //out //deactivated -> not needed
//struct pinInfo CS = PB_4; //in  //chip select
//pb5-7		mosi miso sclk
/*struct pinInfo MOSI = PB_5; //out
struct pinInfo MISO = PB_6; //in
struct pinInfo SCK = PB_7; //out
*/
//pc0-pc7	display
//pd0-pd1	rx/tx

struct pinInfo SimulateButton0 = PD_2; //out
//struct pinInfo isIHOn = PD_3; //in				//Status Bit 4
//struct pinInfo IHOn = PD_4; //out
//struct pinInfo MotorPWM = PD_5; //pwm	//OC1B
//struct pinInfo IHPowerPWM = PD_6; //pwm	//OC2B
//struct pinInfo IHFanPWM = PD_7; //pwm	//OC2A	//Status Bit 5

//uint8_t MotorPWM_TIMER = TIMER1B;
//uint8_t IHPowerPWM_TIMER = TIMER2B;
//uint8_t IHFanPWM_TIMER = TIMER2A;


//Status Byte Bit possition
#define SB_CommandOK		0
#define SB_LidClosed		1
#define SB_LidLocked		2
#define SB_PusherLocked		3
#define SB_isIHOn			4
#define SB_IHFanPWM			5
#define SB_MotorPosSensor	6
#define SB_CommandError		7

#define SPI_CommandOK 0x01;

uint8_t StatusByte = SPI_CommandOK;

boolean wdtRestart __attribute__ ((section (".noinit")));

int main (void)
{
	//Set Pin Modes
	pinMode(RaspiReset, OUTPUT);
//	pinMode(IHTempSensor, INPUT/*_ANALOG*/);
//	pinMode(MotorPosSensor, INPUT_PULLUP);
	pinMode(Ventil, OUTPUT);
	pinMode(SafetyChain, OUTPUT);
	pinMode(PusherLocked, INPUT_PULLUP);
	pinMode(LidLocked, INPUT_PULLUP);
	pinMode(LidClosed, INPUT_PULLUP);
	pinMode(SimulateButton0, OUTPUT);
//	pinMode(IHOff, OUTPUT);
//	pinMode(isIHOn, INPUT_PULLUP);
//	pinMode(IHOn, OUTPUT);
//	pinMode(MotorPWM, OUTPUT/*_PWM*/);
//	pinMode(IHPowerPWM, OUTPUT/*_PWM*/);
//	pinMode(IHFanPWM, OUTPUT/*_PWM*/);

	Motor_init();
	Heating_init();
		
	//initialice time messurement
	initTime();
	
	
	//Enable Power for Motor / IH
	digitalWrite(SafetyChain, HIGH);
	//digitalWrite(IHOff, LOW);
	
	//Verify RaspiHas has Power
	digitalWrite(RaspiReset, LOW);
	
	
	//Init LoL-Shield
	LedSign_Init(DOUBLE_BUFFER | GRAYSCALE);  //Initializes the screen
	LedSign_Clear(3);
	LedSign_Horizontal(5,PIXEL_ON);
	LedSign_Vertical(3,PIXEL_ON);
	LedSign_Flip(false);
	
	//TODO remove, test was WTD reset
	digitalWrite(Ventil, wdtRestart);
	_delay_ms(1000);
	wdtRestart = false;
	digitalWrite(Ventil, wdtRestart);
	
	//init SPI as Master
	//initSPI();
	
	//initWatchDog();
	
	uint16_t ihTemp = 0;
	uint8_t ihTemp8bit;
	boolean pingFromDaemon = false;
	while(1){
		Motor_motorControl();
		Heating_heatControl();
		Heating_controlIHTemp();




		//TODO read input
		//triggerWatchDog(pingFromDaemon);
		
		
//		digitalWrite(Ventil, digitalRead(isIHOn));
		//SPI_MasterTransmit(0xAB);
	}
	
	return 0;
}

#define SPI_MODE_GET_STATUS					1
#define SPI_MODE_MOTOR						2
#define SPI_MODE_HEATING					3
#define SPI_MODE_DISPLAY_TEXT				4
#define SPI_MODE_DISPLAY_TEXT_SMALL			5
#define SPI_MODE_DISPLAY_PERCENT			6
#define SPI_MODE_DISPLAY_PERCENT_TEXT		7
#define SPI_MODE_DISPLAY_CLEAR				8
#define SPI_MODE_DISPLAY_PICTURE			9

uint8_t currentMode = 0;
ISR(SPI_STC_vect){
	uint8_t data = SPDR;
	uint8_t response = 0xFF;
	switch(currentMode){
		case 0: //no mode check new mode
			switch(data){
				case SPI_MODE_GET_STATUS:
					response = StatusByte;
				break;
				case SPI_MODE_DISPLAY_CLEAR:
					LedSign_Clear(0);
					LedSign_Flip(true);
					response = SPI_CommandOK;
				break;
				case SPI_MODE_MOTOR:
				case SPI_MODE_HEATING:
				case SPI_MODE_DISPLAY_TEXT:
				case SPI_MODE_DISPLAY_TEXT_SMALL:
				case SPI_MODE_DISPLAY_PERCENT:
				case SPI_MODE_DISPLAY_PERCENT_TEXT:
				case SPI_MODE_DISPLAY_PICTURE:
					currentMode = data;
					response = SPI_CommandOK;
				break;
			}
		break;
		
		case SPI_MODE_GET_STATUS:
			response = StatusByte;
		break;
		case SPI_MODE_MOTOR:
			Motor_setMotor(data);
			currentMode = 0;
			response = SPI_CommandOK;
		break;
		case SPI_MODE_HEATING:
			Heating_setHeating(data);
			currentMode = 0;
			response = SPI_CommandOK;
		break;
		case SPI_MODE_DISPLAY_TEXT:
		case SPI_MODE_DISPLAY_TEXT_SMALL:
			//response = Display_addText(data);
			if (response != 0xFF){
				currentMode = 0;
			}
		break;
	}
	SPDR = response;
}

/*

Ich würde den Watchdog nicht vom Raspi aus direkt triggern. Der Atmel triggert sich selbst um sicher zu sein, dass er noch lebt und gibt dem Raspi mindestens 30s zum reagieren. 
Nach 10s ohne Antwort schalten wir die Heizung ab.
Nach 20s ohne Antwort machen wir einen Daemon reset (über den Button)
Nach daemon reset warten wir ob er wieder hochkommt.
Und erst nachher reseten wir den Raspi auf die harte tour.
*/



boolean called __attribute__ ((section (".noinit")));

//if it was a watchdog reset, diesable watchdog in early startup (logic from avr/wdt.h)
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

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

//Watchdog timeout exeded
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
	digitalWrite(SafetyChain, LOW);
	//stop heating
	//digitalWrite(IHOff, HIGH);
	//TODO analogWrite(IHPowerPWM_TIMER, 0);
	//stop motor
	//TODO analogWrite(MotorPWM_TIMER, 0);
	
	//try restart atmel -> System reset
	if (resetAtmelTime == 0) {
		//reenable wdt to do systemreset use following lines of code for it:
		// allow changes, disable reset
		WDTCSR = _BV (_WD_CHANGE_BIT) | _BV (WDE);
		// set reset mode and an interval
		WDTCSR = _BV (WDE);    // set WDE(=restart mode), and delay to default 16ms
		
		resetAtmelTime = millis();
		sei();
		return;
	}
	
	//try restart deamon -> send command/ set pin to high to do so
	if (resetDaemonTime == 0) {
		digitalWrite(SimulateButton0, HIGH);
		_delay_ms(DAEMON_RESET_BUTTON_TIME);
		digitalWrite(SimulateButton0, LOW);
		resetDaemonTime = millis();
		sei();
		return;
	}
	
	//if demaon restart don't work, restart raspi
	if (resetRaspiTime == 0) {
		digitalWrite(RaspiReset, HIGH);
		_delay_ms(RASPI_RESET_TIME);
		digitalWrite(RaspiReset, LOW);
		resetRaspiTime = millis();
		//resetRaspiTime = 1;
		resetAtmelTime = 0;
		sei();
		return;
	}
	
	sei();
}
