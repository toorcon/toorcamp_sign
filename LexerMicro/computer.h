#ifndef COMPUTER_H
#define COMPUTER_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <OctoWS2811.h>

#define MAX_STEPS          (20)
#define ARG_COUNT          (3)
#define LED_COUNT          (72)

#define DEBUG_STATE        (false)
#define SERIAL_PRINT_RUN   (false)

// Shortcuts for operator functions
#define f0                 (f(0))
#define f1                 (f(1))
#define f2                 (f(2))

#define true_f             (1.0f)
#define false_f            (0.0f)

#define lerp(a,b,v)    ((a) + ((b) - (a)) * (v))

#ifndef randf
#define randf()    (random(0xffffff) * (1.0f / 0xffffff))
#endif

typedef struct arg {
	union {
		float f;
		float * fp;
	};
	bool is_fp;
} Arg;

// Reference machine (virtual computer instructions)
float (*ops[MAX_STEPS])();
Arg args[MAX_STEPS][ARG_COUNT];
float values[MAX_STEPS];	// Computed values

// Incoming data
void (*serial_fp)(uint8_t);
int8_t step_idx = 0;
Arg * current_arg = NULL;
float float_dec = 1.0f;
uint8_t buf[2];

// Global vars, received as bytes over serial
uint8_t station_id = 0;
uint8_t step_count = 0;
OctoWS2811 * _leds;

// Special vars, floats set at runtime
unsigned long lastMillis = 0;
float vTime = 0.0f;	// in seconds
float vStationID = 0.0f;
float vLEDIndex = 0.0f;
float vLEDRatio = 0.0f;
float vLEDCount = 60.0f;
float vGlobalX = 0.0f;
float vGlobalY = 0.0f;
float vStationX = 0.0f;
float vLEDAngle = 0.0f;
float vGlobalAngle = 0.0f;
float vStationAngle = 0.0f;
float vInside = 0.0f;
float vOutside = 1.0f;
uint16_t computeLED = 0;

//
//  OPERATIONS
//

Arg * compute_arg0 = NULL;

inline float f(uint8_t idx)
{
	Arg * arg = &compute_arg0[idx];
	return (arg->is_fp) ? (*(arg->fp)) : (arg->f);
}

float op_add() { return f0 + f1; }
float op_subtract() { return f0 - f1; }
float op_multiply() { return f0 * f1; }
float op_divide() { return f0 / f1; }

float op_modulus() {
	//return fmodf(f0, f1); // No. Behaves unintuitively with negatie values.
	float cachef0 = f0;
	float cachef1 = f1;
	float wholes = floorf(cachef0 / cachef1);
	return cachef0 - (wholes * cachef1);
}

float op_gt() { return (f0 > f1) ? true_f : false_f; }
float op_gte() { return (f0 >= f1) ? true_f : false_f; }
float op_lt() { return (f0 < f1) ? true_f : false_f; }
float op_lte() { return (f0 <= f1) ? true_f : false_f; }
float op_equal() { return (f0 == f1) ? true_f : false_f; }
float op_notequal() { return (f0 != f1) ? true_f : false_f; }
float op_ternary() { return (f0 != 0.0f) ? f1 : f2; }

// Functions

// Teensy LC: The __f versions of trig functions "should" be faster... right?
//            But they're definitely performing slower, for me.
float op_sin() { return sin(f0); }
float op_cos() { return cos(f0); }
float op_tan() { return tan(f0); }
float op_pow() { return pow(f0, f1); }
float op_abs() { return fabs(f0); }
float op_atan2() { return atan2(f0, f1); }
float op_floor() { return floor(f0); }
float op_ceil() { return ceil(f0); }
float op_round() { return round(f0); }
float op_frac() { float cachef0 = f0; return cachef0 - floor(cachef0); }
float op_sqrt() { return sqrt(f0); }
float op_log() { return log(f0); }
float op_logBase() { return log(f0) / log(f1); }
float op_rand() { return randf(); }
float op_randRange() { float cachef0 = f0; return cachef0 + (f1 - cachef0) * randf(); }
float op_min() { return min(f0, f1); }
float op_max() { return max(f0, f1); }
float op_lerp() { float cachef0 = f0; return cachef0 + (f1 - cachef0) * f2; }
float op_clamp() { return constrain(f0, f1, f2); }

float op_tri() { 	// Triangle wave oscillator
	float cachef0 = f0;
	float r = cachef0 - floor(cachef0);	// remainder
	return ((r < 0.5f) ? (r) : (1.0f - r)) * 2.0f;
}

float op_uni2bi() { return (f0 * 2.0f) - 1.0f; }	// unipolar to bipolar
float op_bi2uni() { return (f0 + 1.0f) * 0.5f; }	// bipolar to unipolar

