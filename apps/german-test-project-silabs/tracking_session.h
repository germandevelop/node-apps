#ifndef TRACKING_SESSION_H_
#define TRACKING_SESSION_H_

#include "speed_calculator.h"

typedef struct tracking_session
{
	speed_calculator_t speed_calculator;

	bool is_active;

} tracking_session_t;

void tracking_session_init(tracking_session_t * const tracking_session);

void tracking_session_start(tracking_session_t * const tracking_session);
void tracking_session_stop(tracking_session_t * const tracking_session);

void tracking_session_register_motion(tracking_session_t * const tracking_session,
					bool * const is_started);

void tracking_session_add_radar_data(tracking_session_t * const tracking_session,
					radar_data_t const * const radar_data);
void tracking_session_is_speed_detected(tracking_session_t const * const tracking_session,
					bool * const is_speed_detected);
int tracking_session_get_speed(tracking_session_t const * const tracking_session,
				int32_t * const speed);

#endif
