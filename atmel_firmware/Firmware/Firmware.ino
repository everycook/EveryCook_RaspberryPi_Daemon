#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <Charliplexing.h> //Imports the library, which needs to be Initialized in setup.
#include "FontHandler.h"
#include "NormalFont.h"
#include "SmallFont.h"

boolean called=false;
//if it was a watchdog reset, diesable watchdog in early startup (logic from avr/wdt.h)
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

void get_mcusr(void) \
  __attribute__((naked)) \
  __attribute__((section(".init3")));
void get_mcusr(void)
{
  called = true;
  mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
}



//DISPLAY_COLS x DISPLAY_ROWS
const uint8_t PERCENT_FIRST_ROW = 6; //0-8
const uint8_t PERCENT_LAST_ROW = 8; //0-8
const uint8_t PERCENT_100_PERCENT_COL = 10;

const uint8_t PERCENT_TEXT_NONE = 0;
const uint8_t PERCENT_TEXT_STOP = 1;
const uint8_t PERCENT_TEXT_TO_MUTCH = 2;


const uint8_t PIXEL_ON = 7;
const uint8_t PIXEL_HALF = 2;
const uint8_t PIXEL_OFF = 0;


uint8_t inByte = 0;         // incoming serial byte

char *textToShow = "EveryCook is starting";
int8_t textXCord = DISPLAY_COLS;
int8_t textYCord = 0;
int8_t textCharPos = 0;

uint8_t lastPercentValue = 0;
uint8_t lastPercentCols = 0;
boolean lastHalfLed = false;
uint8_t lastPercentText = 0;
boolean updatePercentAgain = false;

uint16_t picture[9];

uint8_t currentMode = 0;



boolean motorStop = false;
boolean motorStoped = false;
uint8_t motorSpeed = 0;

boolean heating = false;

//IH vars
const int PowerFBPin = 12;
const int StartSeqPin = 8;
const int PWMPin = 9;
const int IHStartPin = 10;

const int Increment = 5;
const int pause=100;
int runValue = 1;        
int runRead = 0;        
int outputValueIH = 50;        
int start = 0;
int startValue = 1;

//Motor vars
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to
const int outPin = 11;
const int sensorPin= A2;
long LastSensorTrigger = 0;
int LastSensorValue=1;
int slowRotationTime=500;
int AnalogAverage=20;

long previousMillis = 0;
long interval = 1000;   
int rotationTime=0;

int analogValue = 0;        // value read from the pot
int sensorValue = 0;        
int outputValueMotor = 0;        // value output to the PWM (analog out)
int setValue=0;


void setup();
void loop();
void clearPercentTextArea();
void displayProgress(uint8_t percent, boolean noFlip);
void DisplayBitMap();
boolean readText();
/*void displayTextWait(const char *textToShow);*/
void displaySmallText(const char *textToShow);
void displayText(const char *text);

void setup() {
  cli(); //Stop interals
  //init IH Pins
  pinMode(PowerFBPin,OUTPUT);
  digitalWrite(PowerFBPin,HIGH);
  pinMode(PowerFBPin,INPUT);
  pinMode(StartSeqPin,OUTPUT);
  digitalWrite(StartSeqPin,HIGH);
  pinMode(StartSeqPin,INPUT);
  pinMode(PWMPin,OUTPUT);
  pinMode(IHStartPin,OUTPUT);
  digitalWrite(IHStartPin,LOW);
  
  //init Motor
  pinMode(outPin,OUTPUT);
  pinMode(analogInPin,INPUT);
  pinMode(sensorPin,INPUT_PULLUP);
  
  // start serial port at 9600 bps:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  LedSign::Init(DOUBLE_BUFFER | GRAYSCALE);  //Initializes the screen
  
  Serial.print("mcusr_mirror:");
  Serial.println(mcusr_mirror,BIN);
  Serial.print("called:"); 
  if (called){
    Serial.println("true"); 
  } else {
    Serial.println("false"); 
  }
  /*
    MCUSR – MCU Status Register
  • Bit 3 – WDRF: Watchdog System Reset Flag
  • Bit 2 – BORF: Brown-out Reset Flag
  • Bit 1 – EXTRF: External Reset Flag
  • Bit 0 – PORF: Power-on Reset Flag
  */
  
  //Watchdog
  // allow changes, disable reset
  WDTCSR = _BV (_WD_CHANGE_BIT) | _BV (WDE);  //_WD_CHANGE_BIT(new) or WDCE
  // set interrupt mode and an interval
  //WDTCSR = _BV (WDIE) | _BV (WDP3) | _BV (WDP0);    // set WDIE(=interrupt mode), and 8 seconds delay
  WDTCSR = _BV (WDIE) | _BV (WDP2) | _BV (WDP1) | _BV (WDP0);    // set WDIE(=interrupt mode), and 2 seconds delay
  
  //wdt_enable(WDTO_2S); //15MS,30MS,60MS,120MS,250MS,500MS,1S,2S,4S,8S
  wdt_reset();
  sei(); //Start intervals
}

