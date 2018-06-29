#ifndef LED_LAYOUT_H
#define LED_LAYOUT_H

#include <stdint.h>

#define TWOPI (6.283185f)

// Width: Include padding between stations
#define STATION_LEDS_ACROSS   (11)
#define STATION_LED_WIDTH     (16)
#define STATION_LED_HEIGHT    (19)

// Longest LED run length
#define RL   (76)

// Directions
#define X       (0)
#define U       (1)
#define R       (2)
#define D       (3)
#define L       (4)

// And some sub-steps that don't align to the grid
#define DL      (5)
#define DL_2_3  (6)
#define DR_2_3  (7)

typedef enum {
	k_read_led_index = 0,
	k_read_x,
	k_read_y,
	k_read_dir
} LayoutState;

typedef uint8_t station_data_t;

// T
const station_data_t STATION_0[] = {
	// Heading left
	RL*0,  5, 18, L, U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,U, L,L, X,

	// Heading right
	RL*1,  6, 18, U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,U, R,R,R,R, U,U, L,L,L,L,L,L,L,L,L,L, D,D, R, X,

	X
};

// O1
const station_data_t STATION_1[] = {
	// Inside
	RL*0,  8, 16, L,L,L,L,L,L, U,U,U,U,U,U,U,U,U,U,U,U,U,U, R,R,R,R,R,R, D,D,D,D,D,D,D,D,D,D,D,D,D, X,

	// Outside, going left
	RL*1, 10, 18, L,L,L,L,L,L,L,L,L,L, U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,U, R,R,R,R,R,R,R,R,R,R, D, X,

	// Outside, going up
	RL*2, 10, 17, U,U,U,U,U,U,U,U,U,U,U,U,U,U,U,U, X,

	X
};

// O2
const station_data_t STATION_2[] = {
	// Outside, going left
	RL*0,  9, 18, L,L,L,L,L,L,L,L,L, U,U,U,U,U,U,U,U,U,U, X,

	// Inside
	RL*1,  8, 16, L,L,L,L,L,L, U,U,U,U,U,U,U,U,U,U,U,U,U,U, R,R,R,R,R,R, D,D,D,D,D,D,D,D,D,D,D,D,D, X,

	// Outside, going up
	RL*2, 10, 18, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U, L,L,L,L,L, L,L,L,L,L, D,D,D,D,D,D,D, X,

	X
};

// R
const station_data_t STATION_3[] = {
	// Going up
	RL*1, 10, 18,  U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U, L,L,L,L,L, L,L,L, DL,DL, D,D,D,D, DR_2_3,DR_2_3, X,

	// Around the hole
	RL*1+35,  2, 7, U,U,U,U,U, R,R,R,R,R,R, D,D,D,D,D,D, L,L,L,L,L, D, X,

	// Going left
	RL*2,  9, 18,  L, U,U,U,U,U,U,U,U, L,L,L,L,L, DL_2_3,DL_2_3,DL_2_3, D,D,D,D,D,D,D, R,R, U,U,U,U,U,U,U, X,

	X
};

// C
const station_data_t STATION_4[] = {
	// Bottom, going left
	RL*0,  10, 18, L,L,L,L,L, L,L,L,L,L, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U, R,R,R,R,R, R,R,R,R,R, D, X,

	// Going up
	RL*1,  10, 17, U, L,L,L,L,L, L,L,L, U,U,U,U,U,U,U,U,U,U,U,U,U,U, R,R,R,R,R,R,R,R, X,

	X
};

// A
const station_data_t STATION_5[] = {
	// Going right
	RL*0,  9, 18, R, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U, X,

	// Jumps to the center
	RL*0+20,  8, 2, L,L,L,L,L,L, D,D,D,D,D,D, R,R,R,R,R,R, U,U,U,U,U, X,

	// Going up
	RL*2,  8, 18, U,U,U,U,U,U,U,U, L,L,L,L,L,L, D,D,D,D,D,D,D,D, L,L, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U, R,R,R,R,R,R,R,R,R, X,

	X
};

// It's the huge M oh lord
const station_data_t STATION_6[] = {
	// Going left
	RL*0,  8, 18, L, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U, L,L,L,L,L,L, D,D,D,D,D, D,D,D,D,D, D,D,D,D,D, D, L,L, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U, R,R,R,R,R, R,R,R,R,R, R,R,R,R,R,R, X,

	// Going up
	RL*2,  9, 18, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U, R,R,R,R,R,R, D,D,D,D,D, D,D,D,D,D, D,D,D,D,D, D, R,R, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U, L, X,

	X
};

// P
const station_data_t STATION_7[] = {
	// Going left
	RL*2, 1, 18, L, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U, R,R,R,R,R, R,R,R,R,R, D,D,D,D,D,D, X,

	// Going up
	RL*1, 2, 18, U,U,U,U,U, U,U,U, R,R,R,R,R, R,R,R, U,U,U, X,

	// Jumps to the middle
	RL*1+20,  8, 7, D, L,L,L,L,L,L, U,U,U,U,U,U, R,R,R,R,R,R, D,D,D,D, X,

	X
};

const station_data_t * STATIONS[] = {
	STATION_0,
	STATION_1,
	STATION_2,
	STATION_3,
	STATION_4,
	STATION_5,
	STATION_6,
	STATION_7
};

void _set_led_position(float x, float y, bool * does_led_exist_ar, float * x_ptr, float * y_ptr, float * local_angle_ptr)
{
	*does_led_exist_ar = true;
	*x_ptr = x * (1.0f / STATION_LED_WIDTH);
	*y_ptr = y * (1.0f / STATION_LED_HEIGHT);

	*local_angle_ptr = atan2(
		-((float)(y) - STATION_LED_HEIGHT * 0.5f),
		(float)(x) - STATION_LEDS_ACROSS * 0.5f
	) * (1.0f / TWOPI);
}

void led_layout_set_all(uint8_t station_id, bool * does_led_exist_ar, float * x_ar, float * y_ar, float * local_angle_ar)
{
	// Different stations have different LED layouts.
	const station_data_t * data = STATIONS[station_id];

	LayoutState state = k_read_led_index;
	uint8_t led_index = 0;
	float x;
	float y;

	for (uint16_t i = 0; i < 999; i++) {
		switch (state) {
			case k_read_led_index:
			{
				// Terminating X? We're done.
				if ((i > 0) && (data[i] == X)) {
					return;
				}

				led_index = data[i];
				state = k_read_x;
			}
			break;

			case k_read_x:
			{
				x = data[i];
				state = k_read_y;
			}
			break;

			case k_read_y:
			{
				y = data[i];

				// Set this LED
				_set_led_position(x, y, &does_led_exist_ar[led_index], &x_ar[led_index], &y_ar[led_index], &local_angle_ar[led_index]);

				state = k_read_dir;
			}
			break;

			case k_read_dir:
			{
				if (data[i] == X) {
					state = k_read_led_index;
					break;
				}

				else if (data[i] == L) {x--;}
				else if (data[i] == R) {x++;}
				else if (data[i] == U) {y--;}
				else if (data[i] == D) {y++;}
				else if (data[i] == DL) {
					x--;
					y++;
				}
				else if (data[i] == DL_2_3) {
					x -= 1.333f;
					y += 1.333f;
				}
				else if (data[i] == DR_2_3) {
					x += 1.333f;
					y += 1.333f;
				}

				led_index++;

				_set_led_position(x, y, &does_led_exist_ar[led_index], &x_ar[led_index], &y_ar[led_index], &local_angle_ar[led_index]);

				//state = k_read_dir	// already set
			}
			break;
		}
	}

}

#endif
