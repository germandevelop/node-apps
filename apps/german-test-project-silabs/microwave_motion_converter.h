#ifndef MICROWAVE_MOTION_CONVERTER_H_
#define MICROWAVE_MOTION_CONVERTER_H_

// CONFIGURABLE VALUES
/************************************************************************************************/
#define MW_FRAME_LENGHT 256U
#define ADC_BITS 12U
/************************************************************************************************/

#include <stdint.h>

typedef struct motion_data motion_data_t;

int convert_microwave_samples_to_motion_data(uint16_t mw_samples[MW_FRAME_LENGHT], motion_data_t * const motion_data);

#endif
