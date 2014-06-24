#include "heating.h"

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

int lastOutputValueIH = 0;



void Heating::init(){
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
}


void Heating::setHeating(boolean on){
  heating = on;
}

int Heating::getLastOnPWM(){
  return lastOutputValueIH;
}

boolean Heating::isRunning(){
  return startValue == 1;
}
  
void Heating::heatControl(){
    if (heating){
      startValue=digitalRead(StartSeqPin);
      if (startValue==0) {
        start=1; runValue=1;
      } else {
        lastOutputValueIH = outputValueIH;
      }
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
}
