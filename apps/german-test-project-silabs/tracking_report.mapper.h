#ifndef TRACKING_REPORT_MAPPER_H_
#define TRACKING_REPORT_MAPPER_H_

#include "tracking_report.h"

int tracking_report_serialize_to_mote_xml(tracking_report_t const * const tracking_report,
						uint8_t max_data_size,
						uint8_t *serialized_data,
						uint8_t * const data_size);

int tracking_report_deserialize_from_mote_xml(uint8_t const * const serialized_data,
						uint8_t data_size,
						tracking_report_t * const tracking_report);

#endif