void loop() {
  boolean run=true;    //While this is true, the screen updates
  
  //Show all leds for a short period of time on startup
  picture[0] = 0xFFFF;
  picture[1] = 0xFFFF;
  picture[2] = 0xFFFF;
  picture[3] = 0xFFFF;
  picture[4] = 0xFFFF;
  picture[5] = 0xFFFF;
  picture[6] = 0xFFFF;
  picture[7] = 0xFFFF;
  picture[8] = 0xFFFF;
  currentMode = 0x03;
  DisplayBitMap();
  
  delay(500);
  
  //Show starting text until daemon is ready, daemon will then show star screen/image.
  textXCord = DISPLAY_COLS;
  textYCord = 0;
  textCharPos = 0;
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
          readText();
          if (textToShow != NULL){
            textXCord = DISPLAY_COLS;
            textYCord = 0;
            textCharPos = 0;
            displayText(textToShow);
          }
          break;
        
        
        case 0x02: //prozentAnzeigen			[02]<value 1 Byte>
        case 'A':
          Serial.println("percent");
          while (Serial.available() == 0) {
            delay(10);
          }
          inByte = Serial.read();
          Serial.println(inByte);
          displayProgress(inByte, false);
          /*
//          LedSign::SetBrightness(60); 0-127
          for (uint8_t p=0; p<140; p++){
            displayProgress(p, false);
            delay(500);
          }
          */
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
          DisplayBitMap();
        break;
        
        
        case 0x04: //prozentAnzeigen mit laufschrift	[04]<value 1 Byte><textlen 1 Byte><text>[00<control byte>]
        case 't':
          Serial.println("percent and text");
          while (Serial.available() == 0) {
            delay(10);
          }
          inByte = Serial.read();
          Serial.println(inByte);
          readText();
          displayProgress(inByte, true);
          updatePercentAgain = true;
          if (textToShow != NULL){
            textXCord = DISPLAY_COLS;
            textYCord = 0;
            textCharPos = 0;
            displaySmallText(textToShow);
          }
        break;
        
        
        case 0x05:
        case 'H': //Heating on
          heating = true;
        break;
        
        
        case 0x06:
        case 'h': //Heating off
          heating = false;
        break;
        
        
        case 0x07://Motor                               [07]<speed 1 byte>
        case 'M':
          Serial.println("Motor");
          while (Serial.available() == 0) {
            delay(10);
          }
          inByte = Serial.read();
          Serial.println(inByte);
          motorSpeed = inByte;
          motorStop = (motorSpeed == 0);
          
          outputValueMotor=255/1;
          setValue=1;
        break;
        
        
        case 0x08://Motor stop                          [08]
        case 'm':
          Serial.println("Motor stop");
          motorSpeed = 0;
          motorStop = true;
          
          outputValueMotor=min(outputValueMotor,255/6);
          setValue=0;
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
          if (textToShow != NULL){
            displayText(textToShow);
          }
          break;
        case 0x04:
        case 't':
          //if (textXCord == 13 && textCharPos == 0) //2.text step also show progress, so not blinking because of doubble buffer
          if (updatePercentAgain) {
            displayProgress(lastPercentValue, true);
            updatePercentAgain = false;
          }
          
          if (textToShow != NULL){
            displaySmallText(textToShow);
          }
        break;
      }
    }
    
    if (heating){
      startValue=digitalRead(StartSeqPin);
      if (startValue==0) {start=1; runValue=1;}
    //  else {start=0;}
      runRead=digitalRead(PowerFBPin);
      if(runRead==0)  {runValue=0;}
      if(runValue==1 && start==1){
        outputValueIH+=Increment;
        digitalWrite(IHStartPin,HIGH);
        delay(20);
        digitalWrite(IHStartPin,LOW);
      }
      analogWrite(PWMPin, outputValueIH);
      // print the results to the serial monitor:
      Serial.print("start = " );                       
      Serial.print(startValue);      
      Serial.print("\t feedback = " );                       
      Serial.print(runRead);      
      Serial.print("\t output = ");      
      Serial.println(outputValueIH);
      delay(pause);   
    } else if (runValue == 1){
      /*
      digitalWrite(IHStopPin,HIGH);
      delay(20);
      digitalWrite(IHStopPin,LOW);
      */
      analogWrite(PWMPin, 0);
      outputValueIH = 50;
      runValue = 0;
    }
    
    if (outputValueMotor != 0){
      sensorValue = digitalRead(sensorPin);
      if (sensorValue==0 && LastSensorValue==1) {
        rotationTime=millis()-LastSensorTrigger;
        LastSensorTrigger=millis();
        if (setValue==0 && rotationTime>=slowRotationTime) {
          outputValueMotor=0;
          motorStoped = true;
        }
      }
      LastSensorValue=sensorValue;
    
      analogWrite(outPin, outputValueMotor);       
    
      unsigned long currentMillis = millis();
    
      if(currentMillis - previousMillis > interval) {
        // save the last time you blinked the LED 
        previousMillis = currentMillis;   
        analogValue=0;
        for (int i=0;i<AnalogAverage;i++) {
          analogValue += analogRead(analogInPin);            
          analogValue = analogValue/AnalogAverage;
        }
        // print the results to the serial monitor:
        Serial.print("setvalue = " );                       
        Serial.print(analogValue);      
        
        Serial.print("\t sensorvalue = " );                       
        Serial.print(sensorValue);      
        int RPM=60000/rotationTime;
        Serial.print("\t RPM = " );                       
        Serial.print(RPM);      
        Serial.print("\t rTime = " );                       
        Serial.print(rotationTime);      
        Serial.print("\t output = ");      
        Serial.println(outputValueMotor);   
      }
    }
    
    delay(100);
  }
}