float op_rgb() {
	uint8_t r = constrain((int16_t)(f0 * 0xff), 0x0, 0xff);
	uint8_t g = constrain((int16_t)(f1 * 0xff), 0x0, 0xff);
	uint8_t b = constrain((int16_t)(f2 * 0xff), 0x0, 0xff);

	// FIXME: Gamma correction?

	_leds->setPixel(computeLED, r, g, b);

	return true_f;
}

float op_hsv() {
	return false_f;	// FIXME
}

//
//  SERIAL INPUT
//

void serial_line_start(uint8_t x);
void serial_read_station_id(uint8_t x);
void serial_read_step_count(uint8_t x);
void serial_read_step_number(uint8_t x);
void serial_read_op(uint8_t x);
void serial_arg_start(uint8_t x);
void serial_arg_read_float(uint8_t x);
void serial_arg_read_buf(uint8_t x);
void serial_arg_complete(uint8_t x);
void serial_error();
void serial_wait_for_newline(uint8_t x);

void serial_line_start(uint8_t x)
{
	switch (x) {
		case 'i':
		{
			// Station index
			serial_fp = serial_read_station_id;
		}
		break;

		case 'c':
		{
			// Count: number of steps
			serial_fp = serial_read_step_count;
		}
		break;

		case 's':
		{
			// Step instruction
			serial_fp = serial_read_step_number;
		}
		break;

		default:
		{
			serial_error();
		}
		break;
	}
}

void serial_read_station_id(uint8_t x) {
	station_id = x - '!';
	vStationID = (float)station_id;
	serial_fp = serial_wait_for_newline;
}

void serial_read_step_count(uint8_t x) {
	step_count = x - '!';
	serial_fp = serial_wait_for_newline;
}

void serial_read_step_number(uint8_t x) {
	if ((x < '!') || (('!' + MAX_STEPS) <= x)) {
		step_idx = -1;
		serial_error();
		return;
	}

	step_idx = x - '!';
	serial_fp = serial_read_op;

	// Clear args
	for (uint8_t i = 0; i < ARG_COUNT; i++) {
		args[step_idx][i].f = 0.0f;
		args[step_idx][i].is_fp = false;
	}

	if (DEBUG_STATE) {
		Serial.print("step: ");
		Serial.println(step_idx);
	}
}

void serial_read_op(uint8_t x)
{
	if (DEBUG_STATE) {
		Serial.print("  op: ");
		Serial.println((char)x);
	}

	switch (x) {
		// Operators
		case '+': {ops[step_idx] = op_add; break;}
		case '-': {ops[step_idx] = op_subtract; break;}
		case '*': {ops[step_idx] = op_multiply; break;}
		case '/': {ops[step_idx] = op_divide; break;}
		case '%': {ops[step_idx] = op_modulus; break;}
		case '<': {ops[step_idx] = op_lt; break;}
		case '{': {ops[step_idx] = op_lte; break;}
		case '>': {ops[step_idx] = op_gt; break;}
		case '}': {ops[step_idx] = op_gte; break;}
		case '=': {ops[step_idx] = op_equal; break;}
		case '!': {ops[step_idx] = op_notequal; break;}
		case '?': {ops[step_idx] = op_ternary; break;}

		// Functions
		case 'S': {ops[step_idx] = op_sin; break;}
		case 'C': {ops[step_idx] = op_cos; break;}
		case 'T': {ops[step_idx] = op_tan; break;}
		case 'P': {ops[step_idx] = op_pow; break;}
		case 'a': {ops[step_idx] = op_abs; break;}
		case '2': {ops[step_idx] = op_atan2; break;}
		case 'f': {ops[step_idx] = op_floor; break;}
		case 'c': {ops[step_idx] = op_ceil; break;}
		case 'r': {ops[step_idx] = op_round; break;}
		case '.': {ops[step_idx] = op_frac; break;}
		case 'Q': {ops[step_idx] = op_sqrt; break;}
		case 'L': {ops[step_idx] = op_log; break;}
		case 'B': {ops[step_idx] = op_logBase; break;}
		case 'z': {ops[step_idx] = op_rand; break;}
		case 'Z': {ops[step_idx] = op_randRange; break;}
		case 'm': {ops[step_idx] = op_min; break;}
		case 'M': {ops[step_idx] = op_max; break;}
		case 'p': {ops[step_idx] = op_lerp; break;}
		case 'x': {ops[step_idx] = op_clamp; break;}
		case '3': {ops[step_idx] = op_tri; break;}
		case 'b': {ops[step_idx] = op_uni2bi; break;}
		case 'u': {ops[step_idx] = op_bi2uni; break;}
		case '[': {ops[step_idx] = op_rgb; break;}
		case ']': {ops[step_idx] = op_hsv; break;}
	}

	current_arg = &args[step_idx][0];
	serial_fp = serial_arg_start;
}

