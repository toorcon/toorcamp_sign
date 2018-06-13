//
//  LexerMicro.ino
//

#include <stdint.h>
#include <OctoWS2811.h>

// Forward declaration for computer.h :P
void lexer_station_id_did_change(uint8_t x);

#include "computer.h"

#define SERIAL_FORMAT  (SERIAL_8N1)
// Downstream: Down to slave microcontrollers
#define RINGSERIAL     Serial1
#define RING_TX        (1)
#define RING_RX        (0)

// Timeout: one frame at 30fps ?
// 33333 us -> 574 cm -> 18 feet
#define SENSOR_TIMEOUT_MICROS   (1000000 / 30)
#define SENSOR_CM_MAX           (SENSOR_TIMEOUT_MICROS / 58.0f)
#define TRIG                    (19)
#define ECHO                    (18)
#define ULTRASONIC_INTERVAL_MS  ((SENSOR_TIMEOUT_MICROS * 8) / 1000 + 10)

int BLINK_PIN = 13;
uint8_t frameCount = 0;
uint8_t blinkCount = 0;

int LED_STRIP_PIN = 17;	// Teensy LC
const int ledsPerStrip = 72;
DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

unsigned long lastMillis = 0;
uint16_t millisSinceSensor = ULTRASONIC_INTERVAL_MS;

// Arduino Uno: 19200 baud works, 57600 definitely does not.
const int BAUD_RATE = 9600;

// the setup routine runs once when you press reset:
void setup() {
	// USB to computer
	Serial.begin(BAUD_RATE);

	pinMode(TRIG, OUTPUT);
	pinMode(ECHO, INPUT);

	RINGSERIAL.setTX(RING_TX);
	RINGSERIAL.setRX(RING_RX);
	RINGSERIAL.begin(BAUD_RATE, SERIAL_FORMAT);

	// initialize the digital pin as an output.
	pinMode(BLINK_PIN, OUTPUT);

	leds.begin();
	leds.show();

	computer_init(&leds);

	lastMillis = millis();
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
}

void lexer_station_id_did_change(uint8_t x) {
	// Adjust the ultrasonic timing.
	// Each station should at a different time.
	millisSinceSensor = (x / 8.0f) * ULTRASONIC_INTERVAL_MS;
}

// the loop routine runs over and over again forever:
void loop() {
	leds.show();

	unsigned long m = millis();
	uint16_t elapsed = m - lastMillis;
	lastMillis = m;

	// Ultrasonic sensor
	millisSinceSensor += elapsed;
	if (millisSinceSensor >= ULTRASONIC_INTERVAL_MS) {
		millisSinceSensor -= ULTRASONIC_INTERVAL_MS;

		// Activate TRIG
		digitalWrite(TRIG, HIGH);
		delayMicroseconds(10);
		digitalWrite(TRIG, LOW);

		// Duration for ECHO to be heard (in micros)
		unsigned long duration = pulseIn(ECHO, HIGH, SENSOR_TIMEOUT_MICROS);

		// Range varies a lot! 100==very close. 1294000==far.
		float cm = duration / 58.0f;

		// 1.0==near! 0.0==far.
		computer_set_ultrasonic_cm(1.0f - (cm / SENSOR_CM_MAX));
	}

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
