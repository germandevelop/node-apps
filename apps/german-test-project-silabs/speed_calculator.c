#include "speed_calculator.h"

#include <stddef.h>

#include "motion_data.h"
#include "result.h"


#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static void speed_calculator_append_measurements_in_moving_away_mode(speed_calculator_t * const speed_calculator,
									motion_data_t const * const motion_data);
static void speed_calculator_append_measurements_in_moving_toward_mode(speed_calculator_t * const speed_calculator,
									motion_data_t const * const motion_data);

void speed_calculator_init(speed_calculator_t * const speed_calculator)
{
	speed_calculator->speed_size = 0U;
	speed_calculator->energy_size = 0U;

	speed_calculator->append_measurements = NULL;

	speed_calculator->is_computed = false;

	return;
}

void speed_calculator_start_in_moving_away_mode(speed_calculator_t * const speed_calculator)
{
	speed_calculator->speed_size = 0U;
	speed_calculator->energy_size = 0U;

	speed_calculator->append_measurements = speed_calculator_append_measurements_in_moving_away_mode;

	speed_calculator->is_computed = false;

	return;
}

void speed_calculator_start_in_moving_toward_mode(speed_calculator_t * const speed_calculator)
{
	speed_calculator->speed_size = 0U;
	speed_calculator->energy_size = 0U;

	speed_calculator->append_measurements = speed_calculator_append_measurements_in_moving_toward_mode;

	speed_calculator->is_computed = false;

	return;
}

void speed_calculator_is_computed(speed_calculator_t const * const speed_calculator,
					bool * const is_computed)
{
	*is_computed = speed_calculator->is_computed;

	return;
}

void speed_calculator_append_measurements(speed_calculator_t * const speed_calculator,
					motion_data_t const * const motion_data)
{
	if(speed_calculator->is_computed == false)
	{
		speed_calculator->append_measurements(speed_calculator, motion_data);
	}
	return;
}

void speed_calculator_append_measurements_in_moving_away_mode(speed_calculator_t * const speed_calculator,
								motion_data_t const * const motion_data)
{
	bool is_speed_collecting = true;

	// append energy measurements
	if(speed_calculator->speed_size == 0U)
	{
		is_speed_collecting = false;

		// fill energy data at fisrt
		if(speed_calculator->energy_size < ARRAY_SIZE(speed_calculator->energy))
		{
			speed_calculator->energy[speed_calculator->energy_size] = motion_data->energy;

			++speed_calculator->energy_size;
		}

		// process energy data
		else
		{
			// shift previous energy data and append new one
			const uint8_t last_idx = speed_calculator->energy_size - 1U;

			for(uint8_t i = 1U; i <= last_idx; ++i)
			{
				speed_calculator->energy[i - 1U] = speed_calculator->energy[i];
			}
			speed_calculator->energy[last_idx] = motion_data->energy;

			// try to detect "pass detection border"
			// if border is detected -> start speed data collecting
			if(speed_calculator->energy[last_idx] <= PASS_DETECTION_THRESHOLD)
			{
				bool is_energy_decreased = true;

				for(uint8_t i = 0U; i < last_idx; ++i)
				{
					if(speed_calculator->energy[i] <= PASS_DETECTION_THRESHOLD)
					{
						is_energy_decreased = false;

						break;
					}
				}
				if(is_energy_decreased == true)
				{
					is_speed_collecting = true;
				}
			}

		}
	}

	// append speed measurements
	if(is_speed_collecting == true)
	{
		// fill speed data at fisrt
		if(speed_calculator->speed_size < ARRAY_SIZE(speed_calculator->speed))
		{
			speed_calculator->speed[speed_calculator->speed_size] = motion_data->speed;

			++speed_calculator->speed_size;
		}

		// if speed data is filled -> stop calculation process
		else
		{
			speed_calculator->is_computed = true;
		}
	}
	return;
}

void speed_calculator_append_measurements_in_moving_toward_mode(speed_calculator_t * const speed_calculator,
								motion_data_t const * const motion_data)
{
	// append energy measurements
	{
		// fill energy data at fisrt
		if(speed_calculator->energy_size < ARRAY_SIZE(speed_calculator->energy))
		{
			speed_calculator->energy[speed_calculator->energy_size] = motion_data->energy;

			++speed_calculator->energy_size;
		}

		// process energy data
		else
		{
			// shift previous energy data and append new one
			const uint8_t last_idx = speed_calculator->energy_size - 1U;

			for(uint8_t i = 1U; i <= last_idx; ++i)
			{
				speed_calculator->energy[i - 1U] = speed_calculator->energy[i];
			}
			speed_calculator->energy[last_idx] = motion_data->energy;

			// try to detect "pass detection border"
			// if border is detected -> stop calculation process
			if(speed_calculator->energy[last_idx] > PASS_DETECTION_THRESHOLD)
			{
				bool is_energy_increased = true;

				for(uint8_t i = 0U; i < last_idx; ++i)
				{
					if(speed_calculator->energy[i] > PASS_DETECTION_THRESHOLD)
					{
						is_energy_increased = false;

						break;
					}
				}
				if(is_energy_increased == true)
				{
					speed_calculator->is_computed = true;

					return;
				}
			}
		}
	}

	// append speed measurements
	{
		// fill speed data at fisrt
		if(speed_calculator->speed_size < ARRAY_SIZE(speed_calculator->speed))
		{
			speed_calculator->speed[speed_calculator->speed_size] = motion_data->speed;

			++speed_calculator->speed_size;
		}

		// shift previous speed data and append new one
		else
		{
			const uint8_t last_idx = speed_calculator->speed_size - 1U;

			for(uint8_t i = 1U; i <= last_idx; ++i)
			{
				speed_calculator->speed[i - 1U] = speed_calculator->speed[i];
			}
			speed_calculator->speed[last_idx] = motion_data->speed;
		}
	}
	return;
}

int speed_calculator_get_average_speed(speed_calculator_t const * const speed_calculator,
					double * const average_speed)
{
	if(speed_calculator->speed_size > 0U)
	{
		double sum = 0.0;
		double divider = 0.0;

		for(uint8_t i = 0U; i < speed_calculator->speed_size; ++i)
		{
			if(speed_calculator->speed[i] > 0.1)
			{
				sum += speed_calculator->speed[i];

				++divider;
			}
		}
		if(divider > 0.1)
		{
			*average_speed = sum / divider;

			return SUCCESS;
		}
	}
	return FAILURE;
}

