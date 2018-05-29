#ifndef COMPUTER_H
#define COMPUTER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_STEPS     (20)
#define ARG_COUNT     (3)

#define DEBUG_STATE   (true)

typedef struct arg {
	union {
		float f;
		float * fp;
	};
	bool is_fp;
} Arg;

uint8_t step_count = 0;
void (*ops[MAX_STEPS])(uint8_t);
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

inline float get_f(Arg * arg)
{
	return (arg->is_fp) ? (*(arg->fp)) : (arg->f);
}

void op_add(uint8_t step)
{
	values[step] = get_f(&args[step][0]) + get_f(&args[step][1]);
}

void op_subtract(uint8_t step)
{
	values[step] = get_f(&args[step][0]) - get_f(&args[step][1]);
}

void op_multiply(uint8_t step)
{
	values[step] = get_f(&args[step][0]) * get_f(&args[step][1]);
}

void op_divide(uint8_t step)
{
	values[step] = get_f(&args[step][0]) / get_f(&args[step][1]);
}

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
		case '+': {ops[step_idx] = op_add; break;}
		case '-': {ops[step_idx] = op_subtract; break;}
		case '*': {ops[step_idx] = op_multiply; break;}
		case '/': {ops[step_idx] = op_divide; break;}
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
		ops[s](s);

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
