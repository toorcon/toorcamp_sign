#ifndef COMPUTER_H
#define COMPUTER_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define MAX_STEPS          (20)
#define ARG_COUNT          (3)

#define DEBUG_STATE        (false)
#define SERIAL_PRINT_RUN   (true)

// Shortcuts for operator functions
#define f0                 (f(0))
#define f1                 (f(1))
#define f2                 (f(2))

#define true_f             (1.0f)
#define false_f            (0.0f)

typedef struct arg {
	union {
		float f;
		float * fp;
	};
	bool is_fp;
} Arg;

uint8_t step_count = 0;
float (*ops[MAX_STEPS])();
Arg args[MAX_STEPS][ARG_COUNT];
float values[MAX_STEPS];	// Computed values

void (*serial_fp)(uint8_t);
int8_t step_idx = 0;
Arg * current_arg = NULL;
float float_dec = 1.0f;
uint8_t buf[2];

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
float op_modulus() { return fmodf(f0, f1); }
float op_gt() { return (f0 > f1) ? true_f : false_f; }
float op_gte() { return (f0 >= f1) ? true_f : false_f; }
float op_lt() { return (f0 < f1) ? true_f : false_f; }
float op_lte() { return (f0 <= f1) ? true_f : false_f; }
float op_equal() { return (f0 == f1) ? true_f : false_f; }
float op_notequal() { return (f0 != f1) ? true_f : false_f; }
float op_ternary() { return (f0 != 0.0f) ? f1 : f2; }

// Functions

float op_sin() { return sinf(f0); }
float op_cos() { return cosf(f0); }
float op_tan() { return tanf(f0); }
float op_pow() { return powf(f0, f1); }
float op_abs() { return fabsf(f0); }
float op_atan2() { return atan2f(f0, f1); }
float op_floor() { return floorf(f0); }
float op_ceil() { return ceilf(f0); }
float op_round() { return roundf(f0); }
float op_sqrt() { return sqrtf(f0); }
float op_log() { return logf(f0); }	// FIXME need arbitrary bases
float op_rand() { return 0.0f; } // FIXME
float op_randRange() { return 0.0f; } // FIXME
float op_min() { return min(f0, f1); }
float op_max() { return max(f0, f1); }
float op_lerp() { return f0 + (f1 - f0) * f2; }
float op_clamp() { return 0.0f; }	// FIXME
float op_tri() { return 0.0f; }	// FIXME
float op_uni2bi() { return 0.0f; }	// FIXME
float op_bi2uni() { return 0.0f; }	// FIXME

//
//  SERIAL INPUT
//

void serial_read_step(uint8_t x);
void serial_read_op(uint8_t x);
void serial_arg_start(uint8_t x);
void serial_arg_read_float(uint8_t x);
void serial_arg_read_buf(uint8_t x);
void serial_arg_complete(uint8_t x);
void serial_error_wait_for_newline(uint8_t x);

void serial_read_step(uint8_t x)
{
	if ((x < '!') || (('!' + MAX_STEPS) <= x)) {
		step_idx = -1;
		serial_fp = serial_error_wait_for_newline;
	}

	step_idx = x - '!';
	serial_fp = serial_read_op;

	// Clear args
	for (uint8_t i = 0; i < ARG_COUNT; i++) {
		args[step_idx][i] = (Arg){.f = 0.0f, .is_fp = false};
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
		case 'Q': {ops[step_idx] = op_sqrt; break;}
		case 'L': {ops[step_idx] = op_log; break;}
		case 'z': {ops[step_idx] = op_rand; break;}
		case 'Z': {ops[step_idx] = op_randRange; break;}
		case 'm': {ops[step_idx] = op_min; break;}
		case 'M': {ops[step_idx] = op_max; break;}
		case 'p': {ops[step_idx] = op_lerp; break;}
		case 'x': {ops[step_idx] = op_clamp; break;}
		case 't': {ops[step_idx] = op_tri; break;}
		case 'b': {ops[step_idx] = op_uni2bi; break;}
		case 'u': {ops[step_idx] = op_bi2uni; break;}
	}

	current_arg = &args[step_idx][0];
	serial_fp = serial_arg_start;
}

void serial_arg_start(uint8_t x)
{
	if (x == '\n') {
		serial_fp = serial_read_step;
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
			serial_fp = serial_read_step;
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
	current_arg->is_fp = true;

	// Point to a computed value?
	if (buf[0] == 'v') {
		current_arg->fp = &values[x - '!'];

	} else {
		// Interpret the many 2-byte possibilities here

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
		serial_fp = serial_read_step;

	} else if (x == ',') {
		prepare_for_next_argument();

	} else {
		serial_fp = serial_error_wait_for_newline;
	}
}

void serial_error_wait_for_newline(uint8_t x)
{
	if (x == '\n') {
		serial_fp = serial_read_step;
	}
}

//
//  INIT, INPUT
//

void computer_init() {
	serial_fp = serial_read_step;
}

void computer_serial_input(uint8_t x)
{
	serial_fp(x);
}

void computer_run(uint8_t step_count)
{
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
	}

	if (DEBUG_STATE) {
		Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~");
	}
}

#endif
