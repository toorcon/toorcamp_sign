//
//  LexerMicro.ino
//

#include <stdint.h>

int led = 13;

// Arduino Uno: 19200 works, 57600 definitely does not
const int BAUD_RATE = 19200;

// the setup routine runs once when you press reset:
void setup() {
	Serial.begin(BAUD_RATE);

  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);
}

// the loop routine runs over and over again forever:
void loop() {
	if (Serial.available()) {
		uint8_t in = Serial.read();

		if (in == '1') {
			digitalWrite(led, HIGH);

		} else if (in == '0') {
			digitalWrite(led, LOW);
		}
	}
}
