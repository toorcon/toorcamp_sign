//
//  LexerMicro.ino
//

#include <stdint.h>
#include <OctoWS2811.h>
#include "computer.h"

#define SERIAL_FORMAT  (SERIAL_8N1)
// Downstream: Down to slave microcontrollers
#define DOWNSTREAM     Serial2
#define DOWN_TX        (10)
#define DOWN_RX        (9)
// Upstream: Back towards the laptop / programmer
#define UPSTREAM       Serial1
#define UP_TX          (1)
#define UP_RX          (0)

int BLINK_PIN = 13;

int LED_STRIP_PIN = 17;	// Teensy LC

const int ledsPerStrip = 72;

DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];

const int config = WS2811_GRB | WS2811_800kHz;

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

// Arduino Uno: 19200 baud works, 57600 definitely does not.
const int BAUD_RATE = 9600;

uint8_t frameCount = 0;

// the setup routine runs once when you press reset:
void setup() {
	Serial.begin(BAUD_RATE);

	DOWNSTREAM.setTX(DOWN_TX);
	DOWNSTREAM.setRX(DOWN_RX);
	DOWNSTREAM.begin(BAUD_RATE, SERIAL_FORMAT);

	UPSTREAM.setTX(UP_TX);
	UPSTREAM.setRX(UP_RX);
	UPSTREAM.begin(BAUD_RATE, SERIAL_FORMAT);

	// initialize the digital pin as an output.
	pinMode(BLINK_PIN, OUTPUT);

	leds.begin();
	leds.show();

	//computer_init(&strip);
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
	leds.show();

	// Test!
	/*
	test_serial_string("i!\n");
	test_serial_string("s!+3.45,5\n");
	test_serial_string("s\"*v!,3\n");
	test_serial_string("c#\n");
	*/

/*
	// From the laptop/programmer
	while (Serial.available() > 0) {
		// read the incoming byte:
		uint8_t b = Serial.read();
		computer_serial_input(b);

		// Pass down the line
		DOWNSTREAM.write(b);
	}

	// Slave microcontroller: Receive bytes from upstream
	while (UPSTREAM.available() > 0) {
		// read the incoming byte:
		uint8_t b = UPSTREAM.read();
		computer_serial_input(b);

		// Pass down the line
		DOWNSTREAM.write(b);
	}

	computer_run();
*/

	/*
	// Hmm.
	DOWNSTREAM.write('H');
	DOWNSTREAM.write('i');
	DOWNSTREAM.write('\n');
	delay(1000);
	*/

	while (UPSTREAM.available() > 0) {
		char b = UPSTREAM.read();
		if (b == '\n') {
			Serial.println(b);
		} else {
			Serial.print(b);
		}
	}

	// Blink to prove we're alive.
	// Blinks once every 60 frames. If blink rate is >1/sec, frame rate is good!
	frameCount = (frameCount + 1) % 60;
	digitalWrite(BLINK_PIN, (frameCount == 0) ? HIGH : LOW);
}