void clearPercentTextArea(){
  for (uint8_t y=0; y<5; y++){
     LedSign::Horizontal(y, PIXEL_OFF);
  }
}

/*displayProgress logic:
- we have 14 led's
- you see 100% indicator on the 12th col from beginning (4 rows)
- the 100% indicator will fade out, if you came near the 100% ()
- percentage are NOT linear:
  - 0-80 are 8 leds, linear (10% each led) 8
  - 80-88 is 1 led    84      (8% each led)  9
  - 88-95 is 1 led    91      (7% each led)  10
  - 95-100 is 1 led   98      (5% each led)  11
  - 100-105 is 1 led  103      (5% each led)  12
  - 105-112 is 1 led  110      (7% each led)  13
  - 112-120 is 1 led  116      (8% each led)  14
*/
void displayProgress(uint8_t percent, boolean noFlip){
  boolean changed = false;
  boolean halfLed = false;
  uint8_t percentCols = 0; //1-14
  uint8_t percentGrayScale100 = 0; //0-7
//  uint8_t percent10 = floor(percent/10.0); //0-7 //debug
  
  //calc leds
  if (percent<80){
    double percentColsDouble = (10.0 * (unsigned int)percent) / 100.0; //linear: 10 leds if there are 100%
    percentCols = floor(percentColsDouble);
    halfLed = (percentColsDouble - percentCols) > 0.5;
    
    if (percentCols>7){
      percentGrayScale100 = 0;
//    } else if (percentCols<2) {
//      percentGrayScale100 = 7;
    } else {
      percentGrayScale100 = 7-percentCols;
    }
  } else if (percent<84){
    percentCols = 8;
    halfLed = false;
  } else if (percent<88){
    percentCols = 8;
    halfLed = true;
  } else if (percent<91){
    percentCols = 9;
    halfLed = false;
  } else if (percent<95){
    percentCols = 9;
    halfLed = true;
  } else if (percent<98){
    percentCols = 10;
    halfLed = false;
  } else if (percent<100){
    percentCols = 10;
    halfLed = true;
  } else if (percent<103){
    percentCols = 11;
    halfLed = false;
  } else if (percent<105){
    percentCols = 11;
    halfLed = true;
  } else if (percent<120){
    percentCols = 12;
    halfLed = false;
  } else if (percent<112){
    percentCols = 12;
    halfLed = true;
  } else if (percent<116){
    percentCols = 13;
    halfLed = false;
  } else if (percent<120){
    percentCols = 13;
    halfLed = true;
  } else {
    percentCols = 14;
    halfLed = false;
  }
  
  if (percentCols != lastPercentCols || halfLed != lastHalfLed){
    changed = true;
  }
  
  //Show Text
  if (percent >= 98 && percent < 105){
    //STOP
    if (lastPercentText != PERCENT_TEXT_STOP || changed){
      lastPercentText = PERCENT_TEXT_STOP;
      changed = true;
      clearPercentTextArea();
      FontHandler::Draw(0, 0, SmallFont::getChar('S'), PIXEL_ON);
      FontHandler::Draw(4, 0, SmallFont::getChar('T'), PIXEL_ON);
      FontHandler::Draw(7, 0, SmallFont::getChar('O'), PIXEL_ON);
      FontHandler::Draw(11, 0, SmallFont::getChar('P'), PIXEL_ON);
    }
    
  } else if (percent >= 105){
    //to mutch
    if (lastPercentText != PERCENT_TEXT_TO_MUTCH || changed){
      lastPercentText = PERCENT_TEXT_TO_MUTCH;
      changed = true;
      clearPercentTextArea();
      //TODO show text
    }
  } else {
    //clear
    if (lastPercentText != PERCENT_TEXT_NONE || changed){
      lastPercentText = PERCENT_TEXT_NONE;
      changed = true;
      clearPercentTextArea();
    }
  }
  
  //Show progressbar
  if (changed){
    if (percentCols>0){
      for (uint8_t x=0; x<percentCols; x++){
        for (uint8_t y=PERCENT_FIRST_ROW; y<=PERCENT_LAST_ROW; y++){
          LedSign::Set(x,y, PIXEL_ON);
        }
      }
    }
    if (halfLed){
      for (uint8_t y=PERCENT_FIRST_ROW; y<=PERCENT_LAST_ROW; y++){
        LedSign::Set(percentCols,y, PIXEL_HALF);
      }
      percentCols++;
    }
    if (percentCols<14){
      for (uint8_t x=percentCols+1; x<DISPLAY_COLS; x++){
        for (uint8_t y=PERCENT_FIRST_ROW; y<=PERCENT_LAST_ROW; y++){
          LedSign::Set(x,y, PIXEL_OFF);
        }
      }
    }
  }
  LedSign::Horizontal(5, PIXEL_OFF);
  if (percentCols<PERCENT_100_PERCENT_COL){
    LedSign::Set(PERCENT_100_PERCENT_COL,6, percentGrayScale100);
    LedSign::Set(PERCENT_100_PERCENT_COL,7, percentGrayScale100);
    LedSign::Set(PERCENT_100_PERCENT_COL,8, percentGrayScale100);
  }
  LedSign::Set(PERCENT_100_PERCENT_COL,5, PIXEL_ON);
/* //debug
  for(int i=0;i<percent10;i++){
    LedSign::Set(i,5, 7);
  }
*/
  if (changed && !noFlip){
    LedSign::Flip(true);
  }
  lastPercentValue = percent;
}

