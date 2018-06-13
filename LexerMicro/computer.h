#ifndef COMPUTER_H
#define COMPUTER_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <OctoWS2811.h>
#include "led_layout.h"

#define MAX_LINE_LEN       (32)
#define MAX_STEPS          (20)
#define ARG_COUNT          (3)
#define STATION_COUNT      (3)
#define LED_COUNT          (72)
#define ACCUMULATOR_COUNT  (1)
#define NOISE_SIZE         (16)

#define DEFAULT_GAMMA      (true)
#define DEFAULT_BRIGHT     (51)

#define DEBUG_STATE        (false)
#define SERIAL_PRINT_RUN   (false)

// Higher == faster.  6.0f==full change over 1/6th second
#define SENSOR_SMOOTHING   (6.0f)

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

typedef enum {
	k_line_ok = 0,
	k_line_first_byte,
	k_line_end,
	k_line_do_not_share
} LineResult;

typedef enum {
	k_blink_off = 0,
	k_blink_on = 1,
	k_blink_60th_frame = 2,
	k_blink_station_id = 3
} BlinkType;

typedef enum {
	k_float,
	k_float_ptr,
	k_array_of_floats
} ArgType;

typedef struct arg {
	union {
		float f;
		float * fp;
	};
	ArgType type;
} Arg;

// Gamma + brightness LUT
uint8_t lut[256];
BlinkType blink_type = k_blink_60th_frame;

// Reference machine (virtual computer instructions)
float (*ops[MAX_STEPS])();
Arg args[MAX_STEPS][ARG_COUNT];
float values[MAX_STEPS];	// Computed values

// Incoming data lines

// Data from upstream, heading down
uint8_t upstreamBuf[MAX_LINE_LEN];
uint16_t upstreamIdx = 0;

/*
// Data from downstream, heading up
uint8_t downstreamBuf[MAX_LINE_LEN];
uint16_t downstreamIdx = 0;
*/

// Incoming data: State machine
void (*serial_fp)(uint8_t);
int8_t step_idx = 0;
Arg * current_arg = NULL;
float float_dec = 1.0f;
uint8_t buf[2];

// Global vars, received as bytes over serial
uint8_t station_id = 0xff;
uint8_t step_count = 0;
OctoWS2811 * _leds;

// LED layout
float led_x[LED_COUNT];
float led_y[LED_COUNT];
float led_local_angle[LED_COUNT];

// Special vars, floats set at runtime
float vTime = 0.0f;	// in seconds
float vStationID = 0.0f;
float vLEDIndex = 0.0f;
float vLEDRatio = 0.0f;
float vLEDCount = 60.0f;
float vUltrasonicCM = 0.0f;
float vUltrasonicCM_dest = 0.0f;
uint16_t computeLED = 0;

//
//  FORWARD DECLARATIONS for state machine
//

void serial_line_start(uint8_t x);
void serial_read_data_type(uint8_t x);
void serial_read_step_count(uint8_t x);
void serial_read_step_number(uint8_t x);
void serial_read_op(uint8_t x);
void serial_arg_start(uint8_t x);
void serial_arg_read_float(uint8_t x);
void serial_arg_read_buf(uint8_t x);
void serial_arg_complete(uint8_t x);
void serial_read_gamma_start(uint8_t x);
void serial_read_gamma_end(uint8_t x);
void serial_read_blink(uint8_t x);
void serial_error();
void serial_wait_for_newline(uint8_t x);

//
// PERSISTENT STATE
//

void set_station_id(uint8_t x) {
	// Do not set if redundant
	if (x == station_id) return;

	station_id = x;
	vStationID = (float)station_id;

	led_layout_set_all(station_id, led_x, led_y, led_local_angle);
}

float accum[ACCUMULATOR_COUNT][LED_COUNT];

void reset_time_and_accumulators() {
	vTime = 0.0f;

	for (uint8_t a = 0; a < ACCUMULATOR_COUNT; a++) {
		for (uint16_t i = 0; i < LED_COUNT; i++) {
			accum[a][i] = 0.0f;
		}
	}
}

