const int PowerFBPin = 12;
const int StartSeqPin = 8;
const int PWMPin = 9;
const int IHStartPin = 10;
const int Increment = 5;
const int pause=100;
int runValue = 1;        
int runRead = 0;        
int outputValue = 50;        
int start = 0;
int startValue = 1;


void setup() {
  pinMode(PowerFBPin,OUTPUT);
  digitalWrite(PowerFBPin,HIGH);
  pinMode(PowerFBPin,INPUT);
  pinMode(StartSeqPin,OUTPUT);
  digitalWrite(StartSeqPin,HIGH);
  pinMode(StartSeqPin,INPUT);
  pinMode(PWMPin,OUTPUT);
  pinMode(IHStartPin,OUTPUT);
  digitalWrite(IHStartPin,LOW);
  Serial.begin(9600); 
}



void loop() {
  startValue=digitalRead(StartSeqPin);
  if (startValue==0) {start=1;runValue=1;}
//  else {start=0;}
  runRead=digitalRead(PowerFBPin);
  if(runRead==0)  {runValue =0; }
  if(runValue==1 && start==1){
  outputValue+=Increment;
  digitalWrite(IHStartPin,HIGH);
  delay(20);
  digitalWrite(IHStartPin,LOW);
  }  
  analogWrite(PWMPin, outputValue);
  // print the results to the serial monitor:
  Serial.print("start = " );                       
  Serial.print(startValue);      
  Serial.print("\t feedback = " );                       
  Serial.print(runRead);      
  Serial.print("\t output = ");      
  Serial.println(outputValue);   
  delay(pause);                     
}


