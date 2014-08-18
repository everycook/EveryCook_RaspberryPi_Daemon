#include "motor.h"



boolean motorStop = false;
boolean motorStoped = false;
uint8_t motorSpeed = 0;



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


void Motor::init(){
  pinMode(outPin,OUTPUT);
  pinMode(analogInPin,INPUT);
  pinMode(sensorPin,INPUT_PULLUP);
}

          
void Motor::setMotor(uint8_t speed){
  motorSpeed = speed;
  motorStop = (motorSpeed == 0);
  if (motorStop){
    outputValueMotor=min(outputValueMotor,255/6);
    setValue=0;
  } else {
    outputValueMotor=255/1;
    setValue=1;
  }
}

boolean Motor::isStopped(){
  return motorStoped;
}

void Motor::motorControl() {
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
        }           
        analogValue = analogValue/AnalogAverage;
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

}
