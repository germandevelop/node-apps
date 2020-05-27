#include "tracking_partner.mapper.h"

#include "ML.h"
#include "MLE.h"
#include "MLI.h"
#include "dt_types.h"

#include "tracking_summary.h"
#include "result.h"


void mapper_convert_session_to_command(int32_t command_id,
					int32_t tracking_session_id,
					command_t * const command)
{
	command->id = command_id;

	uint8_t i = 0U;

	command->argument_list[i].type = dt_frame;
	command->argument_list[i].value = tracking_session_id;
	++i;

	command->argument_count = i;

	return;
}

int mapper_convert_command_to_session(command_t const * const command,
					int32_t * const tracking_session_id)
{
	bool is_session_found = false;

	for(uint8_t i = 0U; i < command->argument_count; ++i)
	{
		switch(command->argument_list[i].type)
		{
			case dt_frame :
			{
				*tracking_session_id = command->argument_list[i].value;

				is_session_found = true;

				break;
			}
		}
	}

	if(is_session_found == true)
	{
		return SUCCESS;
	}
	return FAILURE;
}

void mapper_convert_summary_to_command(int32_t command_id,
					tracking_summary_t const * const tracking_summary,
					command_t * const command)
{
	command->id = command_id;

	uint8_t i = 0U;

	command->argument_list[i].type = dt_frame;
	command->argument_list[i].value = tracking_summary->session_id;
	++i;

	command->argument_list[i].type = dt_speed_kmh;
	command->argument_list[i].value = (int32_t)(tracking_summary->speed * 1000.0);
	++i;

	command->argument_count = i;

	return;
}

int mapper_convert_command_to_summary(command_t const * const command,
					tracking_summary_t * const tracking_summary)
{
	bool is_session_found = false;
	bool is_speed_found = false;

	for(uint8_t i = 0U; i < command->argument_count; ++i)
	{
		switch(command->argument_list[i].type)
		{
			case dt_frame :
			{
				tracking_summary->session_id = command->argument_list[i].value;

				is_session_found = true;

				break;
			}
			case dt_speed_kmh :
			{
				tracking_summary->speed = ((double)command->argument_list[i].value) / 1000.0;

				is_speed_found = true;

				break;
			}
		}
	}

	if((is_session_found == true) && (is_speed_found == true))
	{
		return SUCCESS;
	}
	return FAILURE;
}


int mapper_serialize_command(command_t const * const command,
				uint8_t max_data_size,
				uint8_t *serialized_data,
				uint8_t * const data_size)
{
	ml_encoder_t mote_xml_encoder;

	if(MLE_initialize(&mote_xml_encoder, serialized_data, max_data_size) == ML_SUCCESS)
	{
        	const uint8_t body_object = MLE_appendO(&mote_xml_encoder, dt_packet);
		{
			const uint8_t command_object = MLE_appendOSV(&mote_xml_encoder, dt_actions, body_object, command->id);
			{
				for(uint8_t i = 0U; i < command->argument_count; ++i)
				{
					MLE_appendOSV(&mote_xml_encoder, command->argument_list[i].type, command_object, command->argument_list[i].value);
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

int mapper_deserialize_command(uint8_t const * const serialized_data,
				uint8_t data_size,
				command_t * const command)
{
	ml_iterator_t mote_xml_itr;

	if(MLI_initialize(&mote_xml_itr, serialized_data, data_size) == ML_SUCCESS)
	{
		ml_object_t mote_xml_object;

		const uint8_t command_object = MLI_nextWithType(&mote_xml_itr, dt_actions, &mote_xml_object);

		if(command_object != 0U)
		{
			command->id = mote_xml_object.value;

			uint8_t i = 0U;

			while(MLI_nextWithSubject(&mote_xml_itr, command_object, &mote_xml_object) != 0U) {

				command->argument_list[i].type = mote_xml_object.type;
				command->argument_list[i].value = mote_xml_object.value;

				++i;
			}
			command->argument_count = i;

			return SUCCESS;
		}
	}
	return FAILURE;
}

