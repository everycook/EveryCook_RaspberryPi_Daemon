
void setup() {
  // start serial port at 9600 bps:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
}

uint8_t globCycle = 0;
uint16_t globPins = 0;

void loop() {
//  for(int pin=2; pin<=13; pin++){
//    pinMode(pin, OUTPUT);
    for(int pin2=2; pin2<=13; pin2++){
      pinMode(pin2, OUTPUT);
      digitalWrite(pin2, HIGH);
      digitalRead(pin2);
      delay(2000);
      digitalWrite(pin2, LOW);
      pinMode(pin2, INPUT);
    }
//    pinMode(pin, INPUT);
//  }
}
