#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "watchdog.h"
#include "mytypes.h"
#include "time.h"
#include "input.h"
#include "motor.h"
#include "heating.h"

//if it was a watchdog reset, diesable watchdog in early startup (logic from avr/wdt.h)
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

boolean isStartup __attribute__ ((section (".noinit")));
boolean isExternalReset __attribute__ ((section (".noinit")));
boolean isMaintenanceMode __attribute__ ((section (".noinit")));
boolean wdtRestart __attribute__ ((section (".noinit")));
boolean wdtRestartLast __attribute__ ((section (".noinit")));
uint8_t wdtRestartCount __attribute__ ((section (".noinit")));

uint16_t lastDataTime __attribute__ ((section (".noinit")));
boolean saftyChainOff __attribute__ ((section (".noinit")));
uint16_t resetAtmelTime __attribute__ ((section (".noinit")));
uint16_t resetDaemonTime __attribute__ ((section (".noinit")));
boolean deamonResetButtonPress __attribute__ ((section (".noinit")));
uint16_t resetRaspiTime __attribute__ ((section (".noinit")));
boolean raspiResetButtonPress __attribute__ ((section (".noinit")));

struct pinInfo PIN_RaspiReset = PA_0; //out 1 to remove power
struct pinInfo PIN_SafetyChain = PA_4; //out 1 to power
struct pinInfo PIN_SimulateButton0 = PD_2; //out

//if it was a watchdog reset, disable watchdog in early startup (logic from avr/wdt.h)
void get_mcusr(void) \
  __attribute__((naked)) \
  __attribute__((section(".init3")));

void get_mcusr(void)
{
  mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
  wdtRestart = (mcusr_mirror & _BV(WDRF)) ? true : false;
  isStartup = (mcusr_mirror & _BV(PORF)) ? true : false;
  isExternalReset = (mcusr_mirror & _BV(EXTRF)) ? true : false;
}

void initWatchDog(){
	cli();
	pinMode(PIN_RaspiReset, OUTPUT);
	pinMode(PIN_SafetyChain, OUTPUT);
	pinMode(PIN_SimulateButton0, OUTPUT);
	
	if (isStartup && lastDataTime == 0){
		lastDataTime = millis();
		deamonResetButtonPress = false;
	}
	
	//Enable Power for Motor / IH
	digitalWrite(PIN_SafetyChain, HIGH);
	saftyChainOff = false;
	
	//Verify Raspi has Power
	if(!raspiResetButtonPress){
		digitalWrite(PIN_RaspiReset, LOW);
	}
	
	//Verify Button is NOT "pressed"
	if(!deamonResetButtonPress){
		digitalWrite(PIN_SimulateButton0, LOW);
	}
	
	//init Watchdog
	// allow changes, disable reset
	WDTCSR = _BV (_WD_CHANGE_BIT) | _BV (WDE);  //_WD_CHANGE_BIT(new) or WDCE
	// set interrupt mode and an interval
	//WDTCSR = _BV (WDIE) | _BV (WDP3) | _BV (WDP0);    // set WDIE(=interrupt mode), and 8 seconds delay
	WDTCSR = _BV (WDIE) | _BV (WDE) | _BV (WDP2) | _BV (WDP1) | _BV (WDP0);    // set WDIE(=interrupt mode), and 2 seconds delay
	
	//wdt_enable(WDTO_2S); //15MS,30MS,60MS,120MS,250MS,500MS,1S,2S,4S,8S
	wdt_reset();
	sei(); //Start interrupts
}

void triggerWatchDog(boolean dataFromDaemon){
	uint16_t currentMillis = millis();
	wdt_reset();
	if (dataFromDaemon){
		isStartup = false;
		lastDataTime = currentMillis;
		if (saftyChainOff){
			digitalWrite(PIN_SafetyChain, HIGH);
			saftyChainOff = false;
		}
		if (resetDaemonTime != 0) {
			resetAtmelTime = 0;
			if (deamonResetButtonPress){
				if (currentMillis - resetDaemonTime > DAEMON_RESET_BUTTON_IGNORE_TIME - DAEMON_RESET_BUTTON_TIME){
					digitalWrite(PIN_SimulateButton0, LOW);
					deamonResetButtonPress = false;
					resetDaemonTime = 0;
				}
			} else {
				resetDaemonTime = 0;
			}
			resetRaspiTime = 0;

		} else if (resetAtmelTime != 0) {
			//if only resetAtmelTime is != 0 WatchDog is still running
			resetAtmelTime = 0;
		}
	} else {
		if (isMaintenanceMode){
			//do no check if in MaintenanceMode
		} else if (isStartup && currentMillis - lastDataTime<RASPI_RESET_WAIT_TIME){
			//all OK, is still on Startup
		} else {
			if (!saftyChainOff && (currentMillis - lastDataTime > NODATA_TIMEOUT_SAFTY_CHAIN_OFF)){
				//Disable safety chain
				digitalWrite(PIN_SafetyChain, LOW);
				saftyChainOff = true;
				//stop heating
				//analogWrite(IHPowerPWM_TIMER, 0);
				//stop motor
				//analogWrite(MotorPWM_TIMER, 0);
			}
			
			if (resetDaemonTime != 0) {
				if (resetRaspiTime != 0){
					uint16_t deltaTime = currentMillis - resetRaspiTime;
					if (deltaTime > 0 &&raspiResetButtonPress){
						digitalWrite(PIN_RaspiReset, LOW);
						raspiResetButtonPress = false;
					} else if (deltaTime > RASPI_RESET_WAIT_TIME){
						//Raspi should be started now, and dataFromDaemon should come up
						//If i come here the reset dosn't fix the problem, what to do???
						//TODO: Show error on display 
					}
				} else {
					uint16_t deltaTime = currentMillis - resetDaemonTime;
					if (deltaTime > 0 && deamonResetButtonPress){
						digitalWrite(PIN_SimulateButton0, LOW);
						deamonResetButtonPress = false;
					} else if (deltaTime > DAEMON_RESET_WAIT_TIME){
						//deamon reset do not fix the problem, reset the raspi
						digitalWrite(PIN_RaspiReset, HIGH);
						resetRaspiTime = currentMillis - RASPI_RESET_BUTTON_TIME;
						raspiResetButtonPress = true;
					}
				}
			} else if (currentMillis - lastDataTime > NODATA_TIMEOUT_RESET_BUTTON){
				digitalWrite(PIN_SimulateButton0, HIGH);
				resetDaemonTime = currentMillis - DAEMON_RESET_BUTTON_TIME;
				deamonResetButtonPress = true;
				/*
				_delay_ms(DAEMON_RESET_BUTTON_TIME);
				digitalWrite(PIN_SimulateButton0, LOW);
				//resetDaemonTime = currentMillis;
				resetDaemonTime = millis();
				*/
			}
		}
	}
}

//############################ Watchdog timeout exeded ############################
ISR(WDT_vect){
	resetAtmelTime = millis();
/*
//	cli();
	
	wdt_reset(); //say wdt all is ok
	mcusr_mirror = MCUSR;
	
	// Clear WDRF in MCUSR
	MCUSR &= ~_BV (WDRF);
	// Write logical one to WDCE and WDE, to enable changes
	// Keep old prescaler setting to prevent unintentional time-out
	WDTCSR |= _BV (_WD_CHANGE_BIT) | _BV (WDE); 
	// Turn off WDT
	WDTCSR = 0x00;
	
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
	
//	sei();
*/
}
