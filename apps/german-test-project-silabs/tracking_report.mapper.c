#include "tracking_report.mapper.h"

#include "ML.h"
#include "MLE.h"
#include "MLI.h"
#include "dt_types.h"

#include "result.h"


int tracking_report_serialize_to_mote_xml(tracking_report_t const * const tracking_report,
						uint8_t max_data_size,
						uint8_t *serialized_data,
						uint8_t * const data_size)
{
	ml_encoder_t mote_xml_encoder;

	if(MLE_initialize(&mote_xml_encoder, serialized_data, max_data_size) == ML_SUCCESS)
	{
        	const uint8_t body_object = MLE_appendO(&mote_xml_encoder, dt_packet);
		{
			const uint8_t document_object = MLE_appendOS(&mote_xml_encoder, dt_data, body_object);
			{
				const uint8_t radar_object = MLE_appendOS(&mote_xml_encoder, dt_sensor, document_object);
				{
					MLE_appendOSV(&mote_xml_encoder, dt_speed_kmh, radar_object, tracking_report->speed);
					//MLE_appendOSV(&mote_xml_encoder, dt_energy_Wh, radar_object, tracking_report->energy);
				}
			}
		}
		*data_size = MLE_finalize(&mote_xml_encoder);

		if(*data_size > 0U)
		{
			return SUCCESS;
		}
	}
	return FAILURE;
}

int tracking_report_deserialize_from_mote_xml(uint8_t const * const serialized_data,
						uint8_t data_size,
						tracking_report_t * const tracking_report)
{
	ml_iterator_t mote_xml_itr;

	if(MLI_initialize(&mote_xml_itr, serialized_data, data_size) == ML_SUCCESS)
	{
		ml_object_t mote_xml_object;

		bool is_speed_parsed = false;

		const uint8_t radar_object = MLI_nextWithType(&mote_xml_itr, dt_sensor, &mote_xml_object);

		while(MLI_nextWithSubject(&mote_xml_itr, radar_object, &mote_xml_object) != 0U) {

			switch(mote_xml_object.type)
			{
				case dt_speed_kmh:
				{
					tracking_report->speed = mote_xml_object.value;

					is_speed_parsed = true;

					break;
				}
			}
		}
		if(is_speed_parsed == true)
		{
			return SUCCESS;
		}
	}
	return FAILURE;
}

