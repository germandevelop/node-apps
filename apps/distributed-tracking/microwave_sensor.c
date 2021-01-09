#include "microwave_sensor.h"

#include "platform_io.h"

#include "platform_adc.h"

#include "mw_vehicle_sensor.h"

#include "loglevels.h"
#define __MODUUL__ "microwave_sensor"
#define __LOG_LEVEL__ (LOG_LEVEL_microwave_sensor & BASE_LOG_LEVEL)
#include "log.h"

#include "motion_data.h"
#include "result.h"


#define SMNT_MICROWAVE_SAMPLING_FREQUENCY 3000
#define SMNT_MICROWAVE_ADC_BITS 12


static motion_data_receive_t receive_microwave_motion_data_callback;


static void microwave_sensor_data_callback(uint32_t event_id, bool active, double speed, double energy, double max_speed, void *user);


void microwave_sensor_init(motion_data_receive_t receive_motion_data_callback)
{
	receive_microwave_motion_data_callback = receive_motion_data_callback;

	platform_adc_management_init();

	// Initialize microwave sensor system
	semw_input_config_t icfg = SEMW_INPUT_CONFIG_INIT_DEFAULT;
	semw_init(&icfg, &semw_power_control);

	// Configure and enable sensing
	semw_sensing_config_t scfg;
	scfg.bits = SMNT_MICROWAVE_ADC_BITS;
	scfg.frequency = SMNT_MICROWAVE_SAMPLING_FREQUENCY;

	semw_enable(&scfg, microwave_sensor_data_callback, NULL);

	return;
}

void microwave_sensor_turn_on()
{
	semw_trigger();

	return;
}

void microwave_sensor_turn_off()
{
	semw_disable();

	return;
}

void microwave_sensor_data_callback(uint32_t event_id, bool active, double speed, double energy, double max_speed, void *user)
{
	info1("MICROWAVE SPEED: %4.2f ENERGY: %5.3f", speed, energy);

	if(receive_microwave_motion_data_callback != NULL)
	{
		motion_data_t motion_data;
		motion_data.speed = speed;
		motion_data.energy = energy;

		receive_microwave_motion_data_callback(&motion_data);
	}
	return;
}

