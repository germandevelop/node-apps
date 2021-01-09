#ifndef TRACKING_UNIT_H_
#define TRACKING_UNIT_H_

#include <stdint.h>

typedef struct comms_layer comms_layer_t;

typedef struct tracking_config
{
	// tracking system
	double distance_between_units; // for example 100.0 m

	// tracking unit
	double pass_detection_threshold;

	// tracking partner
	comms_layer_t *radio_module;
	uint16_t partner_radio_address;
	uint8_t radio_port;

} tracking_config_t;

int tracking_unit_init(tracking_config_t const * const tracking_config);

#endif