void serial_arg_start(uint8_t x)
{
	if (x == '\n') {
		serial_fp = serial_line_start;
		return;
	}

	// Numeric?
	if (('0' <= x) && (x <= '9')) {
		current_arg->is_fp = false;
		current_arg->f = (float)(x - '0');
		serial_fp = serial_arg_read_float;
		float_dec = 1.0f;	// Reading the whole part of the number
		return;
	}

	buf[0] = x;
	serial_fp = serial_arg_read_buf;
}

void prepare_for_next_argument()
{
	current_arg++;
	serial_fp = serial_arg_start;
}

void serial_arg_read_float(uint8_t x)
{
	if ((x == '\n') || (x == ',')) {
		if (DEBUG_STATE) {
			Serial.print("  float: ");
			Serial.println(current_arg->f);
		}

		if (x == '\n') {
			serial_fp = serial_line_start;
		} else {
			prepare_for_next_argument();
		}

		return;
	}

	// Start decimal part?
	if (x == '.') {
		float_dec = 0.1f;
		return;
	}

	if (float_dec == 1.0f) {
		current_arg->f = (current_arg->f * 10.0f) + (float)(x - '0');

	} else {
		current_arg->f += float_dec * (float)(x - '0');
		float_dec *= 0.1f;
	}
}

void serial_arg_read_buf(uint8_t x)
{
	buf[1] = x;

	switch (buf[0]) {
		// Point to a computed value (from a previous step)
		case 'v': {current_arg->fp = &values[x - '!']; break;}
		case 'T': {current_arg->fp = &vTime; break;}
		case 'S': {current_arg->fp = &vStationID; break;}

		case 'I': {
			if (buf[1] == '_') {
				current_arg->fp = &vLEDIndex;
			} else if (buf[1] == 'N') {
				current_arg->fp = &vInside;
			} else {
				serial_error();
				return;
			}
			break;
		}

		case 'C': {current_arg->fp = &vLEDCount; break;}
		case 'P': {current_arg->fp = &vLEDRatio; break;}
		case 'X': {current_arg->fp = &vGlobalX; break;}
		case 'Y': {current_arg->fp = &vGlobalY; break;}

		case 'L': {
			if (buf[1] == 'X') {
				current_arg->fp = &vStationX;
			} else if (buf[1] == 'A') {
				current_arg->fp = &vStationAngle;
			} else {
				serial_error();
				return;
			}
			break;
		}

		case 'A': {current_arg->fp = &vLEDAngle; break;}
		case 'G': {current_arg->fp = &vGlobalAngle; break;}
		case 'O': {current_arg->fp = &vOutside; break;}

		default: {serial_error(); return;}
	}

	current_arg->is_fp = true;
	serial_fp = serial_arg_complete;
}

void serial_arg_complete(uint8_t x)
{
	if (DEBUG_STATE) {
		Serial.print("  fp: ");
		Serial.print((char)buf[0]);
		Serial.println((char)buf[1]);
	}

	if (x == '\n') {
		serial_fp = serial_line_start;

	} else if (x == ',') {
		prepare_for_next_argument();

	} else {
		serial_error();
	}
}

void serial_error() {
	serial_fp = serial_wait_for_newline;
}

void serial_wait_for_newline(uint8_t x)
{
	if (x == '\n') {
		serial_fp = serial_line_start;
	}
}

//
//  INIT, INPUT
//

void computer_init(OctoWS2811 * inLEDs) {
	serial_fp = serial_line_start;
	_leds = inLEDs;
}

void computer_serial_input(uint8_t x)
{
	serial_fp(x);
}

void computer_run()
{
	unsigned long m = millis();
	vTime += (uint16_t)(m - lastMillis) * (1.0f / 1000.0f);
	lastMillis = m;

	vLEDIndex = 0.0f;
	vLEDRatio = 0.0f;
	float ratioInc = 1.0f / vLEDCount;
	vGlobalX = 0.0f;

	for (computeLED = 0; computeLED < LED_COUNT; computeLED++) {

		for (uint8_t s = 0; s < step_count; s++) {
			compute_arg0 = &args[s][0];
			values[s] = ops[s]();

			if (SERIAL_PRINT_RUN) {
				Serial.print(s);
				Serial.print(": ");
				Serial.println(values[s]);
			}

			if (DEBUG_STATE) {
				Serial.print("ran ");
				Serial.print(s);
				Serial.print(": ");
				Serial.println(values[s]);
			}
		}	// !for each step

		// Advance the varying floats
		vLEDIndex += 1.0f;
		vLEDRatio += ratioInc;
		vGlobalX = vLEDIndex;
	}	// !for each LED

	if (DEBUG_STATE) {
		Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~");
	}

}

#endif
