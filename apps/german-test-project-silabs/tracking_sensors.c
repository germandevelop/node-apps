#include "tracking_sensors.h"

#include "motion_data.h"
#include "result.h"


#ifdef USE_SENSORS_SIMULATOR

#include "fake_sensors.h"

int tracking_sensors_init(movement_detect_t detect_movement_callback,
			motion_data_receive_t receive_motion_data_callback)
{
	return fake_sensors_init(detect_movement_callback, receive_motion_data_callback);
}

void tracking_sensors_turn_on_radar()
{
	fake_sensors_turn_on_radar();

	return;
}

void tracking_sensors_turn_off_radar()
{
	fake_sensors_turn_off_radar();

	return;
}

#else
#include <stddef.h>
#include "microwave_sensor.h"

int tracking_sensors_init(movement_detect_t detect_movement_callback,
			motion_data_receive_t receive_motion_data_callback)
{
	microwave_sensor_init(NULL);

	return SUCCESS;
}

void tracking_sensors_turn_on_radar()
{
	return;
}

void tracking_sensors_turn_off_radar()
{
	return;
}

#endif
