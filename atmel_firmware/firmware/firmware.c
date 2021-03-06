#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <stdio.h>

#include "mytypes.h"
#include "firmware.h"
#include "input.h"
#include "time.h"
#include "communication.h"
#include "motor.h"
#include "heating.h"
#include "status.h"
#include "watchdog.h"
#include "DisplayHandler.h"
#include "Charliplexing.h"

#define DEBUG_MODE

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
#define SPI_MODE_MAINTENANCE				11
#define SPI_MODE_GET_MOTOR_SPEED			12
#define SPI_MODE_GET_IGBT_TEMP				13
#define SPI_MODE_GET_HEATING_OUTPUT_LEVEL	14
#define SPI_MODE_GET_MOTOR_POS_SENSOR		15
#define SPI_MODE_GET_MOTOR_RPM				16
#define SPI_MODE_GET_FAN_PWM				17
#define SPI_MODE_GET_DEBUG					18

//struct pinInfo PIN_RaspiReset = PA_0; //out 1 to remove power
//struct pinInfo PIN_IHTempSensor = PA_1; //Analog
//struct pinInfo PIN_MotorPosSensor = PA_2; //in		//Status Bit 6
struct pinInfo PIN_Ventil = PA_3; //out
//struct pinInfo PIN_SafetyChain = PA_4; //out 1 to power
struct pinInfo PIN_PusherLocked = PD_3; //in		//Status Bit 3
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

//struct pinInfo PIN_SimulateButton0 = PD_2; //out
//struct pinInfo PIN_isIHOn = PD_3; //in				//Status Bit 4
struct pinInfo PIN_IHOn = PD_4; //out
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
uint8_t Vdebug=0;

uint8_t currentMode = 0;
uint8_t lastPercentValue = 0;
boolean updatePercentAgain = false;
uint16_t picture[9];
unsigned long lastTextUpdate = 0;
#define TEXT_UPDATE_TIMEOUT 40