float noise[NOISE_SIZE][NOISE_SIZE][NOISE_SIZE];

void reroll_noise() {
	// Noisiest, quietest octave: Just use randf() values.
	for (uint8_t i = 0; i < NOISE_SIZE; i++) {
		for (uint8_t j = 0; j < NOISE_SIZE; j++) {
			for (uint8_t k = 0; k < NOISE_SIZE; k++) {
				noise[i][j][k] = randf();
			}
		}
	}

	// Larger octaves (with interpolated values) get
	// progressively "louder" (stronger magnitude).
	for (uint8_t step = 2; step < NOISE_SIZE; step *= 2) {
		float step_f = (float)(step);
		float invStep = 1.0f / step_f;

		uint8_t sz = NOISE_SIZE / step;
		float table[sz][sz][sz];

		for (uint8_t i = 0; i < sz; i++) {
			for (uint8_t j = 0; j < sz; j++) {
				for (uint8_t k = 0; k < sz; k++) {
					// Lower octaves are "louder" (stronger magnitude)
					table[i][j][k] = randf() * step_f;
				}
			}
		}

		/*
		uint8_t offset_i = random(NOISE_SIZE);
		uint8_t offset_j = random(NOISE_SIZE);
		uint8_t offset_k = random(NOISE_SIZE);
		*/

		// FIXME ? Does 1D and 2D noise look better when
		//         waveforms are snapped to [0,0,0] ?
		uint8_t offset_i = 0;
		uint8_t offset_j = 0;
		uint8_t offset_k = 0;

		// table values are chosen. Interpolate this octave
		// of noise, add onto noise buffer.
		// Get ready for some ugly nested code:
		for (uint8_t i = 0; i < sz; i++) {
			uint8_t i0 = i;
			uint8_t i1 = (i0 + 1) % sz;

			for (uint8_t j = 0; j < sz; j++) {
				uint8_t j0 = j;
				uint8_t j1 = (j0 + 1) % sz;

				for (uint8_t k = 0; k < sz; k++) {
					uint8_t k0 = k;
					uint8_t k1 = (k0 + 1) % sz;

					// Set noise[][][] values
					for (uint8_t iN = 0; iN < step; iN++) {
						float ip = iN * invStep;	// lerp
						uint8_t ic = (i * sz + iN + offset_i) % NOISE_SIZE;	// noise[] index

						// Lerp values across i dimension
						float iv0 = lerp(table[i0][j0][k0], table[i1][j0][k0], ip);
						float iv1 = lerp(table[i0][j1][k0], table[i1][j1][k0], ip);
						float iv2 = lerp(table[i0][j0][k1], table[i1][j0][k1], ip);
						float iv3 = lerp(table[i0][j1][k1], table[i1][j1][k1], ip);

						for (uint8_t jN = 0; jN < step; jN++) {
							float jp = jN * invStep;	// lerp
							uint8_t jc = (j * sz + jN + offset_j) % NOISE_SIZE;	// noise[] index

							// Lerp i-values across the j dimension
							float jv0 = lerp(iv0, iv1, jp);
							float jv1 = lerp(iv2, iv3, jp);

							for (uint8_t kN = 0; kN < step; kN++) {
								float kp = kN * invStep;	// lerp
								uint8_t kc = (k * sz + kN + offset_k) % NOISE_SIZE;	// noise[] index

								// Lerp ij-values across the k dimension
								noise[ic][jc][kc] += lerp(jv0, jv1, kp);
							}
						}
					}

				}
			}
		}
	}

	// Normalize between 0..1
	float _min = 9999.0f;
	float _max = -9999.0f;

	for (uint8_t i = 0; i < NOISE_SIZE; i++) {
		for (uint8_t j = 0; j < NOISE_SIZE; j++) {
			for (uint8_t k = 0; k < NOISE_SIZE; k++) {
				_min = min(_min, noise[i][j][k]);
				_max = max(_max, noise[i][j][k]);
			}
		}
	}

	float mult = 1.0f / (_max - _min);

	for (uint8_t i = 0; i < NOISE_SIZE; i++) {
		for (uint8_t j = 0; j < NOISE_SIZE; j++) {
			for (uint8_t k = 0; k < NOISE_SIZE; k++) {
				noise[i][j][k] = (noise[i][j][k] - _min) * mult;
			}
		}
	}

}

