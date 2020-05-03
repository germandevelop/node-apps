#include "microwave_motion_converter.h"

#include "vehicle_speed_detector.h"
#include "signal_normalizer.h"

#include "motion_data.h"
#include "result.h"


int convert_microwave_samples_to_motion_data(uint16_t mw_samples[MW_FRAME_LENGHT], motion_data_t * const motion_data)
{
	double normalized_samples[MW_FRAME_LENGHT];

	normalize_signal(ADC_BITS, MW_FRAME_LENGHT, mw_samples, normalized_samples);

	double motion_output[2];

	const bool is_detected = detect_vehicle_speed(motion_output, MW_FRAME_LENGHT, normalized_samples);

	if(is_detected == true)
	{
		motion_data->speed = motion_output[0];
		motion_data->energy = motion_output[1];

		return SUCCESS;
	}
	return FAILURE;
}

