#include "tracking_session.h"

#include "result.h"


void tracking_session_init(tracking_session_t * const tracking_session)
{
	tracking_session->is_active = false;
	tracking_session->is_speed_completed = false;

	speed_calculator_init(&tracking_session->speed_calculator);
	tracking_session->start_time = 0U;
	tracking_session->end_time = 0U;

	return;
}

void tracking_session_start(tracking_session_t * const tracking_session,
				tracking_session_mode_t tracking_mode,
				uint32_t current_time,
				uint32_t max_time_length)
{
	tracking_session->start_time = current_time;
	tracking_session->end_time = tracking_session->start_time + max_time_length;

	tracking_session->is_speed_completed = false;
	tracking_session->is_active = true;

	switch(tracking_mode)
	{
		case OBJECT_MOVING_AWAY :
		{
			speed_calculator_start_in_moving_away_mode(&tracking_session->speed_calculator);
			tracking_session->mode = OBJECT_MOVING_AWAY;

			break;
		}
		case OBJECT_MOVING_TOWARD :
		{
			speed_calculator_start_in_moving_toward_mode(&tracking_session->speed_calculator);
			tracking_session->mode = OBJECT_MOVING_TOWARD;

			break;
		}
		default :
		{
			tracking_session_stop(tracking_session);
		}
	}
	return;
}

void tracking_session_set_max_length(tracking_session_t * const tracking_session,
					uint32_t max_time_length)
{
	tracking_session->end_time = tracking_session->start_time + max_time_length;

	return;
}

void tracking_session_is_active(tracking_session_t * const tracking_session,
				bool * const is_active)
{
	*is_active = tracking_session->is_active;

	return;
}

void tracking_session_get_mode(tracking_session_t * const tracking_session,
				tracking_session_mode_t * const tracking_mode)
{
	*tracking_mode = tracking_session->mode;

	return;
}

void tracking_session_stop(tracking_session_t * const tracking_session)
{
	tracking_session->is_active = false;
	tracking_session->is_speed_completed = false;

	tracking_session->start_time = 0U;
	tracking_session->end_time = 0U;

	return;
}

void tracking_session_add_motion_data(tracking_session_t * const tracking_session,
					motion_data_t const * const motion_data,
					uint32_t current_time)
{
	if((tracking_session->is_active == true) && (tracking_session->is_speed_completed == false))
	{
		speed_calculator_append_measurements(&tracking_session->speed_calculator, motion_data);

		bool is_speed_computed;
		speed_calculator_is_computed(&tracking_session->speed_calculator, &is_speed_computed);

		const bool is_time_over = ((current_time > tracking_session->start_time) && (current_time > tracking_session->end_time)) ||
						((current_time < tracking_session->start_time) && (current_time > tracking_session->end_time)); // overflow case

		if((is_speed_computed == true) || (is_time_over == true))
		{
			tracking_session->is_speed_completed = true;
		}
	}
	return;
}

void tracking_session_is_speed_completed(tracking_session_t * const tracking_session,
					bool * const is_speed_completed)
{
	*is_speed_completed = tracking_session->is_speed_completed;

	return;
}

int tracking_session_get_speed(tracking_session_t const * const tracking_session,
				double * const speed)
{
	*speed = 0.0;

	bool is_speed_computed;
	speed_calculator_is_computed(&tracking_session->speed_calculator, &is_speed_computed);

	if(is_speed_computed == true)
	{
		speed_calculator_get_average_speed(&tracking_session->speed_calculator, speed);

		return SUCCESS;
	}
	return FAILURE;
}

