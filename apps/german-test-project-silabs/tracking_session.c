#include "tracking_session.h"


void tracking_session_init(tracking_session_t * const tracking_session)
{
	tracking_session->is_active = false;

	speed_calculator_init(&tracking_session->speed_calculator);

	return;
}

void tracking_session_start(tracking_session_t * const tracking_session)
{
	tracking_session->is_active = true;

	speed_calculator_start_in_moving_closer_mode(&tracking_session->speed_calculator);

	return;
}

void tracking_session_stop(tracking_session_t * const tracking_session)
{
	tracking_session->is_active = false;

	return;
}

void tracking_session_register_motion(tracking_session_t * const tracking_session,
					bool * const is_started)
{
	*is_started = false;

	if(tracking_session->is_active == false)
	{
		tracking_session->is_active = true;

		speed_calculator_start_in_moving_away_mode(&tracking_session->speed_calculator);

		*is_started = true;
	}
	else
	{
		tracking_session_stop(tracking_session);
	}
	return;
}

void tracking_session_add_radar_data(tracking_session_t * const tracking_session,
					radar_data_t const * const radar_data)
{
	speed_calculator_append_measurements(&tracking_session->speed_calculator, radar_data);

	return;
}

void tracking_session_is_speed_detected(tracking_session_t const * const tracking_session,
					bool * const is_speed_detected)
{
	speed_calculator_is_computed(&tracking_session->speed_calculator, is_speed_detected);

	return;
}

int tracking_session_get_speed(tracking_session_t const * const tracking_session,
				int32_t * const speed)
{
	return speed_calculator_get_average_speed(&tracking_session->speed_calculator, speed);
}

