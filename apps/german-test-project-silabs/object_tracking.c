#include "object_tracking.h"

#include "result.h"


void object_tracking_init(object_tracking_t * const object_tracking,
				tracking_params_t const * const tracking_params)
{
	object_tracking->is_speed_completed = false;

	object_tracking->start_time = 0U;
	object_tracking->end_time = 0U;

	speed_calculator_init(&object_tracking->speed_calculator, tracking_params->pass_detection_threshold);

	return;
}

void object_tracking_start(object_tracking_t * const object_tracking,
				tracking_mode_t tracking_mode,
				uint32_t current_time,
				uint32_t max_time_length)
{
	object_tracking->start_time = current_time;
	object_tracking->end_time = object_tracking->start_time + max_time_length;

	object_tracking->is_speed_completed = false;

	switch(tracking_mode)
	{
		case OBJECT_MOVING_AWAY :
		{
			speed_calculator_start_in_moving_away_mode(&object_tracking->speed_calculator);
			object_tracking->mode = OBJECT_MOVING_AWAY;

			break;
		}
		case OBJECT_MOVING_TOWARD :
		{
			speed_calculator_start_in_moving_toward_mode(&object_tracking->speed_calculator);
			object_tracking->mode = OBJECT_MOVING_TOWARD;

			break;
		}
	}
	return;
}

void object_tracking_set_max_length(object_tracking_t * const object_tracking,
					uint32_t max_time_length)
{
	object_tracking->end_time = object_tracking->start_time + max_time_length;

	return;
}

void object_tracking_get_mode(object_tracking_t const * const object_tracking,
				tracking_mode_t * const tracking_mode)
{
	*tracking_mode = object_tracking->mode;

	return;
}

void object_tracking_add_motion_data(object_tracking_t * const object_tracking,
					motion_data_t const * const motion_data,
					uint32_t current_time)
{
	if(object_tracking->is_speed_completed == false)
	{
		speed_calculator_append_measurements(&object_tracking->speed_calculator, motion_data);

		bool is_speed_computed;
		speed_calculator_is_computed(&object_tracking->speed_calculator, &is_speed_computed);

		const bool is_time_over = ((current_time > object_tracking->start_time) && (current_time > object_tracking->end_time)) ||
						((current_time < object_tracking->start_time) && (current_time > object_tracking->end_time)); // overflow case

		if((is_speed_computed == true) || (is_time_over == true))
		{
			object_tracking->is_speed_completed = true;
		}
	}
	return;
}

void object_tracking_is_speed_completed(object_tracking_t const * const object_tracking,
					bool * const is_speed_completed)
{
	*is_speed_completed = object_tracking->is_speed_completed;

	return;
}

int object_tracking_get_speed(object_tracking_t const * const object_tracking,
				double * const speed)
{
	*speed = 0.0;

	bool is_speed_computed;
	speed_calculator_is_computed(&object_tracking->speed_calculator, &is_speed_computed);

	if(is_speed_computed == true)
	{
		speed_calculator_get_average_speed(&object_tracking->speed_calculator, speed);

		return SUCCESS;
	}
	return FAILURE;
}

