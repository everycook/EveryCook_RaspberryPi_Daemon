#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "mytypes.h"
#include "firmware.h"
#include "input.h"
#include "time.h"
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
struct pinInfo IHTempSensor = PA_1; //Analog
struct pinInfo MotorPosSensor = PA_2; //in
struct pinInfo Ventil = PA_3; //out
struct pinInfo SafetyChain = PA_4; //out 1 to power
struct pinInfo PusherLocked = PA_5; //in
struct pinInfo LidLocked = PA_6; //in
struct pinInfo LidClosed = PA_7; //in

//pb0-pb3	display
struct pinInfo Display_8 = PB_0;
struct pinInfo Display_9 = PB_1;
struct pinInfo Display_10 = PB_2;
struct pinInfo Display_11 = PB_3;
struct pinInfo SimulateButton0 = PB_4; //out
//pb5-7		mosi miso clk


//pc0-pc7	display
struct pinInfo Display_0 = PC_0;
struct pinInfo Display_1 = PC_1;
struct pinInfo Display_2 = PC_2;
struct pinInfo Display_3 = PC_3;
struct pinInfo Display_4 = PC_4;
struct pinInfo Display_5 = PC_5;
struct pinInfo Display_6 = PC_6;
struct pinInfo Display_7 = PC_7;

struct pinInfo DisplayX = PC_0;

//pd0-pd1	rx/tx

struct pinInfo IHOff = PD_2; //out
struct pinInfo isIHOn = PD_3; //in
struct pinInfo IHOn = PD_4; //out
struct pinInfo MotorPWM = PD_5; //pwm	//OC1B
struct pinInfo IHPowerPWM = PD_6; //pwm	//OC2B
struct pinInfo IHFanPWM = PD_7; //pwm	//OC2A

uint8_t MotorPWM_TIMER = TIMER1B;
uint8_t IHPowerPWM_TIMER = TIMER2B;
uint8_t IHFanPWM_TIMER = TIMER2A;

uint8_t lastIHFanPWM = 0;

uint16_t ihTemp = 0;
uint8_t ihTemp8bit;



#define PIXEL_ON 7
#define PIXEL_HALF 2
#define PIXEL_OFF 0

void controlIHTemp(){
	//controll IHTemp
	ihTemp = analogRead(IHTempSensor);
	ihTemp8bit = ihTemp >> 2;
	uint8_t IHFanPWM = 0;
	if (ihTemp8bit > 200){
		IHFanPWM = 255;
	} else if (ihTemp8bit > 150){
		IHFanPWM = 200;
	} else if (ihTemp8bit > 100){
		IHFanPWM = 150;
	} else if (ihTemp8bit > 64){
		IHFanPWM = 100;
	}
	if (lastIHFanPWM != IHFanPWM){
		analogWrite(IHFanPWM_TIMER, IHFanPWM);
		lastIHFanPWM = IHFanPWM;
	}
}

boolean wdtRestart __attribute__ ((section (".noinit")));

int main (void)
{
	//Set Pin Modes
	pinMode(RaspiReset, OUTPUT);
	pinMode(IHTempSensor, INPUT/*_ANALOG*/);
	pinMode(MotorPosSensor, INPUT_PULLUP);
	pinMode(Ventil, OUTPUT);
	pinMode(SafetyChain, OUTPUT);
	pinMode(PusherLocked, INPUT_PULLUP);
	pinMode(LidLocked, INPUT_PULLUP);
	pinMode(LidClosed, INPUT_PULLUP);
	pinMode(SimulateButton0, OUTPUT);
	pinMode(IHOff, OUTPUT);
	pinMode(isIHOn, INPUT_PULLUP);
	pinMode(IHOn, OUTPUT);
	pinMode(MotorPWM, OUTPUT/*_PWM*/);
	pinMode(IHPowerPWM, OUTPUT/*_PWM*/);
	pinMode(IHFanPWM, OUTPUT/*_PWM*/);
	
	
	cli();
	//Enable ADC (for IHTempSensor)
	ADCSRA |= _BV(ADEN);		//ATmega_644.pdf, Page 249
	
	
	//OC0A / OC0B is used for LoL-Shield and will be initialized there
	
	//Set Phase-Correct PWM mode for OC1A / OC1B 8bit mode 		//ATmega_644.pdf, Page 127
	TCCR1A |= _BV(WGM10);
	//Set prescaling 64, this enable the timer //Set it to 64 because of time messurement in time.c (other values are also possible, you must call "initTime();" after change this)
	TCCR1B |= _BV(CS11) | _BV(CS10);
	
	//Set Phase-Correct PWM mode for OC2A / OC2B 		//ATmega_644.pdf, Page 148
	TCCR2A |= _BV(WGM20);
	//Set prescaling 64, this enable the timer
	TCCR2B |= _BV(CS21);
	
	
	//Disable JTAG Interface to use them (PC2-PC5) as normal Pins
	MCUCR |= _BV(JTD);
	MCUCR |= _BV(JTD); //must write this twice to disaple it!
	
	sei();
	
	
	//initialice time messurement
	initTime();
	
	//Enable Power for Motor / IH
	digitalWrite(SafetyChain, HIGH);
	digitalWrite(IHOff, LOW);
	
	//Verify RaspiHas has Power
	digitalWrite(RaspiReset, LOW);
	
	
	//Init LoL-Shield
	LedSign_Init(DOUBLE_BUFFER | GRAYSCALE);  //Initializes the screen
	LedSign_Clear(3);
	LedSign_Horizontal(5,PIXEL_ON);
	LedSign_Vertical(3,PIXEL_ON);
	LedSign_Flip(false);
	
	
	digitalWrite(Ventil, wdtRestart);
	_delay_ms(1000);
	wdtRestart = false;
	digitalWrite(Ventil, wdtRestart);
	
	
	//initWatchDog();
	
	uint16_t ihTemp = 0;
	uint8_t ihTemp8bit;
	boolean pingFromDaemon = false;
	//uint8_t led_pin = 0;
	//uint8_t x=0, y=0;
	while(1){
		//TODO read input
		//triggerWatchDog(pingFromDaemon);
		controlIHTemp();
		
		digitalWrite(Ventil, digitalRead(isIHOn));
		/*
		LedSign_Set(x, y, PIXEL_ON);
		LedSign_Flip(false);
		_delay_ms(1000);
		LedSign_Set(x, y, PIXEL_OFF);
		LedSign_Flip(false);
		x++;
		if (x>DISPLAY_COLS){
			x=0;
			y++;
			if (y>DISPLAY_ROWS){
				y=0;
			}
		}
		*/
		
		/*
		pinMode(DisplayX, OUTPUT);
		digitalWrite(DisplayX, HIGH);
		digitalRead(DisplayX);
		_delay_ms(1000);
		digitalWrite(DisplayX, LOW);
		pinMode(DisplayX, INPUT);
		led_pin++;
		DisplayX.port_pin++;
		if (led_pin>11){
			DisplayX.port = PC;
			DisplayX.port_pin = 0;
			led_pin=0;
		} else if (led_pin>7){
			DisplayX.port = PB;
			DisplayX.port_pin = 0;
		}
		*/
	}
	
	return 0;
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
	digitalWrite(IHOff, HIGH);
	analogWrite(IHPowerPWM_TIMER, 0);
	//stop motor
	analogWrite(MotorPWM_TIMER, 0);
	
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
