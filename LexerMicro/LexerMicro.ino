//
//  LexerMicro.ino
//

#include <stdint.h>
#include <OctoWS2811.h>

#define STATION_ID      (0)

#define LEDS_PER_STRIP  (20)
#define LED_COUNT       (LEDS_PER_STRIP * 3)

#include "attract.h"
#include "computer.h"

#define SERIAL_FORMAT   (SERIAL_8N1)
#define USB             Serial
#define RINGSERIAL      Serial1
#define RING_TX         (1)
#define RING_RX         (0)

#define LEVEL_SHIFTER_OE_PIN   (3)

#define ATTRACT_ANIM_MILLIS_MIN    (11000)
#define ATTRACT_ANIM_MILLIS_MAX    (15000)
#define ACTIVE_USER_TIMEOUT_MILLIS (120000)

int BLINK_PIN = 13;
uint8_t frameCount = 0;
uint8_t blinkCount = 0;

int LED_STRIP_PIN = 17;	// Teensy LC
const int ledsPerStrip = 20;
DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];
const int config = WS2811_RBG | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

unsigned long lastMillis = 0;
//uint16_t millisSinceSensor = ULTRASONIC_INTERVAL_MS;

// Arduino Uno: 19200 baud works, 57600 definitely does not.
const int BAUD_RATE = 9600;

int16_t attractIndex = -1;
int32_t millisUntilAttract = ATTRACT_ANIM_MILLIS_MIN;

void run_attract_string(const char * str, const uint8_t lifespan)
{
	bool sendLifespan = true;

	while ((*str) != '\0') {
		if (sendLifespan) {
			computer_input_from_upstream('0' + lifespan);
			sendLifespan = false;
		}

		char ch = (*str);
		if (ch == '\n') {
			sendLifespan = true;
		}

		computer_input_from_upstream(ch);

		// Advance to next character
		str++;
	}
}

// the setup routine runs once when you press reset:
void setup() {
	// USB to computer
	USB.begin(BAUD_RATE);

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

	lastMillis = millis();

	// Default: Run the first attract animation
	run_attract_string(ATTRACT_MODES[0], 0);
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

	unsigned long now = millis();
	uint16_t elapsed = now - lastMillis;
	lastMillis = now;

	// User input: disables attract mode, for a little while
	if (STATION_ID == 0) {
		if ((USB.available() > 0) || (RINGSERIAL.available() > 0)) {
			millisUntilAttract = ACTIVE_USER_TIMEOUT_MILLIS;
		}

		// New attract mode?
		millisUntilAttract -= elapsed;
		if (millisUntilAttract <= 0) {
			millisUntilAttract = lerp(ATTRACT_ANIM_MILLIS_MIN, ATTRACT_ANIM_MILLIS_MAX, randf());

			// FIXME: array length
			attractIndex = (attractIndex + 1) % ATTRACT_MODES_LEN;

			run_attract_string(ATTRACT_MODES[attractIndex], STATION_COUNT - 1);
			//run_attract_string(TEST, 7);
		}
	}

	// From Serial: From the laptop/programmer
	while (USB.available() > 0) {
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

	computer_run(elapsed);

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
