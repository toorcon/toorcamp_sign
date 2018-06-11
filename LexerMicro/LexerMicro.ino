//
//  LexerMicro.ino
//

#include <stdint.h>
#include <OctoWS2811.h>
#include "computer.h"

#define SERIAL_FORMAT  (SERIAL_8N1)
// Downstream: Down to slave microcontrollers
#define RINGSERIAL     Serial1
#define RING_TX        (1)
#define RING_RX        (0)

#define LEVEL_SHIFTER_OE_PIN   (3)

int BLINK_PIN = 13;
uint8_t frameCount = 0;
uint8_t blinkCount = 0;

int LED_STRIP_PIN = 17;	// Teensy LC
const int ledsPerStrip = 72;
DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

// Arduino Uno: 19200 baud works, 57600 definitely does not.
const int BAUD_RATE = 9600;

// the setup routine runs once when you press reset:
void setup() {
	// USB to computer
	Serial.begin(BAUD_RATE);

	pinMode(LEVEL_SHIFTER_OE_PIN, OUTPUT);
	digitalWrite(LEVEL_SHIFTER_OE_PIN, LOW);

	RINGSERIAL.setTX(RING_TX);
	RINGSERIAL.setRX(RING_RX);
	RINGSERIAL.begin(BAUD_RATE, SERIAL_FORMAT);

	// initialize the digital pin as an output.
	pinMode(BLINK_PIN, OUTPUT);

	leds.begin();
	leds.show();

	computer_init(&leds);
}

void test_serial_string(String str)
{
	for (uint8_t i = 0; i < str.length(); i++) {
		computer_input_from_upstream(str.charAt(i));
	}
}

void serial_input(uint8_t b) {
	LineResult result = computer_input_from_upstream(b);

	//Serial.println(result);

	if ((result == k_line_ok) || (result == k_line_end)) {
		// Pass the data along, downstream.
		RINGSERIAL.write(b);

	} else if (result == k_line_first_byte) {
		if (('1' <= b) && (b <= '9')) {
			// Message lifespan: Decrement and pass onward
			RINGSERIAL.write(b - 1);
		}
	}

	/*
	} else if (result == k_line_set_station_id) {
		// Increment station_id and pass it to next station.
		DOWNSTREAM.write('i');
		DOWNSTREAM.write('0' + computer_get_station_id() + 1);
		DOWNSTREAM.write('\n');
	}
	*/
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

	// From Serial: From the laptop/programmer
	while (Serial.available() > 0) {
		// read the incoming byte:
		uint8_t b = Serial.read();
		serial_input(b);
	}

	// From UPSTREAM: From the microprocessor 1 higher
	while (RINGSERIAL.available() > 0) {
		// read the incoming byte:
		uint8_t b = RINGSERIAL.read();
		serial_input(b);
	}

	// From DOWNSTREAM: other micros can send sensor data
	// upstream.
	/*
	// TODO
	while (UPSTREAM.available() > 0) {
		// read the incoming byte:
		uint8_t b = UPSTREAM.read();
		computer_serial_input(b);

		// Pass down the line
		DOWNSTREAM.write(b);
	}
	*/

	computer_run();

	/*
	// Hmm.
	DOWNSTREAM.write('H');
	DOWNSTREAM.write('i');
	DOWNSTREAM.write('\n');
	delay(1000);
	*/

	/*
	while (UPSTREAM.available() > 0) {
		char b = UPSTREAM.read();
		if (b == '\n') {
			Serial.println(b);
		} else {
			Serial.print(b);
		}
	}
	*/

	// Blink to prove we're alive.
	// Blinks once every 60 frames. If blink rate is >1/sec, frame rate is good!
	frameCount = (frameCount + 1) % 60;
	if (frameCount == 0) blinkCount++;

	BlinkType bt = computer_get_blink_type();

	switch (bt) {
		case k_blink_60th_frame:
		case k_blink_station_id:
		{
			int state = (frameCount == 0) ? HIGH : LOW;

			if ((state == HIGH) && (bt == k_blink_station_id)) {
				state = ((blinkCount & 0x7) <= computer_get_station_id()) ? HIGH : LOW;
			}

			digitalWrite(BLINK_PIN, state);
		}
		break;

		case k_blink_off:
		{
			digitalWrite(BLINK_PIN, LOW);
		}
		break;

		case k_blink_on:
		{
			digitalWrite(BLINK_PIN, HIGH);
		}
		break;
	}
}
