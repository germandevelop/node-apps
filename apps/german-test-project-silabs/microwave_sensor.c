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

// Energy threshold previously determined experimentally
#define SMNT_MICROWAVE_ENERGY_THRESHOLD 0.20
// Only 3 kHz has been tested in practice
#define SMNT_MICROWAVE_SAMPLING_FREQUENCY 3000
// Require at least 3 frames for event
#define SMNT_MICROWAVE_MIN_EVENT_FRAMES 3
// ~83 ms for a frame, so max ~12 seconds
#define SMNT_MICROWAVE_MAX_EVENT_FRAMES 145
// Consider event over after N frames
#define SMNT_MICROWAVE_QUIET_FRAMES 3
// Require at least 3 frames with adequate speed for event
#define SMNT_MICROWAVE_MIN_SPEED_EVENT_FRAMES 3
// Consider event over after N frames
#define SMNT_MICROWAVE_SPEED_QUIET_FRAMES 3
// Minimum probe
#define SMNT_MICROWAVE_PROBE_FRAMES 2
// Sleep for 1 second between MW activations
#define SMNT_MICROWAVE_SLEEP_TIME_MS 1000UL
// ADC precision - 12 bits
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
	scfg.sleep_ms = SMNT_MICROWAVE_SLEEP_TIME_MS;
	scfg.probe_frames = SMNT_MICROWAVE_PROBE_FRAMES;
	scfg.quiet_frames = SMNT_MICROWAVE_QUIET_FRAMES;
	scfg.min_event_frames = SMNT_MICROWAVE_MIN_EVENT_FRAMES;
	scfg.max_event_frames = SMNT_MICROWAVE_MAX_EVENT_FRAMES;
	scfg.energy_threshold = SMNT_MICROWAVE_ENERGY_THRESHOLD;
	scfg.bits = SMNT_MICROWAVE_ADC_BITS;
	scfg.frequency = SMNT_MICROWAVE_SAMPLING_FREQUENCY;
	scfg.speed_quiet_frames = SMNT_MICROWAVE_SPEED_QUIET_FRAMES;
	scfg.min_speed_event_frames = SMNT_MICROWAVE_MIN_SPEED_EVENT_FRAMES;

	semw_enable(&scfg, microwave_sensor_data_callback, NULL);

	return;
}

void microwave_sensor_data_callback(uint32_t event_id, bool active, double speed, double energy, double max_speed, void *user)
{
	warn1("MICROWAVE SPEED: %4.2f ENERGY: %5.3f", speed, energy);

	if(receive_microwave_motion_data_callback != NULL)
	{
		motion_data_t motion_data;
		motion_data.speed = speed;
		motion_data.energy = energy;

		receive_microwave_motion_data_callback(&motion_data);
	}
	return;
}

