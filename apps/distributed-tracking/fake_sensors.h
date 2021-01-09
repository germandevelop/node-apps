#ifndef FAKE_SENSORS_H_
#define FAKE_SENSORS_H_

typedef struct motion_data motion_data_t;

typedef void(*movement_detect_t)();
typedef void(*motion_data_receive_t)(motion_data_t const * const motion_data);

int fake_sensors_init(movement_detect_t detect_movement_callback,
			motion_data_receive_t receive_motion_data_callback);

void fake_sensors_turn_on_radar();
void fake_sensors_turn_off_radar();

#endif
