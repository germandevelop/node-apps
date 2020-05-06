#ifndef TRACKING_SESSION_H_
#define TRACKING_SESSION_H_

#include "speed_calculator.h"

typedef enum tracking_session_mode
{
	OBJECT_MOVING_AWAY = 0,
	OBJECT_MOVING_TOWARD

} tracking_session_mode_t;


typedef struct tracking_session
{
	speed_calculator_t speed_calculator;
	uint32_t start_time;
	uint32_t end_time;

	tracking_session_mode_t mode;
	bool is_speed_completed;
	bool is_active;

} tracking_session_t;


void tracking_session_init(tracking_session_t * const tracking_session);

void tracking_session_start(tracking_session_t * const tracking_session,
				tracking_session_mode_t tracking_mode,
				uint32_t current_time,
				uint32_t max_time_length);

void tracking_session_set_max_length(tracking_session_t * const tracking_session,
					uint32_t max_time_length);

void tracking_session_is_active(tracking_session_t * const tracking_session,
				bool * const is_active);

void tracking_session_get_mode(tracking_session_t * const tracking_session,
				tracking_session_mode_t * const tracking_mode);

void tracking_session_stop(tracking_session_t * const tracking_session);


void tracking_session_add_motion_data(tracking_session_t * const tracking_session,
					motion_data_t const * const motion_data,
					uint32_t current_time);

void tracking_session_is_speed_completed(tracking_session_t * const tracking_session,
					bool * const is_speed_completed);

int tracking_session_get_speed(tracking_session_t const * const tracking_session,
				double * const speed);

#endif
