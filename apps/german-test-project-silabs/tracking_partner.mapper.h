#ifndef TRACKING_PARTNER_MAPPER_H_
#define TRACKING_PARTNER_MAPPER_H_

#include <stdint.h>

typedef struct tracking_summary tracking_summary_t;

typedef struct argument
{
	uint32_t type;
	int32_t value;

} argument_t;

typedef struct command
{
	int32_t id;

	argument_t argument_list[8];
	uint8_t argument_count;

} command_t;

void mapper_convert_session_to_command(int32_t command_id,
					int32_t tracking_session_id,
					command_t * const command);

int mapper_convert_command_to_session(command_t const * const command,
					int32_t * const tracking_session_id);

void mapper_convert_summary_to_command(int32_t command_id,
					tracking_summary_t const * const tracking_summary,
					command_t * const command);

int mapper_convert_command_to_summary(command_t const * const command,
					tracking_summary_t * const tracking_summary);


int mapper_serialize_command(command_t const * const command,
				uint8_t max_data_size,
				uint8_t *serialized_data,
				uint8_t * const data_size);

int mapper_deserialize_command(uint8_t const * const serialized_data,
				uint8_t data_size,
				command_t * const command);

#endif
