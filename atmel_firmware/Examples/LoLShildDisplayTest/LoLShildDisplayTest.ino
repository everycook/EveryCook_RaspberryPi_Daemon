#include <Charliplexing.h>
void setup() {
  // start serial port at 9600 bps:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  LedSign::Init();
}

uint8_t globCycle = 0;
uint16_t globPins = 0;

void loop() {
    for (int8_t x=0;x<DISPLAY_COLS; x++) {
        for (int8_t y=0; y<DISPLAY_ROWS;y++) {
            LedSign::Set(x, y, 1);
            delay(100);
            LedSign::Set(x, y, 0);
            
            if (Serial.available() > 0) {
                uint8_t inByte = Serial.read();
                if (inByte == 'E') {
                }
            }
	}
    }
}