//
//  OPERATIONS
//

Arg * compute_arg0 = NULL;

inline float f(uint8_t idx)
{
	Arg * arg = &compute_arg0[idx];

	if (arg->type == k_float_ptr) {
		return (*(arg->fp));

	} else if (arg->type == k_array_of_floats) {
		return arg->fp[computeLED];
	}

	return arg->f;
}

float op_add() { return f0 + f1; }
float op_subtract() { return f0 - f1; }
float op_multiply() { return f0 * f1; }
float op_divide() { return f0 / f1; }

// Modulus ? Remainder ?
// My implementation of '%' expects a positive divisor (f1),
// and returns _positive_ values when dividend (f0)
// is negative, contiguous with the sawtooth-shaped output
// when dividend is positive.   Example: -1 % 3 = 2.
// Deal with it  [*sunglasses*]

float op_mod() {
	float v = f0;	// cache
	float window = f1;	// cache
	return v - floor(v / window) * window;
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
float op_sin01() { return (sin(f0 * (float)(M_PI * 2.0f)) + 1.0f) * 0.5f; }
float op_cos01() { return (cos(f0 * (float)(M_PI * 2.0f)) + 1.0f) * 0.5f; }

// Sine-quad, cos-quad (or maybe the 'q' stands for 'quick')
inline float _sinq(float v) {
	float dec = v - floor(v);	// decimal part. wrap within 0..1

	if (dec >= 0.5f) {
		dec -= 0.75f;
		return dec * dec * 8.0f;
	}

	dec -= 0.25f;
	return 1.0f - (dec * dec * 8.0f);
}

float op_sinq() { return _sinq(f0); }
float op_cosq() { return _sinq(f0 - 0.25f); }

float op_tan() { return tan(f0); }
float op_pow() { return pow(f0, f1); }
float op_abs() { return abs(f0); }
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

float op_noise1() {
	float c0 = f0;	// cache

	float xf = (c0 - floor(c0)) * NOISE_SIZE;	// wrap in 0..NOISE_SIZE

	// noise[] lookups
	uint8_t x0 = (uint8_t)(xf);
	uint8_t x1 = (x0 + 1) % NOISE_SIZE;
	float xp = xf - floor(xf);

	return lerp(noise[x0][0][0], noise[x1][0][0], xp);
}

float op_noise2() {
	// cache
	float c0 = f0;
	float c1 = f1;

	// wrap in 0..NOISE_SIZE
	float xf = (c0 - floor(c0)) * NOISE_SIZE;
	float yf = (c1 - floor(c1)) * NOISE_SIZE;

	// noise[] lookups
	uint8_t x0 = (uint8_t)(xf);
	uint8_t x1 = (x0 + 1) % NOISE_SIZE;
	float xp = xf - floor(xf);
	uint8_t y0 = (uint8_t)(yf);
	uint8_t y1 = (y0 + 1) % NOISE_SIZE;
	float yp = yf - floor(yf);

	float xv0 = lerp(noise[x0][y0][0], noise[x1][y0][0], xp);
	float xv1 = lerp(noise[x0][y1][0], noise[x1][y1][0], xp);

	return lerp(xv0, xv1, yp);
}

float op_noise3() {
	// cache
	float c0 = f0;
	float c1 = f1;
	float c2 = f2;

	// wrap in 0..NOISE_SIZE
	float xf = (c0 - floor(c0)) * NOISE_SIZE;
	float yf = (c1 - floor(c1)) * NOISE_SIZE;
	float zf = (c2 - floor(c2)) * NOISE_SIZE;

	// noise[] lookups
	uint8_t x0 = (uint8_t)(xf);
	uint8_t x1 = (x0 + 1) % NOISE_SIZE;
	float xp = xf - floor(xf);
	uint8_t y0 = (uint8_t)(yf);
	uint8_t y1 = (y0 + 1) % NOISE_SIZE;
	float yp = yf - floor(yf);
	uint8_t z0 = (uint8_t)(zf);
	uint8_t z1 = (z0 + 1) % NOISE_SIZE;
	float zp = zf - floor(zf);

	float xv0 = lerp(noise[x0][y0][z0], noise[x1][y0][z0], xp);
	float xv1 = lerp(noise[x0][y1][z0], noise[x1][y1][z0], xp);
	float xv2 = lerp(noise[x0][y0][z1], noise[x1][y0][z1], xp);
	float xv3 = lerp(noise[x0][y1][z1], noise[x1][y1][z1], xp);

	float yv0 = lerp(xv0, xv1, yp);
	float yv1 = lerp(xv2, xv3, yp);

	return lerp(yv0, yv1, zp);
}

float op_noise1q() {
	float c0 = f0;	// cache

	// wrap in 0..NOISE_SIZE
	uint8_t x = (uint8_t)((c0 - floor(c0)) * NOISE_SIZE);

	return noise[x][0][0];
}

float op_noise2q() {
	// cache
	float c0 = f0;
	float c1 = f1;

	// wrap in 0..NOISE_SIZE
	uint8_t x = (uint8_t)((c0 - floor(c0)) * NOISE_SIZE);
	uint8_t y = (uint8_t)((c1 - floor(c1)) * NOISE_SIZE);

	return noise[x][y][0];
}

float op_noise3q() {
	// cache
	float c0 = f0;
	float c1 = f1;
	float c2 = f2;

	// wrap in 0..NOISE_SIZE
	uint8_t x = (uint8_t)((c0 - floor(c0)) * NOISE_SIZE);
	uint8_t y = (uint8_t)((c1 - floor(c1)) * NOISE_SIZE);
	uint8_t z = (uint8_t)((c2 - floor(c2)) * NOISE_SIZE);

	return noise[x][y][z];
}

float op_min() { return min(f0, f1); }
float op_max() { return max(f0, f1); }
float op_lerp() { float cachef0 = f0; return cachef0 + (f1 - cachef0) * f2; }
float op_clamp() { return constrain(f0, f1, f2); }

float op_tri() { 	// Triangle wave oscillator
	float cachef0 = f0;
	float r = cachef0 - floor(cachef0);	// remainder
	return ((r < 0.5f) ? (r) : (1.0f - r)) * 2.0f;
}

// 0..1..0 around the origin, all other values are 0
float op_peak() {
	return max(0.0f, 1.0f - abs(f0));
}

float op_uni2bi() { return (f0 * 2.0f) - 1.0f; }	// unipolar to bipolar
float op_bi2uni() { return (f0 + 1.0f) * 0.5f; }	// bipolar to unipolar

float op_accum0() {
	accum[0][computeLED] += f0;
	return accum[0][computeLED];
}

float op_rgb() {
	uint8_t r = constrain((int16_t)(f0 * 0xff), 0x0, 0xff);
	uint8_t g = constrain((int16_t)(f1 * 0xff), 0x0, 0xff);
	uint8_t b = constrain((int16_t)(f2 * 0xff), 0x0, 0xff);

	_leds->setPixel(computeLED, lut[r], lut[g], lut[b]);

	return true_f;
}

float op_hsv() {
	float h = f0;
	//float s = f1;
	float v = f2;	// top (max) value
	float p = v * (1.0f - f1);	// bottom (min) value

	uint8_t v8 = constrain((int16_t)(v * 0xff), 0x0, 0xff);
	uint8_t p8 = constrain((int16_t)(p * 0xff), 0x0, 0xff);

	h -= floor(h);	// wrap inside 0..1
	float h6 = h * 6.0f;
	float hRamp = h6 - floor(h6);

	uint8_t h6i = (uint8_t)(h6);	// 0..5

	if (h6i & 0x1) {	// Odds
		// "Falling" (yellow -> green: R falls, etc)
		uint8_t q8 = lerp(v8, p8, hRamp);

		if (h6i == 1) {
			_leds->setPixel(computeLED, lut[q8], lut[v8], lut[p8]);	// yellow -> green
		} else if (h6i == 3) {
			_leds->setPixel(computeLED, lut[p8], lut[q8], lut[v8]);	// cyan -> blue
		} else {
			_leds->setPixel(computeLED, lut[v8], lut[p8], lut[q8]);	// magenta -> red
		}

	} else {	// Evens
		// "Rising" (red -> yellow: G rises, etc)
		uint8_t t8 = lerp(p8, v8, hRamp);

		if (h6i == 0) {
			_leds->setPixel(computeLED, lut[v8], lut[t8], lut[p8]);	// red -> yellow
		} else if (h6i == 2) {
			_leds->setPixel(computeLED, lut[p8], lut[v8], lut[t8]);	// green -> cyan
		} else {
			_leds->setPixel(computeLED, lut[t8], lut[p8], lut[v8]);	// blue -> magenta
		}
	}

	return true_f;
}

uint8_t computer_get_station_id() {
	return station_id;
}

void set_gamma_and_brightness(bool isGamma, uint8_t bright) {
	float raw = 0.0f;

	for (uint16_t i = 0; i < 256; i++) {
		float v = raw;

		if (isGamma) {
			//v = pow(v, 2.2f);	// Slow.
			//v = powf(v, 2.2f);	// Slow also.

			// Fast! TODO: How does it look?
			float v2 = raw * raw;
			float v3 = raw * v2;
			v = lerp(v2, v3, 0.21f);	// Close enough to pow(v, 2.2)!
		}

		lut[i] = (uint8_t)(round(v * bright));

		raw += (1.0f / 0xff);
	}
}

//
//  SERIAL INPUT
//

void serial_line_start(uint8_t x)
{
	// x must be between '0' and '7'
	if ((x < '0') || ('7' < x)) {
		serial_error();
		return;
	}

	serial_fp = serial_read_data_type;
}

void serial_read_data_type(uint8_t x)
{
	switch (x) {
		// Count: number of steps
		case 'c':
		{
			serial_fp = serial_read_step_count;
		}
		break;

		// Step instruction
		case 's':
		{
			serial_fp = serial_read_step_number;
		}
		break;

		// Reset time
		case 't':
		{
			reset_time_and_accumulators();
			serial_fp = serial_wait_for_newline;
		}
		break;

		// Gamma/brightness
		case 'g':
		{
			serial_fp = serial_read_gamma_start;
		}
		break;

		// Station ID
		case 'i':
		{
			// Infer station_if from buf[0]
			uint8_t new_id = (STATION_COUNT - 1) - (upstreamBuf[0] - '0');

			set_station_id(new_id);

			serial_fp = serial_wait_for_newline;
		}
		break;

		case 'b':
		{
			serial_fp = serial_read_blink;
		}
		break;

		default:
		{
			serial_error();
		}
		break;
	}
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
		args[step_idx][i].type = k_float;
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
		case '%': {ops[step_idx] = op_mod; break;}
		case '<': {ops[step_idx] = op_lt; break;}
		case '>': {ops[step_idx] = op_gt; break;}
		case '{': {ops[step_idx] = op_lte; break;}
		case '}': {ops[step_idx] = op_gte; break;}
		case '=': {ops[step_idx] = op_equal; break;}
		case '!': {ops[step_idx] = op_notequal; break;}
		case '?': {ops[step_idx] = op_ternary; break;}

		// Functions
		case 'S': {ops[step_idx] = op_sin; break;}
		case 'C': {ops[step_idx] = op_cos; break;}
		case 's': {ops[step_idx] = op_sin01; break;}
		case 'c': {ops[step_idx] = op_cos01; break;}
		case 'q': {ops[step_idx] = op_sinq; break;}
		case 'Q': {ops[step_idx] = op_cosq; break;}
		case 'T': {ops[step_idx] = op_tan; break;}
		case 'P': {ops[step_idx] = op_pow; break;}
		case '|': {ops[step_idx] = op_abs; break;}
		case 'A': {ops[step_idx] = op_atan2; break;}
		case '_': {ops[step_idx] = op_floor; break;}
		case '`': {ops[step_idx] = op_ceil; break;}
		case 'r': {ops[step_idx] = op_round; break;}
		case '.': {ops[step_idx] = op_frac; break;}
		case 'R': {ops[step_idx] = op_sqrt; break;}
		case 'L': {ops[step_idx] = op_log; break;}
		case 'B': {ops[step_idx] = op_logBase; break;}
		case 'z': {ops[step_idx] = op_rand; break;}
		case 'Z': {ops[step_idx] = op_randRange; break;}
		case '1': {ops[step_idx] = op_noise1; break;}
		case '2': {ops[step_idx] = op_noise2; break;}
		case '3': {ops[step_idx] = op_noise3; break;}
		case '4': {ops[step_idx] = op_noise1q; break;}
		case '5': {ops[step_idx] = op_noise2q; break;}
		case '6': {ops[step_idx] = op_noise3q; break;}
		case 'm': {ops[step_idx] = op_min; break;}
		case 'M': {ops[step_idx] = op_max; break;}
		case 'l': {ops[step_idx] = op_lerp; break;}
		case 'x': {ops[step_idx] = op_clamp; break;}
		case 't': {ops[step_idx] = op_tri; break;}
		case 'p': {ops[step_idx] = op_peak; break;}
		case 'b': {ops[step_idx] = op_uni2bi; break;}
		case 'u': {ops[step_idx] = op_bi2uni; break;}
		case '0': {ops[step_idx] = op_accum0; break;}
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
		current_arg->type = k_float;
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
		case 'v': {
			current_arg->fp = &values[x - '!'];
			current_arg->type = k_float_ptr;
		}
		break;

		case 'T': {
			current_arg->fp = &vTime;
			current_arg->type = k_float_ptr;
		}
		break;

		case 'S': {
			current_arg->fp = &vStationID;
			current_arg->type = k_float_ptr;
		}
		break;

		case 'I': {
			if (buf[1] == '_') {
				current_arg->fp = &vLEDIndex;
				current_arg->type = k_float_ptr;

				/*
			} else if (buf[1] == 'N') {
				current_arg->fp = &vInside;
				current_arg->type = k_float_ptr;
				*/

			} else {
				serial_error();
				return;
			}
		}
		break;

		case 'C': {
			current_arg->fp = &vLEDCount;
			current_arg->type = k_float_ptr;
		}
		break;

		case 'P': {
			current_arg->fp = &vLEDRatio;
			current_arg->type = k_float_ptr;
		}
		break;

		case 'X': {
			current_arg->fp = led_x;
			current_arg->type = k_array_of_floats;
		}
		break;

		case 'Y': {
			current_arg->fp = led_y;
			current_arg->type = k_array_of_floats;
		}
		break;

		case 'A': {
			current_arg->fp = led_local_angle;
			current_arg->type = k_array_of_floats;
		}
		break;

		case 'U': {
			current_arg->fp = &vUltrasonicCM;
			current_arg->type = k_float_ptr;
		}
		break;

		/*
		case 'L': {
			if (buf[1] == 'X') {
				current_arg->fp = led_x;
				current_arg->type = k_array_of_floats;

			} else if (buf[1] == 'A') {
				current_arg->fp = led_local_angle;
				current_arg->type = k_array_of_floats;

			} else {
				serial_error();
				return;
			}
			break;
		}
		*/

		//case 'G': {current_arg->fp = &vGlobalAngle; break;}
		//case 'O': {current_arg->fp = &vOutside; break;}

		default: {serial_error(); return;}
	}

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

// Gamma + brightness:
// 0bx1xxxGBB 0bx1BBBBBB
void serial_read_gamma_start(uint8_t x) {
	if (x == '\n') {
		serial_fp = serial_line_start;
		return;
	}

	buf[0] = x;
	serial_fp = serial_read_gamma_end;
}

void serial_read_blink(uint8_t x) {
	if (x == '\n') {
		serial_fp = serial_line_start;
		return;
	}

	switch (x) {
		case '0': {blink_type = k_blink_off;} break;
		case '1': {blink_type = k_blink_on;} break;
		case '2': {blink_type = k_blink_60th_frame;} break;
		case '3': {blink_type = k_blink_station_id;} break;
	}

	serial_fp = serial_wait_for_newline;
}

void serial_read_gamma_end(uint8_t x) {
	if (x == '\n') {
		serial_fp = serial_line_start;
		return;
	}

	buf[1] = x;

	bool isGamma = buf[0] & 0b100;
	uint8_t bright = ((buf[0] & 0x03) << 6) | (buf[1] & 0x3f);
	//float bright_f = bright * (1.0f / 0xff);

	set_gamma_and_brightness(isGamma, bright);

	serial_fp = serial_wait_for_newline;
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
	randomSeed(1337);
	reset_time_and_accumulators();
	reroll_noise();
	set_gamma_and_brightness(DEFAULT_GAMMA, DEFAULT_BRIGHT);
	serial_fp = serial_line_start;
	_leds = inLEDs;
}

LineResult _input_from_stream(uint8_t * buf, uint16_t * idx, uint8_t c) {
	LineResult result = k_line_ok;

	// Room to store this?
	if ((*idx) < MAX_LINE_LEN) {
		buf[*idx] = c;
	}

	if ((*idx) == 0) {
		result = k_line_first_byte;
	}

	// Advance to next character (line length is longer)
	(*idx)++;

	// End of line?
	if (c == '\n') {

		// Finish a shareable message (lifespan is still valid)
		if (buf[0] != '0') {
			result = k_line_end;
		}

		// Valid line length?
		if ((*idx) <= MAX_LINE_LEN) {

			// Process this line, one byte at a time
			for (uint8_t i = 0; i < (*idx); i++) {
				// State machine (function pointer)
				serial_fp(buf[i]);
			}
		}
	}

	// Do not share messages with expired lifespans.
	if (buf[0] == '0') {
		result = k_line_do_not_share;
	}

	return result;
}

LineResult computer_input_from_upstream(uint8_t x)
{
	LineResult result = _input_from_stream(upstreamBuf, &upstreamIdx, x);

	if (result == k_line_end) {
		upstreamIdx = 0;
	}

	return result;
}

/*
LineResult computer_input_from_downstream(uint8_t x)
{
	LineResult result = _input_from_stream(downstreamBuf, &downstreamIdx, x);

	if ((result == k_line_end) || (result == k_line_set_station_id)) {
		downstreamIdx = 0;
	}

	return result;
}
*/

void computer_run(uint16_t elapsedMillis)
{
	float elapsed_f = elapsedMillis * (1.0f / 1000.0f);
	vTime += elapsed_f;

	vLEDIndex = 0.0f;
	vLEDRatio = 0.0f;
	float ratioInc = 1.0f / vLEDCount;

	vUltrasonicCM = lerp(vUltrasonicCM, vUltrasonicCM_dest, min(elapsed_f * SENSOR_SMOOTHING, 1.0f));

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

	}	// !for each LED

	if (DEBUG_STATE) {
		Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~");
	}

}

BlinkType computer_get_blink_type() {
	return blink_type;
}

void computer_set_ultrasonic_cm(float cm) {
	vUltrasonicCM_dest = cm;
}

#endif
