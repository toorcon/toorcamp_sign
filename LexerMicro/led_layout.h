#ifndef LED_LAYOUT_H
#define LED_LAYOUT_H

#include <stdint.h>

#define TWOPI (6.283185f)

// Width: Include padding between stations
#define STATION_LED_WIDTH     (18)
#define STATION_LED_HEIGHT    (18)

// Longest LED run length
#define RL   (18)

// Directions
#define X    (0)
#define U    (1)
#define R    (2)
#define D    (3)
#define L    (4)

typedef enum {
	k_read_led_index = 0,
	k_read_x,
	k_read_y,
	k_read_dir
} LayoutState;

typedef uint8_t station_data_t;

const station_data_t STATION_0[] = {
	RL*0,  0,  0, R,R,R,R,R, R,R,R,R,R, R,R,R,R,R, R,R,R,X,
	RL*1, 18,  0, D,D,D,D,D, D,D,D,D,D, D,D,D,D,D, D,D,D,X,
	RL*2, 18, 18, L,L,L,L,L, L,L,L,L,L, L,L,L,L,L, L,L,L,X,
	RL*3,  0, 18, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U,X,
	X
};

const station_data_t STATION_1[] = {
	RL*0,  0,  0, R,R,R,R,R, R,R,R,R,R, R,R,R,R,R, R,R,R,X,
	RL*1, 18,  0, D,D,D,D,D, D,D,D,D,D, D,D,D,D,D, D,D,D,X,
	RL*2, 18, 18, L,L,L,L,L, L,L,L,L,L, L,L,L,L,L, L,L,L,X,
	RL*3,  0, 18, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U,X,
	X
};

const station_data_t STATION_2[] = {
	RL*0,  0,  0, R,R,R,R,R, R,R,R,R,R, R,R,R,R,R, R,R,R,X,
	RL*1, 18,  0, D,D,D,D,D, D,D,D,D,D, D,D,D,D,D, D,D,D,X,
	RL*2, 18, 18, L,L,L,L,L, L,L,L,L,L, L,L,L,L,L, L,L,L,X,
	RL*3,  0, 18, U,U,U,U,U, U,U,U,U,U, U,U,U,U,U, U,U,U,X,
	X
};

const station_data_t * STATIONS[] = {
	STATION_0,
	STATION_1,
	STATION_2
};

void _set_led_position(int16_t x, int16_t y, float * x_ptr, float * y_ptr, float * local_angle_ptr)
{
	*x_ptr = x * (1.0f / STATION_LED_WIDTH);
	*y_ptr = y * (1.0f / STATION_LED_HEIGHT);

	*local_angle_ptr = atan2(
		-((float)(y) - STATION_LED_HEIGHT * 0.5f),
		(float)(x) - STATION_LED_WIDTH * 0.5f
	) * (1.0f / TWOPI);
}

void led_layout_set_all(uint8_t station_id, float * x_ar, float * y_ar, float * local_angle_ar)
{
	// Different stations have different LED layouts.
	const station_data_t * data = STATIONS[station_id];

	LayoutState state = k_read_led_index;
	uint8_t led_index = 0;
	int16_t x;
	int16_t y;

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
				_set_led_position(x, y, &x_ar[led_index], &y_ar[led_index], &local_angle_ar[led_index]);

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

				led_index++;

				_set_led_position(x, y, &x_ar[led_index], &y_ar[led_index], &local_angle_ar[led_index]);

				//state = k_read_dir	// already set
			}
			break;
		}
	}

}

#endif
