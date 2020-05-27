#ifndef OBJECT_TRACKING_H_
#define OBJECT_TRACKING_H_

#include "speed_calculator.h"

typedef enum tracking_mode
{
	OBJECT_MOVING_AWAY = 0,
	OBJECT_MOVING_TOWARD

} tracking_mode_t;


typedef struct object_tracking
{
	speed_calculator_t speed_calculator;
	uint32_t start_time;
	uint32_t end_time;

	tracking_mode_t mode;
	bool is_speed_completed;

} object_tracking_t;


typedef struct tracking_params
{
	double pass_detection_threshold;

} tracking_params_t;


void object_tracking_init(object_tracking_t * const object_tracking,
				tracking_params_t const * const tracking_params);

void object_tracking_start(object_tracking_t * const object_tracking,
				tracking_mode_t tracking_mode,
				uint32_t current_time,
				uint32_t max_time_length);

void object_tracking_set_max_length(object_tracking_t * const object_tracking,
					uint32_t max_time_length);

void object_tracking_get_mode(object_tracking_t const * const object_tracking,
				tracking_mode_t * const tracking_mode);


void object_tracking_add_motion_data(object_tracking_t * const object_tracking,
					motion_data_t const * const motion_data,
					uint32_t current_time);

void object_tracking_is_speed_completed(object_tracking_t const * const object_tracking,
					bool * const is_speed_completed);

int object_tracking_get_speed(object_tracking_t const * const object_tracking,
				double * const speed);

#endif

