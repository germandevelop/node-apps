#ifndef FAKE_SENSORS_H_
#define FAKE_SENSORS_H_

typedef struct radar_data radar_data_t;

typedef void (*motion_detect_f) ();
typedef void (*radar_data_receive_f) (radar_data_t const * const radar_data);

int fake_sensors_init(motion_detect_f detect_callback, radar_data_receive_f receive_callback);

void fake_sensors_turn_on_radar();
void fake_sensors_turn_off_radar();

#endif
