#ifndef FAKE_SENSORS_H_
#define FAKE_SENSORS_H_

typedef struct motion_data motion_data_t;

typedef void (*motion_detect_f) ();
typedef void (*motion_data_receive_f) (motion_data_t const * const motion_data);

int fake_sensors_init(motion_detect_f detect_callback, motion_data_receive_f receive_callback);

void fake_sensors_turn_on_radar();
void fake_sensors_turn_off_radar();

#endif