void DisplayBitMap()
{
  byte line = 0;       //Row counter
  unsigned long data;  //Temporary storage of the row data
  for(line = 0; line < 9; line++) {
    data = picture[line];
    for (byte led=0; led<14; ++led) {
      //if (data & (1<<led)) { //data is reversed, last bit is first pixel
      if (data & (1<<(13-led))) { //data is right assigned, so it show the last 14 bits of the word
        LedSign::Set(led, line, PIXEL_ON);
      } else {
        LedSign::Set(led, line, 0);
      }
    }
  }
  LedSign::Flip(true);
}

boolean readText(){
  uint8_t textLen = 0;
  uint8_t readAmount = 0;
  while (Serial.available() == 0) {
    delay(10);
  }
  textLen = Serial.read();
  char *newText = (char *) malloc(textLen * sizeof(char) + 1);
  Serial.print("Text length:");
  Serial.println(textLen);
  while(readAmount < textLen){
    if (Serial.available() > 0) {
        newText[readAmount] = Serial.read();
        Serial.write(newText[readAmount]);
        readAmount++;
    } else {
      delay(10);
    }
  }
  Serial.println();
  boolean sucess = false;
  Serial.println("Text end reached");
  if(newText[readAmount] != 00){
    newText[readAmount] = 0x00;
    if(Serial.available() == 0) {
      delay(100);
    }
    if(Serial.available() == 0) {
      Serial.println("no sign after text end, it have to end with a 0x00");
      sucess = false;
    } else {
      inByte = Serial.peek();
      if (inByte != 0x00) {
        Serial.println("text have to end with a 0x00");
        sucess = false;
      } else {
        inByte = Serial.read();
        Serial.println("text is OK");
        sucess = true;
      }
    }
  } else {
    Serial.println("text end submited");
    sucess = true;
  }
//  if (sucess){
    if (textToShow != NULL){
      free(textToShow);
    }
    textToShow = newText;
//  }
  return sucess;
}