uint16_t picture_hi[9] = {
	0b00000000000000,
	0b00110110011000,
	0b00110110011000,
	0b00110110000000,
	0b00111110011000,
	0b00110110011000,
	0b00110110011000,
	0b00110110011000,
	0b00000000000000
};
uint16_t picture_0[9] = {
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_1[9] = {
	0b10000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_2[9] = {
	0b11000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_3[9] = {
	0b11100000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_4[9] = {
	0b11110000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_5[9] = {
	0b11111000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_6[9] = {
	0b11111100000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_7[9] = {
	0b11111110000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_8[9] = {
	0b11111111000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_9[9] = {
	0b11111111100000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_10[9] = {
	0b11111111110000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_11[9] = {
	0b11111111111000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_12[9] = {
	0b11111111111100,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_13[9] = {
	0b11111111111110,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_14[9] = {
	0b11111111111111,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_15[9] = {
	0b11111111111111,
	0b10000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_16[9] = {
	0b11111111111111,
	0b11000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_17[9] = {
	0b11111111111111,
	0b11100000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};
uint16_t picture_18[9] = {
	0b11111111111111,
	0b11110000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000,
	0b00000000000000
};

uint16_t picture_chess[9]  = {
	0b10101010101010,
	0b01010101010101,
	0b10101010101010,
	0b01010101010101,
	0b10101010101010,
	0b01010101010101,
	0b10101010101010,
	0b01010101010101,
	0b10101010101010
};

int main (void)
{
	//Set Pin Modes
//	pinMode(PIN_RaspiReset, OUTPUT);
//	pinMode(PIN_IHTempSensor, INPUT/*_ANALOG*/);
//	pinMode(PIN_MotorPosSensor, INPUT_PULLUP);
	pinMode(PIN_Ventil, OUTPUT);
//	pinMode(PIN_SafetyChain, OUTPUT);
	pinMode(PIN_PusherLocked, INPUT_PULLUP);
	pinMode(PIN_LidLocked, INPUT_PULLUP);
	pinMode(PIN_LidClosed, INPUT_PULLUP);
//	pinMode(PIN_SimulateButton0, OUTPUT);
//	pinMode(PIN_IHOff, OUTPUT);
//	pinMode(PIN_isIHOn, INPUT_PULLUP);
//	pinMode(PIN_IHOn, OUTPUT);
//	pinMode(PIN_MotorPWM, OUTPUT/*_PWM*/);
//	pinMode(PIN_IHPowerPWM, OUTPUT/*_PWM*/);
//	pinMode(PIN_IHFanPWM, OUTPUT/*_PWM*/);

	Motor_init();
	Heating_init();
	
	
	initTime();
	//Init LoL-Shield
//	#ifdef GRAYSCALE
	LedSign_Init(DOUBLE_BUFFER | GRAYSCALE);  //Initializes the screen
//	#else
//	LedSign_Init(DOUBLE_BUFFER);  //Initializes the screen
//	#endif
	LedSign_Clear(0);
	LedSign_Flip(false);
	
	//initialize time messurement
	//initTime();
	
	//Init SPI
	SPI_init(wdtRestart);
	
	//Disable JTAG interface
	MCUCR = _BV(JTD);
	MCUCR = _BV(JTD); //Need to write twice to disable it (Atmega_644.pdf, page 267, 23.8.1)
	
	
#ifdef DEBUG_MODE
	if (isExternalReset){
		wdtRestartCount = wdtRestartCount + 1;
	}
	//TODO remove, test was WTD reset
	if (wdtRestart && wdtRestartLast){
		wdtRestart = false;
	}
	wdtRestartLast = wdtRestart;
	digitalWrite(PIN_Ventil, wdtRestart);
	wdtRestart = false;
	
	char valueAsString[10];
	sprintf(valueAsString, "R%X", wdtRestartCount); //HEX
	//sprintf(valueAsString, "%d", wdtRestartCount);  //DEC
	DisplayHandler_setText2(valueAsString);
	DisplayHandler_displayText(false);
	_delay_ms(50);
#endif

	LedSign_Clear(PIXEL_HALF);
	LedSign_Flip(true);
	_delay_ms(100);
	LedSign_Clear(PIXEL_OFF);
	LedSign_Flip(true);
	
	/*
	digitalWrite(PIN_Ventil, HIGH);
	_delay_ms(5000);
	digitalWrite(PIN_Ventil, LOW);
	*/
	if (isStartup){
		//Was power On reset		
		LedSign_Clear(PIXEL_OFF);
		LedSign_Flip(true);
		DisplayHandler_setText("EveryCook is starting");
		currentMode = SPI_MODE_DISPLAY_TEXT;	
	} else {
		//is other than power on reset
		DisplayHandler_setPicture(&picture_hi[0]);
		DisplayHandler_DisplayBitMap();
	}

	initWatchDog();
	

	
	while(1){
		Motor_motorControl();
		Heating_heatControl();
		Heating_controlIHTemp();
		checkLocks();
		if (!LidClosed) Motor_setMotor(0);
		if (availableSPI() > 0) {
			triggerWatchDog(true);
			uint8_t data;
			uint8_t newMode = readSPI(true);
			char* newText;
			uint8_t readAmount = 0;
			boolean setOn = false;
			boolean setOff = false;
			switch(newMode){
				case SPI_MODE_IDLE:
				//DisplayHandler_setPicture(&picture_0[0]);
				//DisplayHandler_DisplayBitMap();
				break;
				case SPI_MODE_GET_STATUS:
					nextResponse = StatusByte;
					//DisplayHandler_setPicture(&picture_1[0]);
					//DisplayHandler_DisplayBitMap();
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
					if(LidClosed){
						Motor_setMotor(data);
					}else{
						Motor_setMotor(0);
					}
					nextResponse = SPI_CommandOK;
					//DisplayHandler_setPicture(&picture_2[0]);
					//DisplayHandler_DisplayBitMap();
				break;
				case SPI_MODE_HEATING:
					data = readSPI(true);
					wdt_reset();
					Heating_setHeating(data);
					nextResponse = SPI_CommandOK;
					//DisplayHandler_setPicture(&picture_3[0]);
					//DisplayHandler_DisplayBitMap();
				break;
				case SPI_MODE_GET_DEBUG:
					nextResponse = Vdebug;
					//DisplayHandler_setPicture(&picture_18[0]);
					//DisplayHandler_DisplayBitMap();
				break;
				case SPI_MODE_VENTIL:
					VentilState = readSPI(true);
					wdt_reset();
					digitalWrite(PIN_Ventil, VentilState);
					nextResponse = SPI_CommandOK;
					//DisplayHandler_setPicture(&picture_10[0]);
					//DisplayHandler_DisplayBitMap();
				break;
				
				case SPI_MODE_DISPLAY_PERCENT:
				case SPI_MODE_DISPLAY_PERCENT_TEXT:
					data = readSPI(true);
					wdt_reset();
					DisplayHandler_displayProgress(data, newMode == SPI_MODE_DISPLAY_PERCENT_TEXT);
					lastPercentValue = data;
					if (newMode != SPI_MODE_DISPLAY_PERCENT_TEXT){
						nextResponse = SPI_CommandOK;
						currentMode = newMode;
						break;
					}
				case SPI_MODE_DISPLAY_TEXT:
				case SPI_MODE_DISPLAY_TEXT_SMALL:
					if (DisplayHandler_readText()) {//[01]<textlen 1 Byte><text>[00<control byte>]
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
					} else {
						nextResponse = SPI_Error_Text_Invalid;
					}
				break;
				
				case SPI_MODE_DISPLAY_PICTURE:
					readAmount=0;
					while (readAmount < 9){
						if (availableSPI() >= 2) {
							data = readSPI(true);
							picture[readAmount] = data << 8;
							data = readSPI(true);
							picture[readAmount] |= data;
							readAmount++;
							wdt_reset();
						} else {
							_delay_ms(1);
						}
					}
					if (availableSPI() == 0) {
						_delay_ms(10);
						if (availableSPI() == 0) {
							_delay_ms(20);
						}
					}
					if (availableSPI() > 0 && peekSPI() == 0x00) {
						readSPI(false);
						DisplayHandler_setPicture(&picture[0]);
						DisplayHandler_DisplayBitMap();
						currentMode = newMode;
						nextResponse = SPI_CommandOK;
					} else {
						DisplayHandler_setPicture(&picture[0]);
						DisplayHandler_DisplayBitMap();
						currentMode = newMode;
						nextResponse = SPI_Error_Picture_Invalid;
					}
				break;
				
				case SPI_MODE_MAINTENANCE:
					data = readSPI(true);
					if (data == 0x99) {
						setOn = true;
					} else if (data == 0x22){
						setOff = true;
					}
					//be sure its change to maintenance, it has the be send 2 times
					data = readSPI(true);
					if (data == SPI_MODE_MAINTENANCE) {
						data = readSPI(true);
						if (setOn && data != 0x99) {
							setOn = false;
						} else if (setOff && data != 0x22){
							setOff = false;
						}
						data = readSPI(true);
						//00 as command end mark
						if (data == 0x00){
							if (setOn){
								isMaintenanceMode = true;
								nextResponse = SPI_CommandOK;
							} else if (setOff){
								isMaintenanceMode = false;
								nextResponse = SPI_CommandOK;
							} else {
								nextResponse = SPI_Error_Unknown_Command;
							}
						} else {
							nextResponse = SPI_Error_Unknown_Command;
						}
					} else {
						nextResponse = SPI_Error_Unknown_Command;
					}
				break;
				
				case SPI_MODE_GET_MOTOR_SPEED:
					nextResponse = outputValueMotor;
					//DisplayHandler_setPicture(&picture_12[0]);
					//DisplayHandler_DisplayBitMap();
				break;
				
				case SPI_MODE_GET_IGBT_TEMP:
					nextResponse = ihTemp8bit;
					//DisplayHandler_setPicture(&picture_hi[0]);
					//DisplayHandler_DisplayBitMap();
				break;
				case SPI_MODE_GET_FAN_PWM:
					nextResponse = lastIHFanPWM;
					//DisplayHandler_setPicture(&picture_17[0]);
					//DisplayHandler_DisplayBitMap();
				break;
				
				case SPI_MODE_GET_HEATING_OUTPUT_LEVEL:
					nextResponse = 0;
					//nextResponse = Heating_getLastOnPWM();
					//DisplayHandler_setPicture(&picture_14[0]);
					//DisplayHandler_DisplayBitMap();
				break;
				
				case SPI_MODE_GET_MOTOR_POS_SENSOR:
					nextResponse = lastSensorValue;
					//DisplayHandler_setPicture(&picture_15[0]);
					//DisplayHandler_DisplayBitMap();
				break;
				
				case SPI_MODE_GET_MOTOR_RPM:
					nextResponse = rpm;
					//DisplayHandler_setPicture(&picture_16[0]);
					//DisplayHandler_DisplayBitMap();
				break;
				
				default:
					nextResponse = SPI_Error_Unknown_Command;
			}
		} else {
			triggerWatchDog(false);
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
						if(!DisplayHandler_displayText(currentMode != SPI_MODE_DISPLAY_TEXT)){
							currentMode = 0;
						}
					}
				break;
			}
			//DisplayHandler_setPicture(&picture_0[0]);
			//DisplayHandler_DisplayBitMap();
		}	
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
