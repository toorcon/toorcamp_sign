//
//  LexerMicro.ino
//

#include <stdint.h>
#include "computer.h"

int led = 13;

// Arduino Uno: 19200 baud works, 57600 definitely does not.
const int BAUD_RATE = 19200;

// the setup routine runs once when you press reset:
void setup() {
	Serial.begin(BAUD_RATE);

  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);

	computer_init();
}

void test_serial_string(String str)
{
	for (uint8_t i = 0; i < str.length(); i++) {
		computer_serial_input(str.charAt(i));
	}
}

// the loop routine runs over and over again forever:
void loop() {
	// Test!
	test_serial_string("i0\n");
	test_serial_string("c2\n");
	test_serial_string("s!+3.45,5\n");
	test_serial_string("s\"*v!,3\n");
	computer_run();

	delay(5000);
}