void displaySmallText(const char *text){
  int8_t x=textXCord;
  int8_t i=textCharPos;
//  LedSign::Clear();
  clearPercentTextArea();
  for (int8_t x2=x, i2=i; x2<DISPLAY_COLS;) {
    if (i2 < strlen(text)){
      const uint8_t *character = SmallFont::getChar(text[i2]);
      int8_t w = FontHandler::Draw(x2, textYCord, character);
      x2 += w;
      i2 = (i2+1);
    } else {
      //add a "space"
      x2 += 4;
      i2 = (i2+1);
    }
    if (x2 <= 0){ // the currently drawn character was completely out of screen
      //adjust looping values for next loop
      x = x2, i = i2;
    }
  }
  LedSign::Flip(true);
  x--;
  textXCord=x;
  textCharPos=i;
  if (i == strlen(text) && x <= 0){
    currentMode = 0;
  }
}

void displayText(const char *text){
  int8_t x=textXCord;
  int8_t i=textCharPos;
  LedSign::Clear();
  for (int8_t x2=x, i2=i; x2<DISPLAY_COLS;) {
    if (i2 < strlen(text)){
      const uint8_t *character = NormalFont::getChar(text[i2]);
      int8_t w = FontHandler::Draw(x2, textYCord, character);
      x2 += w;
      i2 = (i2+1);
    } else {
      //add a "space"
      x2 += 5;
      i2 = (i2+1);
    }
    if (x2 <= 0){ // the currently drawn character was completely out of screen
      //adjust looping values for next loop
      x = x2, i = i2;
    }
  }
  LedSign::Flip(true);
  x--;
  textXCord=x;
  textCharPos=i;
  if (i == strlen(text) && x <= 0){
    currentMode = 0;
  }
}




//Watchdog timeout exeded
ISR(WDT_vect){
  cli();
  wdt_reset();
  
  mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
  Serial.println("ISR(WDT_vect)");
  

  /* Clear WDRF in MCUSR */
  MCUSR &= ~_BV (WDRF);
  /* Write logical one to WDCE and WDE */
  /* Keep old prescaler setting to prevent unintentional time-out */
  WDTCSR |= _BV (_WD_CHANGE_BIT) | _BV (WDE); 
  /* Turn off WDT */
  WDTCSR = 0x00;
  
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

