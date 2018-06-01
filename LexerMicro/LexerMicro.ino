//
//  LexerMicro.ino
//

#include <stdint.h>
#include <Adafruit_NeoPixel.h>
#include "computer.h"

int led = 13;

int DATA_PIN = 2;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(72, DATA_PIN, NEO_GRB + NEO_KHZ800);

// Arduino Uno: 19200 baud works, 57600 definitely does not.
const int BAUD_RATE = 9600;

// the setup routine runs once when you press reset:
void setup() {
	Serial.begin(BAUD_RATE);

	// initialize the digital pin as an output.
	pinMode(led, OUTPUT);

	strip.begin();
	strip.show();

	computer_init(&strip);
	//computer_init(NULL);
}

void test_serial_string(String str)
{
	for (uint8_t i = 0; i < str.length(); i++) {
		computer_serial_input(str.charAt(i));
	}
}

// the loop routine runs over and over again forever:
void loop() {
	digitalWrite(led, HIGH);

	strip.show();

	// Test!
	/*
	test_serial_string("i!\n");
	test_serial_string("s!+3.45,5\n");
	test_serial_string("s\"*v!,3\n");
	test_serial_string("c#\n");
	*/


	// led = rgb(0.1 * sin(I), 0, 0);
	/*
	test_serial_string("i!\n");
	test_serial_string("s!SI_\n");
	test_serial_string("s\"*0.1,v!\n");
	test_serial_string("s#[v\"\n");
	test_serial_string("c$\n");
	*/

	while (Serial.available() > 0) {
		// read the incoming byte:
		uint8_t b = Serial.read();
		computer_serial_input(b);
	}

	computer_run();

	digitalWrite(led, LOW);

	//delay(2000);
	//delay(100);
	//delay(1);
}
