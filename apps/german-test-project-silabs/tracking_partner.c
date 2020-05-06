#include "tracking_partner.h"

#include "loglevels.h"
#define __MODUUL__ "tracking_partner"
#define __LOG_LEVEL__ (LOG_LEVEL_tracking_partner)
#include "log.h"

#include "radio_socket.h"
#include "tracking_partner.mapper.h"
#include "tracking_summary.h"
#include "result.h"


#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


typedef enum tracking_partner_command
{
	DO_NOTHING = 0,
	START_TRACKING_SESSION,
	STOP_TRACKING_SESSION,
	PROCESS_TRACKING_SUMMARY,
	CATCH_TRACKING_FAILURE

} tracking_partner_command_t;


static int tracking_partner_send_command(tracking_partner_t * const tracking_partner, command_t const * const command);
static void tracking_partner_catch_sending_error(void *user_data);
static void tracking_partner_receive_command(radio_message_t const * const raw_message, uint8_t data_size, void *user_data);

void tracking_partner_init(tracking_partner_t * const tracking_partner,
				tracking_session_state_set_t set_session_state_callback,
				tracking_summary_process_t process_summary_callback,
				tracking_failure_catch_t catch_failure_callback)
{
	tracking_partner->set_session_state_callback = set_session_state_callback;
	tracking_partner->process_summary_callback = process_summary_callback;
	tracking_partner->catch_failure_callback = catch_failure_callback;

	return;
}

int tracking_partner_setup_radio_network(tracking_partner_t * const tracking_partner,
					comms_layer_t *radio,
					am_addr_t partner_address,
					am_id_t port)
{
	radio_socket_init(&tracking_partner->radio_socket, radio, partner_address, port, tracking_partner_catch_sending_error);

	if(radio_socket_set_receiver(&tracking_partner->radio_socket, tracking_partner_receive_command, (void*)tracking_partner) == SUCCESS)
	{
		return SUCCESS;
	}
	return FAILURE;
}

int tracking_partner_start_session(tracking_partner_t * const tracking_partner,
					int32_t tracking_session_id)
{
	command_t command;
	mapper_convert_session_to_command(START_TRACKING_SESSION, tracking_session_id, &command);

	return tracking_partner_send_command(tracking_partner, &command);
}

int tracking_partner_stop_session(tracking_partner_t * const tracking_partner,
					int32_t tracking_session_id)
{
	command_t command;
	mapper_convert_session_to_command(STOP_TRACKING_SESSION, tracking_session_id, &command);

	return tracking_partner_send_command(tracking_partner, &command);
}

int tracking_partner_send_summary(tracking_partner_t * const tracking_partner,
					tracking_summary_t const * const tracking_summary)
{
	command_t command;
	mapper_convert_summary_to_command(PROCESS_TRACKING_SUMMARY, tracking_summary, &command);

	return tracking_partner_send_command(tracking_partner, &command);
}

int tracking_partner_send_failure(tracking_partner_t * const tracking_partner,
					int32_t tracking_session_id)
{
	command_t command;
	mapper_convert_session_to_command(CATCH_TRACKING_FAILURE, tracking_session_id, &command);

	return tracking_partner_send_command(tracking_partner, &command);
}

int tracking_partner_send_command(tracking_partner_t * const tracking_partner, command_t const * const command)
{
	bool is_radio_valid;
	radio_socket_is_valid(&tracking_partner->radio_socket, &is_radio_valid);

	if(is_radio_valid == true)
	{
		radio_message_t *radio_message = NULL;
		uint8_t max_data_size;

		if(radio_socket_alloc_message(&tracking_partner->radio_socket, &radio_message, &max_data_size) == SUCCESS)
		{
			uint8_t data_size;

			if(mapper_serialize_command(command, max_data_size, radio_message->data, &data_size) == SUCCESS)
			{
				if(radio_socket_send_message(&tracking_partner->radio_socket, data_size, (void*)tracking_partner) == SUCCESS)
				{
					return SUCCESS;
				}
			}
		}
	}
	return FAILURE;
}


void tracking_partner_catch_sending_error(void *user_data)
{
	tracking_partner_t const * const tracking_partner = (tracking_partner_t*)user_data;

	tracking_partner->catch_failure_callback(COMMUNICATION_ERROR);

	return;
}


// Receiving messages from partner traffic monitoring system
typedef void (*command_f) (command_t const * const command, void *user_data);

static void command_do_nothing(command_t const * const command, void *user_data);
static void command_start_tracking_session(command_t const * const command, void *user_data);
static void command_stop_tracking_session(command_t const * const command, void *user_data);
static void command_process_tracking_summary(command_t const * const command, void *user_data);
static void command_catch_tracking_failure(command_t const * const command, void *user_data);

static const command_f commands[] = {

	[DO_NOTHING]			= command_do_nothing,
	[START_TRACKING_SESSION]	= command_start_tracking_session,
	[STOP_TRACKING_SESSION]		= command_stop_tracking_session,
	[PROCESS_TRACKING_SUMMARY]	= command_process_tracking_summary,
	[CATCH_TRACKING_FAILURE]	= command_catch_tracking_failure
};

void tracking_partner_receive_command(radio_message_t const * const raw_message, uint8_t data_size, void *user_data)
{
	command_t command;

	if(mapper_deserialize_command(raw_message->data, data_size, &command) == SUCCESS)
	{
		if(command.id < ARRAY_SIZE(commands))
		{
			commands[command.id](&command, user_data);
		}
		else
		{
			warn1("rcv UNKNOWN COMMAND");
		}
	}
	else
	{
		warn1("rcv COMMAND PARSING ERROR");
	}
	return;
}


void command_do_nothing(command_t const * const command, void *user_data)
{
	debug1("COMMAND : do_nothing()");

	return;
}

void command_start_tracking_session(command_t const * const command, void *user_data)
{
	debug1("COMMAND : start_tracking_session()");

	int32_t tracking_session_id;

	if(mapper_convert_command_to_session(command, &tracking_session_id) == SUCCESS)
	{
		tracking_partner_t const * const tracking_partner = (tracking_partner_t*)user_data;

		const bool is_needed_to_start = true;

		tracking_partner->set_session_state_callback(tracking_session_id, is_needed_to_start);
	}
	return;
}

void command_stop_tracking_session(command_t const * const command, void *user_data)
{
	debug1("COMMAND : stop_tracking_session()");

	int32_t tracking_session_id;

	if(mapper_convert_command_to_session(command, &tracking_session_id) == SUCCESS)
	{
		tracking_partner_t const * const tracking_partner = (tracking_partner_t*)user_data;

		const bool is_needed_to_start = false;

		tracking_partner->set_session_state_callback(tracking_session_id, is_needed_to_start);
	}
	return;
}

void command_process_tracking_summary(command_t const * const command, void *user_data)
{
	debug1("COMMAND : process_tracking_summary()");

	tracking_summary_t tracking_summary;

	if(mapper_convert_command_to_summary(command, &tracking_summary) == SUCCESS)
	{
		tracking_partner_t const * const tracking_partner = (tracking_partner_t*)user_data;

		tracking_partner->process_summary_callback(&tracking_summary);
	}
	return;
}

void command_catch_tracking_failure(command_t const * const command, void *user_data)
{
	debug1("COMMAND : catch_tracking_failure()");

	int32_t tracking_session_id;

	if(mapper_convert_command_to_session(command, &tracking_session_id) == SUCCESS)
	{
		tracking_partner_t const * const tracking_partner = (tracking_partner_t*)user_data;

		tracking_partner->catch_failure_callback(TRACKING_ERROR);
	}
	return;
}

