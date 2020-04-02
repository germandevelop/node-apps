#include "tracking_partner.h"

#include "loglevels.h"
#define __MODUUL__ "partner_system"
#define __LOG_LEVEL__ (LOG_LEVEL_sensor_sim & BASE_LOG_LEVEL)
#include "log.h"

#include "radio_socket.h"
#include "tracking_report.mapper.h"
#include "result.h"


#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


typedef enum radio_command
{
	DO_NOTHING = 0,
	TURN_ON_RADAR,
	TURN_OFF_RADAR,
	PROCESS_TRACKING_REPORT

} radio_command_t;


static int tracking_partner_send_command(tracking_partner_t * const tracking_partner, radio_command_t command_id);
static void tracking_partner_receive_message(radio_message_t const * const raw_message, uint8_t data_size, void *user);

int tracking_partner_init(tracking_partner_t * const tracking_partner,
				tracking_state_set_f set_state_callback,
				tracking_report_process_f process_report_callback)
{
	tracking_partner->set_state_callback = set_state_callback;
	tracking_partner->process_report_callback = process_report_callback;

	if(radio_socket_init(&tracking_partner->radio_socket, tracking_partner_receive_message) == SUCCESS)
	{
		return SUCCESS;
	}
	return FAILURE;
}

int tracking_partner_turn_on_radar(tracking_partner_t * const tracking_partner)
{
	return tracking_partner_send_command(tracking_partner, TURN_ON_RADAR);
}

int tracking_partner_turn_off_radar(tracking_partner_t * const tracking_partner)
{
	return tracking_partner_send_command(tracking_partner, TURN_OFF_RADAR);
}

int tracking_partner_send_command(tracking_partner_t * const tracking_partner, radio_command_t command_id)
{
	bool is_radio_valid;
	radio_socket_is_valid(&tracking_partner->radio_socket, &is_radio_valid);

	if(is_radio_valid == true)
	{
		radio_message_t *radio_message = NULL;
		uint8_t max_data_size;

		if(radio_socket_alloc_message(&tracking_partner->radio_socket, &radio_message, &max_data_size) == SUCCESS)
		{
			radio_message->command_id = command_id;

			if(radio_socket_send_message(&tracking_partner->radio_socket, 0U, (void*)tracking_partner) == SUCCESS)
			{
				return SUCCESS;
			}
		}
	}
	return FAILURE;
}

int tracking_partner_send_tracking_report(tracking_partner_t * const tracking_partner,
						tracking_report_t const * const tracking_report)
{
	bool is_radio_valid;
	radio_socket_is_valid(&tracking_partner->radio_socket, &is_radio_valid);

	if(is_radio_valid == true)
	{
		radio_message_t *radio_message = NULL;
		uint8_t max_data_size;

		if(radio_socket_alloc_message(&tracking_partner->radio_socket, &radio_message, &max_data_size) == SUCCESS)
		{
			radio_message->command_id = PROCESS_TRACKING_REPORT;

			uint8_t data_size;

			if(tracking_report_serialize_to_mote_xml(tracking_report, max_data_size, radio_message->data, &data_size) == SUCCESS)
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


// Receiving messages from partner traffic monitoring system
typedef void (*command_f) (radio_message_t const * const raw_message, uint8_t data_size, void *user);

static void command_do_nothing(radio_message_t const * const raw_message, uint8_t data_size, void *user);
static void command_turn_on_radar(radio_message_t const * const raw_message, uint8_t data_size, void *user);
static void command_turn_off_radar(radio_message_t const * const raw_message, uint8_t data_size, void *user);
static void command_process_tracking_report(radio_message_t const * const raw_message, uint8_t data_size, void *user);

static const command_f commands[] = {

	[DO_NOTHING]			= command_do_nothing,
	[TURN_ON_RADAR]			= command_turn_on_radar,
	[TURN_OFF_RADAR]		= command_turn_off_radar,
	[PROCESS_TRACKING_REPORT]	= command_process_tracking_report
};

void command_do_nothing(radio_message_t const * const raw_message, uint8_t data_size, void *user)
{
	debug1("COMMAND : do_nothing()");

	return;
}

void command_turn_on_radar(radio_message_t const * const raw_message, uint8_t data_size, void *user)
{
	debug1("COMMAND : turn_on_radar()");

	tracking_partner_t const * const tracking_partner = (tracking_partner_t*)user;

	tracking_partner->set_state_callback(true);

	return;
}

void command_turn_off_radar(radio_message_t const * const raw_message, uint8_t data_size, void *user)
{
	debug1("COMMAND : turn_off_radar()");

	tracking_partner_t const * const tracking_partner = (tracking_partner_t*)user;

	tracking_partner->set_state_callback(false);

	return;
}

void command_process_tracking_report(radio_message_t const * const raw_message, uint8_t data_size, void *user)
{
	debug1("COMMAND : process_tracking_report()");

	tracking_report_t tracking_report;

	if(tracking_report_deserialize_from_mote_xml(raw_message->data, data_size, &tracking_report) == SUCCESS)
	{
		tracking_partner_t const * const tracking_partner = (tracking_partner_t*)user;

		tracking_partner->process_report_callback(&tracking_report);
	}
	return;
}


void tracking_partner_receive_message(radio_message_t const * const raw_message, uint8_t data_size, void *user)
{
	const uint8_t cmd_id = raw_message->command_id;

	if(cmd_id < ARRAY_SIZE(commands))
	{
		commands[cmd_id](raw_message, data_size, user);
	}
	else
	{
		warn1("rcv UNKNOWN COMMAND");
	}
	return;
}

