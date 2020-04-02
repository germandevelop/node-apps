#ifndef SPEED_CALCULATOR_H_
#define SPEED_CALCULATOR_H_

#include <stdint.h>
#include <stdbool.h>

#define SPEED_DETECTION_WINDOW 12U
#define PASS_DETECTION_THRESHOLD 400U

typedef struct radar_data radar_data_t;

typedef struct speed_calculator
{
	int32_t energy[4]; // [2 - 4]
	uint8_t energy_size;

	int32_t speed[SPEED_DETECTION_WINDOW];
	uint8_t speed_size;

	void(*append_measurements)(struct speed_calculator * const speed_calculator,
					radar_data_t const * const radar_data);
	bool is_computed;

} speed_calculator_t;

void speed_calculator_init(speed_calculator_t * const speed_calculator);

void speed_calculator_start_in_moving_away_mode(speed_calculator_t * const speed_calculator);
void speed_calculator_start_in_moving_closer_mode(speed_calculator_t * const speed_calculator);
void speed_calculator_is_computed(speed_calculator_t const * const speed_calculator,
					bool * const is_computed);

void speed_calculator_append_measurements(speed_calculator_t * const speed_calculator,
					radar_data_t const * const radar_data);
int speed_calculator_get_average_speed(speed_calculator_t const * const speed_calculator,
					int32_t * const average_speed);

#endif
