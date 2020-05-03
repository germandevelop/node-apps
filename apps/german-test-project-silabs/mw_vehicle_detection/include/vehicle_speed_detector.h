#ifndef VEHICLE_SPEED_DETECTOR_H_
#define VEHICLE_SPEED_DETECTOR_H_

#include <stdbool.h>

#define FREQUENCY 3000
#define SPEED_COEFICIENT 17.3148
#define LOW_PASS_FILTER 1
#define SPECTRAL_ENERGY_COEFICIENT 0.010 //adjusted for vehicle detection

bool detect_vehicle_speed (double output[2], int num_points, double * points);

#endif//VEHICLE_SPEED_DETECTOR_H_
