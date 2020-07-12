#ifndef MOTION_DATA_EXAMPLES_H_
#define MOTION_DATA_EXAMPLES_H_

// The first column of arrays means a speed value, the second - a an energy value

#define DEAFAULT_ARRAY_SIZE 256U
#define LAST_ELEMENT_VALUE -1.0

extern const double moving_toward_good_data[DEAFAULT_ARRAY_SIZE];
extern const double moving_toward_no_speed_data[DEAFAULT_ARRAY_SIZE];

extern const double moving_away_good_data[DEAFAULT_ARRAY_SIZE];
extern const double moving_away_no_speed_data[DEAFAULT_ARRAY_SIZE];


#endif
