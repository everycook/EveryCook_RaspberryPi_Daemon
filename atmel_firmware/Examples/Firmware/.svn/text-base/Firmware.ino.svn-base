#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <Charliplexing.h> //Imports the library, which needs to be Initialized in setup.
#include "DisplayHandler.h"
#include "heating.h"
#include "motor.h"

boolean called __attribute__ ((section (".noinit")));
//if it was a watchdog reset, diesable watchdog in early startup (logic from avr/wdt.h)
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

void get_mcusr(void) \
  __attribute__((naked)) \
  __attribute__((section(".init1")));
void get_mcusr(void)
{
  mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
  called = true;
}


uint8_t lastPercentValue = 0;
boolean updatePercentAgain = false;

uint8_t inByte = 0;         // incoming serial byte


uint8_t currentMode = 0;




void setup() {
  cli(); //Stop interals
  Heating:init();
  Motor::init();
  
  // start serial port at 9600 bps:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  //Init LoL-Shield
  LedSign::Init(DOUBLE_BUFFER | GRAYSCALE);  //Initializes the screen
  
  
//  delay(2000);
  Serial.print("mcusr_mirror:");
  Serial.println(mcusr_mirror,BIN);
  Serial.print("called:"); 
  if (called){
    Serial.println("true"); 
  } else {
    Serial.println("false"); 
  }
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

void loop() {
  boolean run=true;    //While this is true, the screen updates
  
  //Show all leds for a short period of time on startup
  uint16_t picture[9];
  picture[0] = 0xFFFF;
  picture[1] = 0xFFFF;
  picture[2] = 0xFFFF;
  picture[3] = 0xFFFF;
  picture[4] = 0xFFFF;
  picture[5] = 0xFFFF;
  picture[6] = 0xFFFF;
  picture[7] = 0xFFFF;
  picture[8] = 0xFFFF;
  DisplayHandler::setPicture(picture);
  currentMode = 0x03;
  DisplayHandler::DisplayBitMap();
  
  delay(500);
  
  //Show starting text until daemon is ready, daemon will then show star screen/image.
  DisplayHandler::setText("EveryCook is starting");
  currentMode = 0x01;
  
//  uint8_t test = 0;
  while(run == true) {
    wdt_reset();
    if (Serial.available() > 0) {
      uint8_t newMode = Serial.read();
      
      Serial.print("mode:");
      Serial.println(currentMode);
      char* newText;
      uint8_t readAmount = 0;
      switch(newMode){
        case 0x00: //clear				[00]
        case 'C':
          Serial.println("clear");
          LedSign::Clear(0);
          LedSign::Flip(true);  
          break;
        
        
        case 0x01: //text anzeige			[01]<textlen 1 Byte><text>[00<control byte>]
        case 'T':
          Serial.println("text");
          DisplayHandler::readText();
          DisplayHandler::displayText(false);
          break;
        
        
        case 0x02: //prozentAnzeigen			[02]<value 1 Byte>
        case 'A':
          Serial.println("percent");
          while (Serial.available() == 0) {
            delay(10);
          }
          inByte = Serial.read();
          Serial.println(inByte);
          DisplayHandler::displayProgress(inByte, false);
          lastPercentValue = inByte;
        break;
        
        
        case 0x03: //Bild anzeigen			[03]9x2Bytes
        case 'P':
          Serial.println("picture");
          readAmount = 0;
          while (readAmount < 9){
            if(Serial.available() >= 2) {
              inByte = Serial.read();
              picture[readAmount] = inByte << 8;
              inByte = Serial.read();
              picture[readAmount] |= inByte;
              readAmount++;
            } else {
              delay(10);
            }
          }
          DisplayHandler::DisplayBitMap();
        break;
        
        
        case 0x04: //prozentAnzeigen mit laufschrift	[04]<value 1 Byte><textlen 1 Byte><text>[00<control byte>]
        case 't':
          Serial.println("percent and text");
          while (Serial.available() == 0) {
            delay(10);
          }
          inByte = Serial.read();
          Serial.println(inByte);
          DisplayHandler::readText();
          DisplayHandler::displayProgress(inByte, true);
          lastPercentValue = inByte;
          updatePercentAgain = true;
          DisplayHandler::displayText(true);
        break;
        
        
        case 0x05:
        case 'H': //Heating on
          Heating::setHeating(true);
        break;
        
        
        case 0x06:
        case 'h': //Heating off
          Heating::setHeating(false);
        break;
        
        
        case 0x07://Motor                               [07]<speed 1 byte>
        case 'M':
          Serial.println("Motor");
          while (Serial.available() == 0) {
            delay(10);
          }
          inByte = Serial.read();
          Serial.println(inByte);
          
          Motor::setMotor(inByte);
        break;
        
        
        case 0x08://Motor stop                          [08]
        case 'm':
          Serial.println("Motor stop");
          Motor::setMotor(0);
        break;
        
        
        case 'E': //Exit
          Serial.println("exit");
          run = false;
        break;
        
        
        default: //Unknown value, clear screen
          Serial.println("unknown command");
          LedSign::Clear(0);
          newMode = currentMode; //don't override currentMode
        break;
      }
      
      currentMode = newMode;
    } else {
      switch(currentMode){
/*        //Testmode
        case 0x00:
          ++test;
          if (test>140){
            test = 0;
          }
          displayProgress(test, false);
        break;
*/
        case 0x01:
        case 'T':
          if(!DisplayHandler::displayText(false)){
            currentMode = 0;
          }
          break;
        case 0x04:
        case 't':
          //if (textXCord == 13 && textCharPos == 0) //2.text step also show progress, so not blinking because of doubble buffer
          if (updatePercentAgain) {
            DisplayHandler::displayProgress(lastPercentValue, true);
            updatePercentAgain = false;
          }
          
          if(!DisplayHandler::displayText(true)){
            currentMode = 0;
          }
        break;
      }
    }
    
    Heating::heatControl();
    Motor::motorControl();
    
    delay(100);
  }
}




//Watchdog timeout exeded
ISR(WDT_vect){
  cli();
  wdt_reset();
  
  mcusr_mirror = MCUSR;
//  MCUSR = 0;
//  wdt_disable();
  Serial.println("ISR(WDT_vect)");  

  /* Clear WDRF in MCUSR */
//  MCUSR &= ~_BV (WDRF);
  /* Write logical one to WDCE and WDE */
  /* Keep old prescaler setting to prevent unintentional time-out */
//  WDTCSR |= _BV (_WD_CHANGE_BIT) | _BV (WDE); 
  /* Turn off WDT */
//  WDTCSR = 0x00;


  /*
  //reenable wdt to do systemreset use following lines of code for it:
  // allow changes, disable reset
  WDTCSR = _BV (_WD_CHANGE_BIT) | _BV (WDE);
  // set interrupt AND reset mode and an interval
  WDTCSR = _BV (WDIE) | _BV (WDE) | _BV (WDP2) | _BV (WDP1) | _BV (WDP0);    // set WDIE(=interrupt mode), and 2 seconds delay
//  // set reset mode and an interval
//  WDTCSR = _BV (WDE) | _BV (WDP2) | _BV (WDP1) | _BV (WDP0);    // set WDIE(=interrupt mode), and 2 seconds delay
  */


  
  //what to do?
  //stop heating
  //stop motor
  //try restart atmel -> System reset
  //try restart deamon -> send command/ set pin to high to do so
  //if demaon restart don't work, restart raspi -> set pin to remove power?
  sei();
}

