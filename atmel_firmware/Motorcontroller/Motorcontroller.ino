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
int outputValue = 0;        // value output to the PWM (analog out)
int inByte = 0;         // incoming serial byte
int setValue=0;

void setup() {
  // initialize serial communications at 9600 bps:
Serial.begin(9600); 
pinMode(9,OUTPUT);  
pinMode(10,OUTPUT);
pinMode(11,OUTPUT);
pinMode(13,OUTPUT);
pinMode(analogInPin,INPUT);
pinMode(sensorPin,INPUT_PULLUP);
}

void loop() {
  if (Serial.available() > 0) {
    inByte = Serial.read();
  
  if (inByte==97){
  Serial.println("ON" );
  outputValue=255/1;
  setValue=1;
  }
  else if (inByte=98){
  Serial.println("OFF" );
  outputValue=min(outputValue,255/6);
  setValue=0;
  }
  }
//  Serial.print("read = " );                       
//  Serial.print(inByte);      
  
  sensorValue = digitalRead(sensorPin);
  if (sensorValue==0 && LastSensorValue==1)
  {  
  rotationTime=millis()-LastSensorTrigger;
  LastSensorTrigger=millis(); 
  //outputValue=0;
  if (setValue==0 && rotationTime>=slowRotationTime) {
    outputValue=0;
  }
  }
  LastSensorValue=sensorValue;

  analogWrite(outPin, outputValue);           
  analogWrite(13, outputValue);           

  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   
    analogValue=0;
for (int i=0;i<AnalogAverage;i++)
{
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
  Serial.println(outputValue);   
}
}
