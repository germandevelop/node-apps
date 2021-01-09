#ifndef MICROWAVE_SENSOR_H_
#define MICROWAVE_SENSOR_H_

#include <stdbool.h>

typedef struct motion_data motion_data_t;

typedef void(*motion_data_receive_t)(motion_data_t const * const motion_data);

void microwave_sensor_init(motion_data_receive_t receive_motion_data_callback);

void microwave_sensor_turn_on();
void microwave_sensor_turn_off();


//Platform-specific MW sensor power-control function.
//This must be provided externally!
void semw_power_control(bool enable);

#endif
